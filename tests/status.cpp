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

void check_prop(socket_ptr socket, const char *propname) {
    std::string info = recv_status(socket);
    std::string prop = propname;
    BOOST_REQUIRE_MESSAGE(info.compare(0, prop.length(), prop) == 0, "cannot find '" << prop << "' in vpd status output");
}

BOOST_AUTO_TEST_CASE( status_cmd ) {
    auto socket = connect_to_server(io_service_, epi_);
    const char* cmdstatus = "status\r\n";
    send_cmd(socket, cmdstatus);
    check_prop(socket, "volume");
    check_prop(socket, "repeat");
    check_prop(socket, "random");
    check_prop(socket, "single");
    check_prop(socket, "consume");
    check_prop(socket, "playlist");
    check_prop(socket, "playlistlength");
    check_prop(socket, "videoclip");
    check_prop(socket, "videoclipid");
    check_prop(socket, "time");
    check_prop(socket, "elapsed");
    check_prop(socket, "nextvideoclip");
    check_prop(socket, "nextvideoid");
    std::string info = recv_status(socket);
    BOOST_REQUIRE_EQUAL(info, "OK");
}

BOOST_AUTO_TEST_SUITE_END()
