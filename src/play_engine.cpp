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
#include "config.h"
#include "playlist.h"
#include "player.h"

#include <thread>
#include <future>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/array.hpp>

namespace PlayEngine {

namespace bp = boost::process;
namespace io = boost::asio;
using socket_t = io::ip::tcp::socket;

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

io::io_service ioservice_;
io::ip::tcp::acceptor acceptor(ioservice_);
io::signal_set signals(ioservice_);

void handle_signals() {
    signals.add(SIGINT);
    signals.add(SIGTERM);
    signals.add(SIGQUIT);
    signals.async_wait(
        [](const boost::system::error_code &ec, int signum) {
            if (ec) {
                BOOST_LOG_TRIVIAL(error) << "error ecountered when waiting for signals. code: " << ec.value() << "(" << ec.message() << ")";
            } else {
                BOOST_LOG_TRIVIAL(info) << "received signal " << signum << ". Quitting...";
                ioservice_.stop();
            }
        });
}



using ClientSessionPtr = std::shared_ptr<ClientEngine::ClientSession>;
std::list< ClientSessionPtr > clientSessions_;

void prepare_accept_remote_connection(const RuntimeConfig &config) {
    BOOST_LOG_TRIVIAL(info) << "listening on " << config.listen_address_ << ":" << config.listen_port_;
    io::ip::tcp::resolver resolver(ioservice_);
    io::ip::tcp::resolver::query query(config.listen_address_, std::to_string( config.listen_port_));
    io::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(io::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();
}

void accept_remote_connection(const RuntimeConfig &config) {
    io::ip::tcp::socket remoteSocket(ioservice_);
    auto session = std::make_shared<ClientEngine::ClientSession>(std::move(remoteSocket));
    acceptor.async_accept(session->socket(),
        [config, session](const boost::system::error_code &ec) {
            if (!acceptor.is_open()) {
                return;
            }

            if (!ec) {
                clientSessions_.emplace_back(session);
                session->start();
            }
            accept_remote_connection(config);
        });
}

void sessionClosed(ClientSessionPtr sessionPtr) {
    clientSessions_.remove(sessionPtr);
}


int start (const RuntimeConfig &config) {
    BOOST_LOG_TRIVIAL(debug) << "PlayEngine: starting up...";

    handle_signals();

    prepare_accept_remote_connection(config);
    accept_remote_connection(config);

    // main program loop, handling async IO
    for (;;) {
        try {
            ioservice_.run();
            BOOST_LOG_TRIVIAL(info) << "stopping";
            break;
        }
        catch (...) {
            BOOST_LOG_TRIVIAL(error) << "PlayEngine: Unknown exception occurred.";
        }
    }

    return 0;
}

struct PlayerExec : public std::enable_shared_from_this<PlayerExec>
{
    using args_vector = std::vector<string>;
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
    string  execPath_;
    std::vector<string> execArgs_;
    bp::context ctx_;
    using child_ptr = std::shared_ptr<bp::child>;
    child_ptr child_;
};


int playSopcast(std::string uri) {
    using namespace std::literals;
    // TODO this should be configurable as it platform dependant
    PlayerExec sopcast("sp-sc-auth", { "sop://broker.sopcast.com:3912/151929", "1234", "12345" });// play protv


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

void play(std::string uri) {
    // NOTE should we use cpp-netlib for URI parsing ?
    auto proto_end = uri.find_first_of(':');
    if (proto_end == std::string::npos) {
        return;
    }
    std::string proto = uri.substr(0, proto_end-1);
    if (proto == "sop") {
        playSopcast(uri);
    } else {

    }
}

void play(int pos) {
}

void stop() {
    // TODO
}

void add(const std::string &uri) {
    Player::playlist().add(uri);
}

void enumeratePlaylist(enumPlaylistFn fn) {
    Player::playlist().enumerate(fn);
}

void clearPlaylist() {
    Player::playlist().items_.clear();
    stop();
}

const Player::Status& status() {
    return Player::status();
}
} // namespace PlayEngine

