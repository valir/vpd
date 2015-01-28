/*
 * This file is part of the Video Player Daemon
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
#define BOOST_TEST_MODULE ClientConnection

#include "asio_setup.h"


BOOST_FIXTURE_TEST_SUITE(s, AsioSetup)

BOOST_AUTO_TEST_CASE( server_accept_connection ) {
    BOOST_TEST_MESSAGE("Testing server_accept_connection");
    socket_ptr socket = connect_to_server(io_service_, epi_);
}

#define VPDN(n) #n
#define VPDNN(n) VPDN(n)
#define VPD_WELCOME_LINE "VPD " VPDNN(VPD_MAJOR_VERSION) "." VPDNN(VPD_MINOR_VERSION) " ready\r"

BOOST_AUTO_TEST_CASE( server_responds_hello ) {
    BOOST_TEST_MESSAGE("Testing servers responds hello");
    socket_ptr socket = connect_to_server(io_service_, epi_, false);
    boost::system::error_code ec;
    io::streambuf data;
    auto bytes = io::read_until(*socket, data, "\r\n", ec);
    BOOST_MESSAGE("socket connect error_code: " << ec.value() << ", message: " << ec.message());
    BOOST_REQUIRE(!ec);
    std::istream datais(&data);
    char response[bytes+1];
    std::memset(response, 0, bytes+1);
    datais.getline(response, bytes);
    BOOST_MESSAGE("server greeting is: " << static_cast<char*>(response));
    BOOST_MESSAGE("expected greeting is: " VPD_WELCOME_LINE);
    BOOST_REQUIRE(strncmp(response, VPD_WELCOME_LINE, bytes) == 0);
}

BOOST_AUTO_TEST_CASE( server_closes_connection_on_close ) {
    BOOST_TEST_MESSAGE("Testing server is closing connection");
    socket_ptr socket = connect_to_server(io_service_, epi_);
    boost::system::error_code ec;
    const char closeCmd[] = "close\r\n";
    auto bytes = io::write(*socket, io::buffer(closeCmd), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(bytes == sizeof(closeCmd));
    boost::array<char, 256> data;
    ec.clear();
    bytes = socket->read_some(io::buffer(data), ec);
    BOOST_REQUIRE(ec);
}

BOOST_AUTO_TEST_SUITE_END()
