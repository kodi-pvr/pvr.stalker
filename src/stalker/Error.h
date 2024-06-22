/*
 *  Copyright (C) 2015-2021 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2016 Jamal Edey
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

typedef enum
{
  SERROR_AUTHORIZATION = -8,
  SERROR_STREAM_URL,
  SERROR_LOAD_EPG,
  SERROR_LOAD_CHANNEL_GROUPS,
  SERROR_LOAD_CHANNELS,
  SERROR_AUTHENTICATION,
  SERROR_API,
  SERROR_INITIALIZE,
  SERROR_UNKNOWN,
  SERROR_OK,
} SError;
