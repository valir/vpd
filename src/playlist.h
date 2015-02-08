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
#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <boost/log/trivial.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace fs = boost::filesystem;

struct PlaylistItem
{
    explicit PlaylistItem(std::string uri = "") :
        uri_(uri)
        , id_(-1)
        {}
    bool empty() const noexcept { return uri_.empty(); }
    std::string uri_;
    std::string name_;
    int id_;
};

using EnumPlaylistFn = std::function< void(const PlaylistItem&) >;

struct Playlist
{
    Playlist() : version_(0), current_(0) {}
    void add(std::string uri) {
        BOOST_LOG_TRIVIAL(debug) << "Playlist: adding URI: " << uri;
        items_.emplace_back(uri);
        version_++;
    }
    PlaylistItem next() noexcept {
        if (items_.empty() || current_ == items_.size()) {
            return PlaylistItem();
        } else {
            return items_[current_++];
        }
    }
    void enumerate(EnumPlaylistFn fn) const;
    void save(fs::path path, const std::string &name);
    std::vector<PlaylistItem> items_;
    uint32_t version_;
    size_t current_;
};

struct PlaylistInfo;
using EnumPlaylistsFn = std::function< void(const PlaylistInfo&) >;
struct PlaylistInfo
{
    PlaylistInfo(const fs::path &path) : path_(path) {}
    fs::path path_;

    static void enumeratePlaylists(const fs::path &playlistsPath, EnumPlaylistsFn fn);

    std::string name() const { return path_.filename().string(); }
    std::time_t lastModified() const { return fs::last_write_time(path_); }
};


#endif // PLAYLIST_H
