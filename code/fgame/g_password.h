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

// g_password.h: the join-password check in G_ClientConnect.
//
// A player gets in when the server has no join password (password), when
// they send it, or when they send the reserved-slot password
// (sv_privatePassword). The stock games compare the reserved-slot password
// even when it is empty, its default, so a player who sends no password
// matches it and skips the join password. Here it only counts when set.

#pragma once

#include <string.h>

static inline bool G_JoinPasswordAccepted(const char *joinPassword, const char *privatePassword, const char *sent)
{
    if (!*joinPassword || !strcmp(joinPassword, sent)) {
        return true;
    }

    return *privatePassword && !strcmp(privatePassword, sent);
}
