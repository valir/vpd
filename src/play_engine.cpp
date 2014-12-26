/*
 * This file is part of the VPD project
 * 
 * Copyright (C) 2014 Valentin Rusu kde@rusu.info
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
#include "play_engine.h"
#include "client_connection.h"

#include <thread>
#include <future>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/array.hpp>

namespace PlayEngine {

using namespace ClientConnection;
namespace bp = boost::process;
namespace io = boost::asio;

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

struct PlayerExecInfo
{
    string  execPath_;
    std::vector<string> execArgs_;
    bp::context ctx_;
};

void accept_remote_connection();
int playSopcast();

io::io_service ioservice;
io::signal_set signals(ioservice);
io::ip::tcp::acceptor acceptor(ioservice);
io::ip::tcp::socket remoteSocket(ioservice);

int start (const RuntimeConfig &config) {
    BOOST_LOG_TRIVIAL(debug) << "PlayEngine: starting up...";

    signals.add(SIGINT);
    signals.add(SIGTERM);
    signals.add(SIGQUIT);

    io::ip::tcp::resolver resolver(ioservice);
    io::ip::tcp::endpoint endpoint = *resolver.resolve({config.listen_address_, config.listen_port_});
    acceptor.open(endpoint.protocol());
    acceptor.set_option(io::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    accept_remote_connection();

    for (;;) {
        try {
            io::io_service::work work(ioservice);
            break;
        }
        catch (...) {
            BOOST_LOG_TRIVIAL(error) << "PlayEngine: Unknown exception occurred.";
        }
    }

    return 0;
}

void accept_remote_connection() {
    acceptor.async_accept(remoteSocket, 
        [](const boost::system::error_code &ec) {
            if (!acceptor.is_open()) {
                return;
            }

            if (!ec) {
                ClientConnection::start(std::move(remoteSocket));
            }
            accept_remote_connection();
        });
}

int playSopcast() {
    PlayerExecInfo sopcast;
    // TODO this should be configurable as it platform dependant
    sopcast.execPath_ = bp::find_executable_in_path("sp-sc-auth");
    if (sopcast.execPath_.length() == 0) {
        BOOST_LOG_TRIVIAL(error) << "Cannot find sp-sc-auth executable!";
        return 1;
    }
    BOOST_LOG_TRIVIAL(debug) << "found " << sopcast.execPath_;
    sopcast.execArgs_ = { "sop://broker.sopcast.com:3912/151929", "1234", "12345" }; // play protv
    sopcast.ctx_.process_name = "sopcast";
    // this will sink sp-sc-auth output, which is somewhat verbose
    sopcast.ctx_.streams[bp::stdout_id] = bp::behavior::pipe();


    PlayerExecInfo vlc;
    // TODO let the cvlc exec name be configurable
    vlc.execPath_ = bp::find_executable_in_path("cvlc");
    BOOST_LOG_TRIVIAL(debug) << "found " << vlc.execPath_;
    vlc.execArgs_ = {
        // TODO add an ifdef here to insert these two arguments on the RPI platform
        //        "--vout", "omxil_vout", 
        "http://localhost:12345/tv.asf"
    };
    vlc.ctx_.process_name = "cvlc";
//    vlc.ctx_.streams[bp::stdin_id] = bp::behavior::pipe(); 

    bp::child sopcast_child = bp::create_child(sopcast.execPath_, sopcast.execArgs_, sopcast.ctx_);
    BOOST_LOG_TRIVIAL(debug) << "sopcast receiver started. standby for clvlc launch...";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    bp::child vlc_child = bp::create_child(vlc.execPath_, vlc.execArgs_, vlc.ctx_);
    BOOST_LOG_TRIVIAL(debug) << "cvlc launched. playback should start shortly...";

    bp::handle read_h = sopcast_child.get_handle(bp::stdout_id);
    bp::pipe sop_stdout(ioservice, read_h.release());
    //
    // // bp::handle write_h = vlc_child.get_handle(bp::stdin_id);
    // // bp::pipe video_buffer_out(ioservice, write_h.release());
    //
    // bp::postream vlc_stdin(vlc_child.get_handle(bp::stdin_id));
    StdOutNullDevice dropSopcastStdout(sop_stdout);
    return 0;
}

} // namespace PlayEngine

