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

#define REQUIRE_OK(info) BOOST_REQUIRE(info == "OK");
#define REQUIRE_OK_STATUS(socket) REQUIRE_OK(recv_status(socket));

BOOST_AUTO_TEST_CASE( playlist_cmd ) {
    auto socket = connect_to_server(io_service_, epi_);

    // TODO store the playlist version here and check later that it gets increased by successive modifications
    send_cmd(socket, clearcmd);
    BOOST_REQUIRE_EQUAL(recv_status(socket), "OK");

    auto uri_protv = "sop://broker.sopcast.com:3912/149252";
    std::string cmdadd = std::string("add ").append(uri_protv).append("\r\n"); // this is protv channel from romania
    send_cmd(socket, cmdadd);
    REQUIRE_OK_STATUS(socket);

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

    send_cmd(socket, playlistinfocmd);
    std::string info = recv_status(socket);
    REQUIRE_ATTR(info, attr_file);
    REQUIRE_ATTR_VALUE(info, attr_file, uri_protv);
    REQUIRE_ATTR(recv_status(socket), attr_pos);
    REQUIRE_OK_STATUS(socket);

    // info = recv_status(socket);
    // BOOST_REQUIRE(info == "Id: 0");

    info = vpd_status_line(socket, "playlistlength");
    REQUIRE_ATTR(info, attr_playlistlength);
    REQUIRE_ATTR_VALUE(info, attr_playlistlength, "1");

    send_cmd(socket, clearcmd);
    REQUIRE_OK_STATUS(socket);
}

BOOST_AUTO_TEST_CASE( playlist_persist ) {
    auto socket = connect_to_server(io_service_, epi_);

    send_cmd(socket, clearcmd);
    REQUIRE_OK_STATUS(socket);

    auto uri_protv = "sop://broker.sopcast.com:3912/149252";
    std::string cmdadd = std::string("add ").append(uri_protv).append("\r\n"); // this is protv channel from romania
    send_cmd(socket, cmdadd);
    REQUIRE_OK_STATUS(socket);

    const char* savecmd = "save test_list\r\n";
    send_cmd(socket, savecmd);
    REQUIRE_OK_STATUS(socket);

    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // vpd creates the file asynchronously, so wait a bit here
    fs::path plpath = (workPath() /= "playlists") /= "test_list.m3u";
    BOOST_REQUIRE_MESSAGE(fs::exists(plpath), plpath.string() + " not found!");
    std::ifstream ifs(plpath.string());
    std::string line;
    getline(ifs, line);
    BOOST_REQUIRE(line == uri_protv);

    // now clear the playlist, the reload it and check it again
    send_cmd(socket, clearcmd);
    REQUIRE_OK_STATUS(socket);

    send_cmd(socket, listplaylistscmd);
    auto info_line = recv_status(socket);
    BOOST_REQUIRE(info_line.substr(0, 9) == "playlist:");
    BOOST_REQUIRE(info_line.substr(10) == "test_list");
    info_line = recv_status(socket);
    const char* attr_last_modified = "Last-Modified:";
    BOOST_REQUIRE(info_line.substr(0, strlen(attr_last_modified)) == attr_last_modified);
    REQUIRE_OK_STATUS(socket);

    send_cmd(socket, "load test_list\r\n");
    REQUIRE_OK_STATUS(socket);

    {
        send_cmd(socket, playlistinfocmd);
        std::string info = recv_status(socket);
        REQUIRE_ATTR(info, attr_file);
        REQUIRE_ATTR_VALUE(info, attr_file, uri_protv);
        info = recv_status(socket);
        REQUIRE_ATTR(info, attr_pos);
        REQUIRE_OK_STATUS(socket);
    }
}

BOOST_AUTO_TEST_SUITE_END()
