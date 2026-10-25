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

#pragma once

#include "../qcommon/q_shared.h"

static const unsigned int NAVMESH_CACHE_MAGIC = ('O' << 24) | ('M' << 16) | ('N' << 8) | 'V';

#pragma pack(push, 1)
struct NavmeshCacheHeader {
    uint32_t magic;
    uint32_t headerVersion;
    int32_t  bspLength;
    uint32_t bspChecksum;
    uint32_t mapNameLen;
    char     mapName[64];
    uint32_t tileCount;
    uint32_t reserved0;
};
#pragma pack(pop)

struct NavmeshCacheKey {
    char     mapQPath[64];
    int32_t  bspLength;
    uint32_t bspChecksum;
    uint32_t cacheVersion;
};

#ifndef NAVMESH_CACHE_UNIT_TEST
// Build key by reading BSP via gi.FS_ReadFile and Com_BlockChecksum.
bool NavmeshCache_BuildKey(const char *mapQPath, NavmeshCacheKey *out);
#endif

// "navmesh/dm/mohdm6.bin" from "maps/dm/mohdm6.bsp" or "_sml.bsp"
bool NavmeshCache_PathForMap(const char *mapQPath, char *out, size_t outSize);

// Validate header against key; also walk tile sizes if tileCount present.
// fileBuf/fileLen are the full file. On success, sets *tileData and *tileSize for tile 0.
bool NavmeshCache_Validate(const void *fileBuf, long fileLen, const NavmeshCacheKey *want,
                           const unsigned char **tileData, int *tileSize);

// Fill header from key for writing (tileCount=1).
void NavmeshCache_FillHeader(NavmeshCacheHeader *hdr, const NavmeshCacheKey *key);
