/*
 * This file is part of the VPD project
 *
 * Copyright (C) 2014 Valentin Rusu vpd@rusu.info
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef  PLAY_ENGINE_INC
#define  PLAY_ENGINE_INC

#include "runtime_config.h"
#include "client_engine.h"
#include "player.h"

namespace PlayEngine {

int start(const RuntimeConfig &config);
void sessionClosed(ClientEngine::ClientSessionPtr sessionPtr);
void play(std::string uri);
void play(int pos);
void stop();
void next();
void prev();

// playlist commands
void add(const std::string& uri);
void enumeratePlaylist(EnumPlaylistFn fn);
void clearPlaylist();

// playlist persistence commands
void save(const std::string& filename);
void enumeratePlaylists(EnumPlaylistsFn fn);
void load(const std::string& listname);

// status commands
const Player::Status& status();

} // namespace PlayEngine

#endif // PLAY_ENGINE_INC
