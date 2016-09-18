/*
 *      Copyright (C) 2015, 2016  Jamal Edey
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#ifndef ACTION_H
#define ACTION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // STB
    STB_HANDSHAKE,
    STB_GET_PROFILE,
    STB_DO_AUTH,

    // ITV
    ITV_GET_ALL_CHANNELS,
    ITV_GET_ORDERED_LIST,
    ITV_CREATE_LINK,
    ITV_GET_GENRES,
    ITV_GET_EPG_INFO,

    // WATCHDOG
    WATCHDOG_GET_EVENTS
} sc_action_t;

#ifdef __cplusplus
}
#endif

#endif /* ACTION_H */
