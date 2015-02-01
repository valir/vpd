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

BOOST_AUTO_TEST_CASE( play_cmd ) {
    auto socket = connect_to_server(io_service_, epi_);
    auto uri_protv = "sop://broker.sopcast.com:3912/149252";
    std::string cmdadd = std::string("add ").append(uri_protv).append("\r\n"); // this protv channel from romania
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

    // TODO clear the playlist
}

BOOST_AUTO_TEST_SUITE_END()
