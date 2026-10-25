/*
===========================================================================
Copyright (C) 2025 the OpenMoHAA team

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenMoHAA source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenMoHAA source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "navigation_recast_cache.h"
#include "navigation_recast_config.h"

#ifndef NAVMESH_CACHE_UNIT_TEST
#include "g_local.h"

extern "C" unsigned Com_BlockChecksum(const void *buffer, int length);
#endif

static bool NavmeshCache_IsPathChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')
           || c == '_' || c == '.' || c == '/' || c == '-';
}

#ifndef NAVMESH_CACHE_UNIT_TEST
bool NavmeshCache_BuildKey(const char *mapQPath, NavmeshCacheKey *out)
{
    void    *buf;
    long     len;
    unsigned checksum;

    if (!mapQPath || !out) {
        return false;
    }

    if (strlen(mapQPath) >= sizeof(out->mapQPath)) {
        return false;
    }

    buf = NULL;
    len = gi.FS_ReadFile(mapQPath, &buf, qtrue);
    if (len <= 0 || !buf) {
        return false;
    }

    checksum = Com_BlockChecksum(buf, (int)len);

    memset(out, 0, sizeof(*out));
    Q_strncpyz(out->mapQPath, mapQPath, sizeof(out->mapQPath));
    out->bspLength    = (int32_t)len;
    out->bspChecksum  = checksum;
    out->cacheVersion = NAVMESH_CACHE_VERSION;

    gi.FS_FreeFile(buf);
    return true;
}
#endif

bool NavmeshCache_PathForMap(const char *mapQPath, char *out, size_t outSize)
{
    char   rel[MAX_QPATH];
    size_t len;
    size_t i;

    if (!mapQPath || !out || outSize == 0) {
        return false;
    }

    Q_strncpyz(rel, mapQPath, sizeof(rel));

    for (i = 0; rel[i]; i++) {
        if (rel[i] == '\\') {
            rel[i] = '/';
        }
    }

    if (!Q_stricmpn(rel, "maps/", 5)) {
        memmove(rel, rel + 5, strlen(rel + 5) + 1);
    }

    len = strlen(rel);
    if (len >= 8 && !Q_stricmp(rel + len - 8, "_sml.bsp")) {
        rel[len - 8] = '\0';
    } else if (len >= 4 && !Q_stricmp(rel + len - 4, ".bsp")) {
        rel[len - 4] = '\0';
    }

    if (!rel[0] || rel[0] == '/') {
        return false;
    }

    if (strstr(rel, "..")) {
        return false;
    }

    for (i = 0; rel[i]; i++) {
        if (!NavmeshCache_IsPathChar(rel[i])) {
            return false;
        }
    }

    // Reject a trailing slash so we never emit "navmesh/.bin" or "navmesh/dm/.bin"
    len = strlen(rel);
    if (rel[len - 1] == '/') {
        return false;
    }

    Com_sprintf(out, outSize, "navmesh/%s.bin", rel);
    return out[0] != '\0';
}

bool NavmeshCache_Validate(
    const void           *fileBuf,
    long                  fileLen,
    const NavmeshCacheKey *want,
    const unsigned char **tileData,
    int                  *tileSize
)
{
    NavmeshCacheHeader   hdr;
    const unsigned char *cursor;
    long                 remaining;
    uint32_t             i;
    int32_t              dataSize;

    if (!fileBuf || !want || !tileData || !tileSize) {
        return false;
    }

    if (fileLen < (long)sizeof(NavmeshCacheHeader)) {
        return false;
    }

    memcpy(&hdr, fileBuf, sizeof(hdr));
    hdr.magic         = LittleLong(hdr.magic);
    hdr.headerVersion = LittleLong(hdr.headerVersion);
    hdr.bspLength     = LittleLong(hdr.bspLength);
    hdr.bspChecksum   = LittleLong(hdr.bspChecksum);
    hdr.mapNameLen    = LittleLong(hdr.mapNameLen);
    hdr.tileCount     = LittleLong(hdr.tileCount);

    if (hdr.magic != NAVMESH_CACHE_MAGIC) {
        return false;
    }
    if (hdr.headerVersion != want->cacheVersion) {
        return false;
    }
    if (hdr.bspLength != want->bspLength) {
        return false;
    }
    if (hdr.bspChecksum != want->bspChecksum) {
        return false;
    }
    if (hdr.mapNameLen >= sizeof(hdr.mapName)) {
        return false;
    }
    {
        // The name comes from disk: find its end inside the field only.
        const char *nul = (const char *)memchr(hdr.mapName, 0, sizeof(hdr.mapName));

        if (!nul || (uint32_t)(nul - hdr.mapName) != hdr.mapNameLen) {
            return false;
        }
    }
    if (Q_stricmp(hdr.mapName, want->mapQPath)) {
        return false;
    }
    if (hdr.tileCount == 0) {
        return false;
    }

    cursor    = (const unsigned char *)fileBuf + sizeof(NavmeshCacheHeader);
    remaining = fileLen - (long)sizeof(NavmeshCacheHeader);

    *tileData = NULL;
    *tileSize = 0;

    for (i = 0; i < hdr.tileCount; i++) {
        if (remaining < (long)sizeof(int32_t)) {
            return false;
        }

        memcpy(&dataSize, cursor, sizeof(dataSize));
        dataSize = LittleLong(dataSize);
        if (dataSize <= 0) {
            return false;
        }
        if (remaining < (long)sizeof(int32_t) + dataSize) {
            return false;
        }

        if (i == 0) {
            *tileData = cursor + sizeof(int32_t);
            *tileSize = dataSize;
        }

        cursor += sizeof(int32_t) + dataSize;
        remaining -= (long)sizeof(int32_t) + dataSize;
    }

    return *tileData != NULL && *tileSize > 0;
}

void NavmeshCache_FillHeader(NavmeshCacheHeader *hdr, const NavmeshCacheKey *key)
{
    if (!hdr || !key) {
        return;
    }

    memset(hdr, 0, sizeof(*hdr));
    hdr->magic         = NAVMESH_CACHE_MAGIC;
    hdr->headerVersion = key->cacheVersion;
    hdr->bspLength     = key->bspLength;
    hdr->bspChecksum   = key->bspChecksum;
    Q_strncpyz(hdr->mapName, key->mapQPath, sizeof(hdr->mapName));
    hdr->mapNameLen = (uint32_t)strlen(hdr->mapName);
    hdr->tileCount  = 1;
    hdr->reserved0  = 0;
}
