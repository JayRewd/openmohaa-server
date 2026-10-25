/*
===========================================================================
Copyright (C) 2026 the OpenMoHAA team

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "../navigation_recast_cache.h"
#include "../navigation_recast_config.h"

#include <cstdio>
#include <cstring>
#include <vector>

static int failures;

#define CHECK(condition)                                                          \
    do {                                                                          \
        if (!(condition)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                           \
        }                                                                         \
    } while (0)

static void test_path_for_map()
{
    char path[MAX_QPATH];

    CHECK(NavmeshCache_PathForMap("maps/dm/mohdm6.bsp", path, sizeof(path)));
    CHECK(!strcmp(path, "navmesh/dm/mohdm6.bin"));

    CHECK(NavmeshCache_PathForMap("maps/obj/obj_team2_sml.bsp", path, sizeof(path)));
    CHECK(!strcmp(path, "navmesh/obj/obj_team2.bin"));

    CHECK(NavmeshCache_PathForMap("dm/mohdm1.bsp", path, sizeof(path)));
    CHECK(!strcmp(path, "navmesh/dm/mohdm1.bin"));

    CHECK(!NavmeshCache_PathForMap("maps/../evil.bsp", path, sizeof(path)));
    CHECK(!NavmeshCache_PathForMap("maps//.bsp", path, sizeof(path)));
    CHECK(!NavmeshCache_PathForMap(NULL, path, sizeof(path)));
    CHECK(!NavmeshCache_PathForMap("maps/dm/mohdm6.bsp", NULL, sizeof(path)));
    CHECK(!NavmeshCache_PathForMap("maps/dm/mohdm6.bsp", path, 0));
}

static void test_fill_and_validate()
{
    NavmeshCacheKey    key;
    NavmeshCacheHeader hdr;
    const unsigned char *tileData;
    int                  tileSize;
    int32_t              dataSize;
    unsigned char        tileBytes[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<unsigned char> file;

    memset(&key, 0, sizeof(key));
    Q_strncpyz(key.mapQPath, "maps/dm/mohdm6.bsp", sizeof(key.mapQPath));
    key.bspLength    = 12345;
    key.bspChecksum  = 0xA1B2C3D4u;
    key.cacheVersion = NAVMESH_CACHE_VERSION;

    NavmeshCache_FillHeader(&hdr, &key);
    CHECK(hdr.magic == NAVMESH_CACHE_MAGIC);
    CHECK(hdr.headerVersion == NAVMESH_CACHE_VERSION);
    CHECK(hdr.bspLength == 12345);
    CHECK(hdr.bspChecksum == 0xA1B2C3D4u);
    CHECK(hdr.tileCount == 1);
    CHECK(!strcmp(hdr.mapName, key.mapQPath));
    CHECK(hdr.mapNameLen == (uint32_t)strlen(hdr.mapName));

    dataSize = (int32_t)sizeof(tileBytes);
    file.resize(sizeof(hdr) + sizeof(dataSize) + sizeof(tileBytes));
    memcpy(file.data(), &hdr, sizeof(hdr));
    memcpy(file.data() + sizeof(hdr), &dataSize, sizeof(dataSize));
    memcpy(file.data() + sizeof(hdr) + sizeof(dataSize), tileBytes, sizeof(tileBytes));

    tileData = NULL;
    tileSize = 0;
    CHECK(NavmeshCache_Validate(file.data(), (long)file.size(), &key, &tileData, &tileSize));
    CHECK(tileSize == (int)sizeof(tileBytes));
    CHECK(tileData && !memcmp(tileData, tileBytes, sizeof(tileBytes)));

    // Wrong checksum
    key.bspChecksum = 1;
    CHECK(!NavmeshCache_Validate(file.data(), (long)file.size(), &key, &tileData, &tileSize));
    key.bspChecksum = 0xA1B2C3D4u;

    // Truncated file
    CHECK(!NavmeshCache_Validate(file.data(), (long)sizeof(hdr), &key, &tileData, &tileSize));

    // Map name field with no terminator (a corrupt file): rejected without
    // reading past the field
    {
        NavmeshCacheHeader bad = hdr;

        memset(bad.mapName, 'x', sizeof(bad.mapName));
        bad.mapNameLen = sizeof(bad.mapName) - 1;
        memcpy(file.data(), &bad, sizeof(bad));
        CHECK(!NavmeshCache_Validate(file.data(), (long)file.size(), &key, &tileData, &tileSize));
    }

    // Recorded length disagrees with the name
    {
        NavmeshCacheHeader bad = hdr;

        bad.mapNameLen = hdr.mapNameLen + 1;
        memcpy(file.data(), &bad, sizeof(bad));
        CHECK(!NavmeshCache_Validate(file.data(), (long)file.size(), &key, &tileData, &tileSize));
    }

    // Bad magic
    hdr.magic = 0;
    memcpy(file.data(), &hdr, sizeof(hdr));
    CHECK(!NavmeshCache_Validate(file.data(), (long)file.size(), &key, &tileData, &tileSize));
}

int main()
{
    failures = 0;
    test_path_for_map();
    test_fill_and_validate();

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("test_navmesh_cache: ok\n");
    return 0;
}
