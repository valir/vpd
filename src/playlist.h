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

struct PlaylistItem
{
    explicit PlaylistItem(std::string uri) :
        uri_(uri)
        , id_(-1)
    {}
    std::string uri_;
    std::string name_;
    int id_;
};

using enumPlaylistFn = std::function< void(const PlaylistItem&) >;

struct Playlist
{
    Playlist() : version_(0) {}
    void add(std::string uri) {
        BOOST_LOG_TRIVIAL(debug) << "Playlist: adding URI: " << uri;
        items_.emplace_back(uri);
        version_++;
    }
    void enumerate(enumPlaylistFn fn) const;
    std::vector<PlaylistItem> items_;
    uint32_t version_;
};


#endif // PLAYLIST_H
