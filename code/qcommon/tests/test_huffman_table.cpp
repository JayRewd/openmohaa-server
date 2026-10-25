/*
===========================================================================
Copyright (C) 2026 the OpenMoHAA team

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

// Huff_offsetTransmitCode must write exactly the bits Huff_offsetTransmit
// writes, including the stop at maxoffset.

#include "../q_shared.h"
#include "../qcommon.h"

#include <cstdio>
#include <cstring>

static unsigned int rngState = 12345u;

static unsigned int Rand()
{
    rngState = rngState * 1103515245u + 12345u;
    return rngState >> 8;
}

static huffman_t       huffs;
static huff_t         &tree = huffs.compressor;
static huffCodeTable_t table;

static void BuildTree(int skew)
{
    // Same setup MSG_initHuffman uses for the message compressor
    Huff_Init(&huffs);

    for (int ch = 0; ch < 256; ch++) {
        int count = 1 + (int)(Rand() % (skew ? (ch < 8 ? 20000 : 3) : 2000));

        for (int j = 0; j < count; j++) {
            Huff_addRef(&tree, (byte)ch);
        }
    }

    Huff_BuildCodeTable(&tree, &table);
}

static int RunTrials(int trials)
{
    byte bufA[2048];
    byte bufB[2048];

    for (int t = 0; t < trials; t++) {
        int offsetA, offsetB;
        int maxoffset;
        int symbols = 1 + (int)(Rand() % 64);

        for (size_t i = 0; i < sizeof(bufA); i++) {
            bufA[i] = bufB[i] = (byte)Rand();
        }

        offsetA = offsetB = (int)(Rand() % 4000);
        // Include limits inside, at and just past the written range
        maxoffset = offsetA + (int)(Rand() % 400) - 8;
        if (maxoffset < 0) {
            maxoffset = 0;
        }

        for (int s = 0; s < symbols; s++) {
            int ch = (int)(Rand() % 256);

            Huff_offsetTransmit(&tree, ch, bufA, &offsetA, maxoffset);
            Huff_offsetTransmitCode(&table, &tree, ch, bufB, &offsetB, maxoffset);

            if (offsetA != offsetB) {
                printf("FAIL trial %d symbol %d: offset %d vs %d\n", t, ch, offsetA, offsetB);
                return 1;
            }
        }

        if (memcmp(bufA, bufB, sizeof(bufA))) {
            printf("FAIL trial %d: buffers differ\n", t);
            return 1;
        }
    }

    return 0;
}

// Huff_putBits must leave the buffer and offset exactly as n Huff_putBit calls.
static int TestPutBits(int trials)
{
    byte bufA[512];
    byte bufB[512];

    for (int t = 0; t < trials; t++) {
        int offsetA, offsetB;

        for (size_t i = 0; i < sizeof(bufA); i++) {
            bufA[i] = bufB[i] = (byte)Rand();
        }
        offsetA = offsetB = (int)(Rand() % 2000);

        for (int s = 0; s < 32; s++) {
            int          n     = 1 + (int)(Rand() % 32);
            unsigned int value = Rand() ^ (Rand() << 16);

            if (n < 32) {
                value &= (1u << n) - 1;
            }
            for (int k = 0; k < n; k++) {
                Huff_putBit((int)((value >> k) & 1), bufA, &offsetA);
            }
            Huff_putBits(value, n, bufB, &offsetB);

            if (offsetA != offsetB) {
                printf("FAIL putBits trial %d: offset %d vs %d\n", t, offsetA, offsetB);
                return 1;
            }
        }

        if (memcmp(bufA, bufB, sizeof(bufA))) {
            printf("FAIL putBits trial %d: buffers differ\n", t);
            return 1;
        }
    }

    return 0;
}

int main()
{
    int tabled = 0;

    if (TestPutBits(20000)) {
        return 1;
    }

    for (int round = 0; round < 8; round++) {
        BuildTree(round & 1);

        for (int ch = 0; ch < 256; ch++) {
            tabled += table.length[ch] >= 0;
        }

        if (RunTrials(20000)) {
            return 1;
        }
    }

    if (!tabled) {
        printf("FAIL: no symbol used the table\n");
        return 1;
    }

    printf("huffman table matches tree walk (%d tabled symbols)\n", tabled);
    return 0;
}
