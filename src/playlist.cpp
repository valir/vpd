/*
 * This file is part of the Video Player Daemon
 *
 * Copyright (C) 2015 Valentin Rusu vpd@rusu.info
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

PlaylistItem Playlist::current() noexcept {
    if (items_.empty() || current_ == items_.size())
        return PlaylistItem();
    else
        return items_[current_];
}

PlaylistItem Playlist::next() noexcept {
    if (items_.empty() || current_ == items_.size()) {
        return PlaylistItem();
    } else {
        return items_[current_++];
    }
}

PlaylistItem Playlist::prev() noexcept {
    if (current_ == 0) {
        if (items_.empty())
            return PlaylistItem();
        else
            return items_[0];
    } else {
        return items_[--current_];
    }
}

void Playlist::clear() {
    items_.clear();
    current_ = 0;
    dirty_ = false;
}

void Playlist::add(std::string uri) {
    BOOST_LOG_TRIVIAL(debug) << "Playlist: adding URI: " << uri;
    items_.emplace_back(uri);
    if (!dirty_) {
        // stop incrementing version if it's already dirty
        version_++;
    }
    dirty_ = true;
}

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
    dirty_ = false;
}

void Playlist::load(const PlaylistInfo &plinfo) {
    BOOST_LOG_TRIVIAL(debug) << "loading playlist " << plinfo.path_.string();
    // NOTE silently clear the list here, even if it's dirty
    items_.clear();
    dirty_ = false;
    version_ = 0;
    std::ifstream ifs(plinfo.path_.string());
    std::string line;
    while (std::getline(ifs, line)) {
        std::string name;
        if (line.at(0) == '#') {
            name = line.substr(2);
            if (!std::getline(ifs, line)) {
                BOOST_LOG_TRIVIAL(error) << "The playlist " << plinfo.name() << " is malformed! Aborting load";
                items_.clear();
                break;
            }
        }
        BOOST_LOG_TRIVIAL(debug) << "load: appending " << line << ", name: " << name;
        items_.emplace_back(line, name);
    }
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
PlaylistInfoPtr PlaylistInfo::fromPath(fs::path path, std::string name) {
    assert(fs::exists(path));
    name += ".m3u";
    path /= name;
    if (fs::exists(path)) {
        return std::make_shared<PlaylistInfo>(path);
    } else {
        BOOST_LOG_TRIVIAL(warning) << "OOPS! Playlist " << path.string() << " not found";
        return PlaylistInfoPtr();
    }
}
