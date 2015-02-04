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

#define BOOST_TEST_MODULE Playlist

#include "asio_setup.h"


BOOST_FIXTURE_TEST_SUITE(s, AsioSetup)

BOOST_AUTO_TEST_CASE( playlist_cmd ) {
    auto socket = connect_to_server(io_service_, epi_);

    // TODO store the playlist version here and check later that it gets increased by successive modifications
    const char* clearcmd = "clear\r\n";
    send_cmd(socket, clearcmd);
    BOOST_REQUIRE_EQUAL(recv_status(socket), "OK");

    auto uri_protv = "sop://broker.sopcast.com:3912/149252";
    std::string cmdadd = std::string("add ").append(uri_protv).append("\r\n"); // this is protv channel from romania
    send_cmd(socket, cmdadd);
    std::string addreply = recv_status(socket);
    BOOST_REQUIRE(addreply == "OK");

    { // test the case when no params are given
        std::string cmdadd_noparams = "add\r\n";
        send_cmd(socket, cmdadd_noparams);
        std::string reply = recv_status(socket);
        BOOST_REQUIRE(reply.substr(0, 3) == "ACK");
        // TODO I think we also should test that the player sends "missing param" here
    }
    { // test the case when too many params are given
        std::string cmdadd_noparams = "add param1 param2\r\n";
        send_cmd(socket, cmdadd_noparams);
        std::string reply = recv_status(socket);
        BOOST_REQUIRE(reply.substr(0, 3) == "ACK");
        // TODO I think we also should test that the player sends "too many params" here
    }
    { // test the case when an invalid URI is given
        std::string cmdadd_noparams = "add paramInvalidURI\r\n";
        send_cmd(socket, cmdadd_noparams);
        std::string reply = recv_status(socket);
        BOOST_REQUIRE(reply.substr(0, 3) == "ACK");
    }

    auto cmdplaylistinfo = "playlistinfo\r\n";
    send_cmd(socket, cmdplaylistinfo);

    std::string info = recv_status(socket);
    BOOST_REQUIRE(info.substr(0, 5) == "file:");
    BOOST_REQUIRE(info.substr(6) == uri_protv);

    info = recv_status(socket);
    BOOST_REQUIRE(info == "Pos: 0");

    // info = recv_status(socket);
    // BOOST_REQUIRE(info == "Id: 0");

    info = recv_status(socket);
    BOOST_REQUIRE(info == "OK");

    BOOST_CHECK_EQUAL(vpd_status_line(socket, "playlistlength"), "playlistlength: 1");

    send_cmd(socket, clearcmd);
    BOOST_REQUIRE_EQUAL(recv_status(socket), "OK");
}

BOOST_AUTO_TEST_CASE( playlist_persist ) {
    auto socket = connect_to_server(io_service_, epi_);

    const char* clearcmd = "clear\r\n";
    send_cmd(socket, clearcmd);
    BOOST_REQUIRE_EQUAL(recv_status(socket), "OK");

    auto uri_protv = "sop://broker.sopcast.com:3912/149252";
    std::string cmdadd = std::string("add ").append(uri_protv).append("\r\n"); // this is protv channel from romania
    send_cmd(socket, cmdadd);
    std::string addreply = recv_status(socket);
    BOOST_REQUIRE(addreply == "OK");

    const char* savecmd = "save test_list\r\n";
    send_cmd(socket, savecmd);
    std::string savereply = recv_status(socket);
    BOOST_REQUIRE(savereply == "OK");

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // vpd creates the file asynchronously, so wait a bit here
    fs::path plpath = (workPath() /= "playlists") /= "test_list.m3u";
    BOOST_REQUIRE_MESSAGE(fs::exists(plpath), plpath.string() + " not found!");
    std::ifstream ifs(plpath.string());
    std::string line;
    getline(ifs, line);
    BOOST_REQUIRE(line == uri_protv);
}

BOOST_AUTO_TEST_SUITE_END()
