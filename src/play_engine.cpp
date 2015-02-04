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
#include <list>
#include <fstream>
#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace PlayEngine {

    namespace io = boost::asio;
    namespace fs = boost::filesystem;
    using socket_t = io::ip::tcp::socket;

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


    void play(int pos) {
        Player::play();
    }

    void stop() {
        Player::stop();
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

    fs::path workdir() {
        boost::system::error_code ec;
        if (!fs::exists(RuntimeConfig::workdir_, ec)) {
            BOOST_LOG_TRIVIAL(debug) << "workdir not found (" << ec.message() << ")";
            fs::create_directories(RuntimeConfig::workdir_);
            // let it throw an crash the program, as that's not an acceptable situation
            // not to be able to create the working directory
        }
        return fs::path(RuntimeConfig::workdir_);
    }

    fs::path playlistsPath() {
        boost::system::error_code ec;
        auto p = workdir();
        p /= "playlists";
        if (!fs::exists(p)) {
            BOOST_LOG_TRIVIAL(debug) << "creating directory " << p;
            fs::create_directories(p);
            // here also, let it crash!
        }
        return p;
    }

    void save(const std::string& filename){
        // NOTE has already been checked by the save command
        auto p = playlistsPath() /= filename;
        p += ".m3u";
        if (fs::exists(p)) {
            BOOST_LOG_TRIVIAL(warning) << "replacing playlist " << p;
        }

        ofstream ofs(p.string(), std::ofstream::out | std::ofstream::trunc);
        Player::playlist().enumerate(
            [&](const PlaylistItem &item) {
                if (!item.name_.empty()) {
                    ofs << "# " << item.name_ << endl;
                }
                ofs << item.uri_;
            });
    }
} // namespace PlayEngine

