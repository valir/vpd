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

#include "player.h"
#include "playlist.h"

#include <thread>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/array.hpp>

namespace Player {

namespace bp = boost::process;
namespace io = boost::asio;

Playlist playlist_;
Status status_;


Playlist& playlist() {
    return playlist_;
}

void updateStatus() {
    status_.playlist_ = playlist_.version_;
    status_.playlistLength_ = playlist_.items_.size();
}

const Status& status() {
    updateStatus();
    return status_;
}

struct StdOutNullDevice
{
    StdOutNullDevice(bp::pipe &p) : p_(p) {
        p_.async_read_some(io::buffer(iobuffer_), *this);
    }
    void operator()(const boost::system::error_code &ec, std::size_t bytes) {
        if (!ec)
        {
            // simply eat those verbose output
            p_.async_read_some(io::buffer(iobuffer_), *this);
        } else {
            BOOST_LOG_TRIVIAL(error) << "Error signaled in the StdOutNullDevice: " << ec.message();
        }
    }
    bp::pipe &p_;
    boost::array<char, 65536> iobuffer_;
};

struct PlayerExec : public std::enable_shared_from_this<PlayerExec>
{
    using args_vector = std::vector<std::string>;
    PlayerExec(std::string execName, args_vector&& args = args_vector()) :
        execArgs_(args)
    {
        execPath_ = bp::find_executable_in_path(execName);
        if (execPath_.length() == 0) {
            BOOST_LOG_TRIVIAL(error) << "Cannot find " << execName << "executable!";
            setInvalid();
        } else {
            BOOST_LOG_TRIVIAL(debug) << "found " << execPath_;
            ctx_.process_name = execName;
        }
    }
    void setInvalid() { valid_ = false; }
    void start() {
        child_ = std::make_shared<bp::child>(bp::create_child(execPath_, execArgs_, ctx_));
        BOOST_LOG_TRIVIAL(debug) << ctx_.process_name << "process started. standby for clvlc launch...";
    }
private:
    bool valid_;
    std::string  execPath_;
    std::vector<std::string> execArgs_;
    bp::context ctx_;
    using child_ptr = std::shared_ptr<bp::child>;
    child_ptr child_;
};


int playSopcast(std::string uri) {
    using namespace std::literals;
    // TODO this should be configurable as it platform dependant
    // protv uri : "sop://broker.sopcast.com:3912/151929"
    PlayerExec sopcast("sp-sc-auth", { uri, "1234", "12345" });


    // TODO let the cvlc exec name be configurable
    PlayerExec vlc("cvlc", {
        // TODO add an ifdef here to insert these two arguments on the RPI platform
        //        "--vout", "omxil_vout",
        "http://localhost:12345/tv.asf"
    });

    sopcast.start();
    std::this_thread::sleep_for(5s);
    vlc.start();

    return 0;
}

bool play(std::string uri) {
    // NOTE should we use cpp-netlib for URI parsing ?
    BOOST_LOG_TRIVIAL(info) << "playing uri " << uri;
    auto proto_end = uri.find(':');
    if (proto_end == std::string::npos) {
        BOOST_LOG_TRIVIAL(warning) << "uri protocol not recognized. skipping.";
        return false;
    }
    std::string proto = uri.substr(0, proto_end);
    if (proto == "sop") {
        return playSopcast(uri);
    } else {
        BOOST_LOG_TRIVIAL(warning) << "uri protocol not recognized. skipping.";
        return false;
    }
    return true;
}

void play() {
    for (;;) {
        PlaylistItem item = playlist_.next();
        if (item.empty()) {
            BOOST_LOG_TRIVIAL(debug) << "Playlist is currently empty, or we reached its end.";
            return;
        }

        if (play(item.uri_))
            break;
        // no need to update status now. it'll get updated when client will ask for it
        // if the item could not be played, then automatically continue with next item
        BOOST_LOG_TRIVIAL(debug) << "Trying next item in the playlist";
    }
}

void stop() {

}

} // namespace
