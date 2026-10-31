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

// A passworded server must refuse a player who sends no password while
// sv_privatePassword is empty, its default.

#include "../g_password.h"

#include <cstdio>

static int failures;

#define CHECK(join, priv, sent, expected)                                                  \
    do {                                                                                   \
        if (G_JoinPasswordAccepted(join, priv, sent) != (expected)) {                      \
            fprintf(                                                                       \
                stderr, "FAIL %s:%d: password \"%s\", sv_privatePassword \"%s\", sent \"%s\": want %s\n", \
                __FILE__, __LINE__, join, priv, sent, (expected) ? "accepted" : "refused"  \
            );                                                                             \
            failures++;                                                                    \
        }                                                                                  \
    } while (0)

int main()
{
    failures = 0;

    // No join password: everyone gets in.
    CHECK("", "", "", true);
    CHECK("", "", "anything", true);
    CHECK("", "vip", "", true);

    // Join password only, the usual setup.
    CHECK("secret", "", "", false);
    CHECK("secret", "", "wrong", false);
    CHECK("secret", "", "secret", true);

    // Join password and reserved-slot password: either one gets in.
    CHECK("secret", "vip", "", false);
    CHECK("secret", "vip", "wrong", false);
    CHECK("secret", "vip", "secret", true);
    CHECK("secret", "vip", "vip", true);

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("test_join_password: ok\n");
    return 0;
}
