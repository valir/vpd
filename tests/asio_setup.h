/*
 * This file is part of the Video Player Daemon
 *
 * Copyright (C) 2015 Valentin Rusu vpd@rusu.info
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

#ifndef ASIO_SETUP_H
#define ASIO_SETUP_H

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
#include <boost/asio.hpp>
#include <boost/date_time/time_duration.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/process.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "config.h"

namespace io = boost::asio;
namespace bp = boost::process;
namespace fs = boost::filesystem;
namespace t = boost::posix_time;
using namespace std;
using socket_t = io::ip::tcp::socket;
using socket_ptr = std::shared_ptr<socket_t>;

int getProcIdByName(string procName) {
  int pid = -1;

  // Open the /proc directory
  DIR* dp = opendir("/proc");
  if (dp != NULL) {
    // Enumerate all entries in directory until process found
    struct dirent* dirp;
    while (pid < 0 && (dirp = readdir(dp))) {
      // Skip non-numeric entries
      int id = atoi(dirp->d_name);
      if (id > 0) {
        // Read contents of virtual /proc/{pid}/cmdline file
        string cmdPath = string("/proc/") + dirp->d_name + "/cmdline";
        ifstream cmdFile(cmdPath.c_str());
        string cmdLine;
        getline(cmdFile, cmdLine);
        if (!cmdLine.empty()) {
          // Keep first cmdline item which contains the program path
          size_t pos = cmdLine.find('\0');
          if (pos != string::npos) cmdLine = cmdLine.substr(0, pos);
          // Keep program name only, removing the path
          pos = cmdLine.rfind('/');
          if (pos != string::npos) cmdLine = cmdLine.substr(pos + 1);
          // Compare against requested process name
          if (procName == cmdLine) pid = id;
        }
      }
    }
  }

  closedir(dp);

  return pid;
}

struct AsioSetup {
  AsioSetup() : own_vpd_(false) {
    BOOST_TEST_MESSAGE("setup test fixture");
    io::ip::tcp::resolver resolver(io_service_);
    epi_ = resolver.resolve({"127.0.0.1", "7700"});

    int existing_vpd_id = getProcIdByName("vpd");
    if (-1 == existing_vpd_id) {
      BOOST_TEST_MESSAGE("launching VPD...");
      // FIXME use a cmake variable or something else to avoir hardcoding this
      auto execPath = "../src/vpd";
      auto configPath = createTestConfigFile();
      vector<string> args = {execPath, "--config", configPath};
      vpd_ = std::make_shared<bp::child>(bp::execute(bp::initializers::set_args(args)));
      vpd_pid_ = vpd_->pid;
      BOOST_TEST_MESSAGE("VPD thread exit");
      // wait for the vpd process start
      // FIXME we should wait more reliably for VPD start, as maybe in the
      // future
      // it'll take longer to initialize
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      own_vpd_ = true;
    } else {
      BOOST_TEST_MESSAGE("using existing VPD pid: " << existing_vpd_id);
      own_vpd_ = false;
    }
  }

  ~AsioSetup() {
    BOOST_TEST_MESSAGE("teardown test fixture");
    if (own_vpd_) {
      bp::terminate(*vpd_);
      auto exit_code = bp::wait_for_exit(*vpd_);
      BOOST_TEST_MESSAGE("VPD exit_code:" << exit_code);
      if (fs::exists(workPath())) {
        fs::remove_all(workPath());
      }
    }
  }

  fs::path workPath() const { return fs::current_path() /= "work"; }
  string createTestConfigFile() {
    fs::path p(fs::current_path());
    p /= "vpd_test_rc";
    ofstream ofs(p.string(), ofstream::out | ofstream::trunc);

    auto workp = workPath();
    ofs << "workdir = ";
    ofs.write(workp.string().c_str(), workp.string().size());
    return p.string();
  }

  void send_cmd(socket_ptr socket, std::string cmd) {
    BOOST_REQUIRE(cmd.substr(cmd.length() - 2) == "\r\n");
    boost::system::error_code ec;
    BOOST_MESSAGE(">> SENDING " << cmd);
    auto bytes = io::write(*socket, io::buffer(cmd), ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(bytes == cmd.size());
  }

  std::string recv_status(socket_ptr socket) {
    io::deadline_timer timeout(io_service_);
    timeout.expires_from_now(t::seconds(10));
    timeout.async_wait([&](boost::system::error_code ec) {
      if (!ec) {
        BOOST_MESSAGE("deadline expired!");
        socket->close(ec);
      }
    });
    boost::system::error_code ec = boost::asio::error::would_block;
    size_t bytes = 0;
    io::async_read_until(
        *socket, data_, "\r\n",
        [&](boost::system::error_code compl_ec, size_t compl_bytes) {
          ec = compl_ec;
          bytes = compl_bytes;
        });
    do
      io_service_.run_one();
    while (ec == boost::asio::error::would_block);
    BOOST_REQUIRE_MESSAGE(!ec, ec.message());
    BOOST_REQUIRE(bytes > 0);
    std::istream is(&data_);
    std::string result;
    std::getline(is, result);
    BOOST_REQUIRE(result.substr(result.length() - 1) == "\r");
    auto ret = result.substr(0, result.length() - 1);
    BOOST_MESSAGE("<< RECEIVED " << ret);
    return ret;
  }

  std::string vpd_status_line(socket_ptr socket, const char* prop) {
    const char* cmdstatus = "status\r\n";
    send_cmd(socket, cmdstatus);
    std::string result;
    for (;;) {
      std::string si = recv_status(socket);
      if (si == "OK") break;
      if (si.substr(0, strlen(prop)) == prop) {
        result = si;
        // do not break, but continue reading until OK is received
      }
    }
    BOOST_REQUIRE_MESSAGE(!result.empty(),
                          "cannot find the \""
                              << prop << ":\" line in the vpd status output");
    return result;
  }
  socket_ptr connect_to_server(io::io_service& io_service,
                               io::ip::tcp::resolver::iterator& epi,
                               bool eatHello = true) {
    auto socket = std::make_shared<socket_t>(io_service);
    boost::system::error_code ec;
    socket->connect(*epi, ec);
    BOOST_MESSAGE("socket connect error_code: " << ec.value() << ", message: "
                                                << ec.message());
    BOOST_CHECK(!ec);
    if (eatHello) {
      io::streambuf data;
      auto bytes = io::read_until(*socket, data, "\r\n", ec);
      BOOST_MESSAGE("socket connect error_code: " << ec.value() << ", message: "
                                                  << ec.message());
      BOOST_REQUIRE(!ec);
    }
    return socket;
  }
  bool own_vpd_;
  io::io_service io_service_;
  io::ip::tcp::resolver::iterator epi_;
  int vpd_pid_;
  std::shared_ptr<bp::child> vpd_;
  io::streambuf data_;
};

const char* clearcmd = "clear\r\n";
const char* playlistinfocmd = "playlistinfo\r\n";
const char* listplaylistscmd = "listplaylists\r\n";

const char* attr_playlistlength = "playlistlength";
const char* attr_file = "file";
const char* attr_pos = "Pos";

#define REQUIRE_ATTR(info, attr)                                  \
  {                                                               \
    std::string attribute(attr);                                  \
    attribute += ':';                                             \
    BOOST_REQUIRE(info.substr(0, attribute.size()) == attribute); \
  }

#define REQUIRE_ATTR_VALUE(info, attr, required_value)                  \
  {                                                                     \
    std::string attribute(attr);                                        \
    attribute += ':';                                                   \
    BOOST_REQUIRE(info.substr(attribute.size() + 1) == required_value); \
  }

#endif  // ASIO_SETUP_H
