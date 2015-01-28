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

#include "config.h"

#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <future>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <boost/test/included/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace io = boost::asio;
namespace bp = boost::process;
using namespace std;
using socket_t = io::ip::tcp::socket;
using socket_ptr = std::shared_ptr< socket_t >;

int getProcIdByName(string procName)
{
    int pid = -1;

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL)
    {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp)))
        {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
            if (id > 0)
            {
                // Read contents of virtual /proc/{pid}/cmdline file
                string cmdPath = string("/proc/") + dirp->d_name + "/cmdline";
                ifstream cmdFile(cmdPath.c_str());
                string cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
            }
        }
    }

    closedir(dp);

    return pid;
}

struct AsioSetup {
    AsioSetup() {
        BOOST_TEST_MESSAGE("setup test fixture");
        io::ip::tcp::resolver resolver(io_service_);
        epi_ = resolver.resolve({"127.0.0.1", "7700"});

        int existing_vpd_id = getProcIdByName("vpd");
        if (-1 == existing_vpd_id) {
            std::thread vpd = std::thread(
                    [this]() {
                        BOOST_TEST_MESSAGE("launching VPD...");
                        auto execPath = "../vpd";
                        // auto execArgs = {
                        // };
                        //vlc.ctx_.process_name = "cvlc";

                        bp::child vpd = bp::create_child(execPath);
                        vpd_pid_ = vpd.get_id();
                        bp::status vpdstatus(io_service_);
                        vpdstatus.async_wait(vpd.get_id(),
                            [](const boost::system::error_code &ec, int exit_code){
                               BOOST_TEST_MESSAGE("VPD exit_code:" << exit_code);
                            });
                        BOOST_TEST_MESSAGE("VPD thread exit");
                    });
            // wait for the vpd process start
            // FIXME we should wait more reliably for VPD start, as maybe in the future
            // it'll take longer to initialize
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            vpd.detach();
        } else {
            BOOST_TEST_MESSAGE("using existing VPD pid: " << existing_vpd_id);
        }
        // asio_thread_ = std::thread(
        //         [this]() {
        //             BOOST_TEST_MESSAGE("Starting asio thread");
        //             io::io_service::work work(io_service_);
        //             BOOST_TEST_MESSAGE("asio thread stop");
        //         });
    }
    ~AsioSetup() {
        BOOST_TEST_MESSAGE("teardown test fixture");
        io_service_.stop();
        // FIXME figure out how to terminate the child using bp::process:terminate
        if (vpd_pid_ >0)
            kill(vpd_pid_, SIGKILL);
    }

    io::io_service io_service_;
    io::ip::tcp::resolver::iterator epi_;
    int vpd_pid_;
    std::thread asio_thread_;
};

BOOST_FIXTURE_TEST_SUITE(s, AsioSetup)

socket_ptr connect_to_server(io::io_service& io_service, io::ip::tcp::resolver::iterator& epi, bool eatHello =true) {
    auto socket = std::make_shared< socket_t >(io_service);
    boost::system::error_code ec;
    socket->connect(*epi, ec);
    BOOST_MESSAGE("socket connect error_code: " << ec.value() << ", message: " << ec.message());
    BOOST_CHECK(!ec);
    if (eatHello) {
        io::streambuf data;
        auto bytes = io::read_until(*socket, data, "\r\n", ec);
        BOOST_MESSAGE("socket connect error_code: " << ec.value() << ", message: " << ec.message());
        BOOST_REQUIRE(!ec);
    }
    return socket;
}

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
