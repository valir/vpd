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

#include "playlist.h"

#include <algorithm>
#include <fstream>
#include <boost/filesystem/operations.hpp>

void Playlist::enumerate(EnumPlaylistFn fn) const {
    for (const PlaylistItem &item : items_) {
        fn(item);
    }
}

void Playlist::save(fs::path path, const std::string& filename) {
    // NOTE has already been checked by the save command
    auto p = path /= filename;
    p += ".m3u";
    if (fs::exists(p)) {
        BOOST_LOG_TRIVIAL(warning) << "replacing playlist " << p.string();
    }

    std::ofstream ofs(p.string(), std::ofstream::out | std::ofstream::trunc);
    for (auto item : items_) {
        if (!item.name_.empty()) {
            ofs << "# " << item.name_ << std::endl;
        }
        ofs << item.uri_;
    };
}

void PlaylistInfo::enumeratePlaylists(const fs::path& playlistsPath, EnumPlaylistsFn fn) {
    assert(fs::exists(playlistsPath));
    fs::directory_iterator end_iter;
    for ( fs::directory_iterator it(playlistsPath); it != end_iter; it++ ) {
        if (fs::is_regular_file(it->path())) {
            // NOTE: I don't think we need to filter the m3u files here
            // was we own the containing directory
            PlaylistInfo info(it->path());
            fn(info);
        }
    }
}
