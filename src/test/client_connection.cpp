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
#include "client_connection.h"

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
#include <boost/test/included/unit_test.hpp>
#include <boost/asio.hpp>
#include <boost/process.hpp>

namespace io = boost::asio;
namespace bp = boost::process;
using namespace std;

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

BOOST_AUTO_TEST_CASE( server_accept_connection ) {
    BOOST_TEST_MESSAGE("Testing server_accept_connection");
    io::ip::tcp::socket socket(io_service_);
    boost::system::error_code ec;
    socket.connect(*epi_, ec);
    BOOST_MESSAGE("socket connect error_code: " << ec.value() << ", message: " << ec.message());
    BOOST_CHECK(!ec);
    socket.close();
}

BOOST_AUTO_TEST_SUITE_END()
