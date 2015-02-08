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

#define BOOST_TEST_MODULE Status

#include "asio_setup.h"

BOOST_FIXTURE_TEST_SUITE(s, AsioSetup)

BOOST_AUTO_TEST_CASE( status_cmd ) {
    auto socket = connect_to_server(io_service_, epi_);
    const char* cmdstatus = "status\r\n";
    send_cmd(socket, cmdstatus);
    REQUIRE_ATTR(recv_status(socket), "volume");
    REQUIRE_ATTR(recv_status(socket), "repeat");
    REQUIRE_ATTR(recv_status(socket), "random");
    REQUIRE_ATTR(recv_status(socket), "single");
    REQUIRE_ATTR(recv_status(socket), "consume");
    REQUIRE_ATTR(recv_status(socket), "playlist");
    REQUIRE_ATTR(recv_status(socket), "playlistlength");
    REQUIRE_ATTR(recv_status(socket), "videoclip");
    REQUIRE_ATTR(recv_status(socket), "videoclipid");
    REQUIRE_ATTR(recv_status(socket), "time");
    REQUIRE_ATTR(recv_status(socket), "elapsed");
    REQUIRE_ATTR(recv_status(socket), "nextvideoclip");
    REQUIRE_ATTR(recv_status(socket), "nextvideoid");
    std::string info = recv_status(socket);
    BOOST_REQUIRE_EQUAL(info, "OK");
}

BOOST_AUTO_TEST_SUITE_END()
