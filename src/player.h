 /*
 * This file is part of the Video Player Daemon
 *
 * Copyright (C) 2015 Valentin Rusu kde@rusu.info
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
#ifndef PLAYER_H
#define PLAYER_H

#include "playlist.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace Player {

using namespace boost::posix_time;

struct Status
{
    int volume_;
    bool repeat_;
    bool random_;
    bool single_;
    bool consume_;
    int playlist_;
    int playlistLength_;
    int videoClip_;;
    int videoClipId_;
    time_duration time_;
    time_duration elapsed_;
    std::string nextVideo_;
    int nextVideoId_;
};

Playlist& playlist();
const Status& status();

void play();
void stop();
void next();
void prev();

} // namespace
#endif // PLAYER_H
