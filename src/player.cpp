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

#include "player.h"
#include "playlist.h"

#include <thread>
#include <boost/process.hpp>
#include <boost/process/initializers.hpp>
#include <boost/process/mitigate.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/array.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <sys/utsname.h>

namespace bp = boost::process;
namespace io = boost::asio;

extern io::io_service ioservice_;

namespace Player {

Playlist playlist_;
Status status_;

Playlist &playlist() { return playlist_; }

void updateStatus() {
  status_.playlist_ = playlist_.version_;
  status_.playlistLength_ = playlist_.items_.size();
}

const Status &status() {
  updateStatus();
  return status_;
}

struct StdOutNullDevice {
  StdOutNullDevice(bp::pipe &p) : p_(p) {
    io::posix::stream_descriptor pend_(ioservice_, p_.source);
    io::async_read(pend_, io::buffer(iobuffer_), *this);
  }
  void operator()(const boost::system::error_code &ec, std::size_t bytes) {
    if (!ec) {
      // simply eat those verbose output
      io::posix::stream_descriptor pend_(ioservice_, p_.source);
      io::async_read(pend_, io::buffer(iobuffer_), *this);
    } else {
      BOOST_LOG_TRIVIAL(error)
          << "Error signaled in the StdOutNullDevice: " << ec.message();
    }
  }
  bp::pipe &p_;
  boost::array<char, 65536> iobuffer_;
};

struct PlayerExec : public std::enable_shared_from_this<PlayerExec> {
  using args_vector = std::vector<std::string>;
  PlayerExec(std::string execName, args_vector args = args_vector())
      : execArgs_(args) {
    execPath_ = bp::search_path(execName);
    if (execPath_.length() == 0) {
      BOOST_LOG_TRIVIAL(error) << "Cannot find " << execName << "executable!";
      setInvalid();
    } else {
      BOOST_LOG_TRIVIAL(debug) << "found " << execPath_;
      execArgs_.insert(execArgs_.begin(), execPath_);
    }
  }
  void setInvalid() { valid_ = false; }
  virtual void start() {
    child_ = std::make_shared<bp::child>(bp::execute(
        bp::initializers::set_args(execArgs_), bp::initializers::close_stdout(),
        bp::initializers::close_stdin(), bp::initializers::close_stderr()));
    BOOST_LOG_TRIVIAL(debug) << execPath_
                             << " process started. standby for clvlc launch...";
  }
  void start(boost::iostreams::file_descriptor_sink &stdout,
             boost::iostreams::file_descriptor_source &stdin) {
    child_ = std::make_shared<bp::child>(bp::execute(
        bp::initializers::set_args(execArgs_),
        bp::initializers::bind_stdout(stdout),
        bp::initializers::bind_stdin(stdin), bp::initializers::close_stderr()));
  }
  void kill() {
    BOOST_LOG_TRIVIAL(info) << "killing process " << child_->pid << " (was "
                            << execPath_ << ")";
    try {
      bp::terminate(*child_);
      bp::wait_for_exit(*child_);
      // FIXME find out why and how the exception is thrown upon killing one of
      // these processes
    } catch (boost::system::system_error ex) {
      BOOST_LOG_TRIVIAL(warning)
          << "exception caucht when attempting to kill the process :"
          << ex.code() << ": " << ex.what();
    }
  }

 private:
  bool valid_;
  std::string execPath_;
  std::vector<std::string> execArgs_;
  using child_ptr = std::shared_ptr<bp::child>;
  child_ptr child_;
};

using PlayerExecPtr = std::shared_ptr<PlayerExec>;

std::vector<PlayerExecPtr> players_;

struct PlayerVlc : public PlayerExec {
  static PlayerExecPtr create(const char *uri) {
    PlayerExec::args_vector args = {
        // "-I", "rc"  // this is the text console of
        // VLC where we we'll connect to
        //             // sent it commands
    };
    struct utsname sysinfo;
    if (0 == uname(&sysinfo)) {
      BOOST_LOG_TRIVIAL(info) << "prepare to launch VLC on " << sysinfo.machine
                              << " system";
      if (strncmp(sysinfo.machine, "arm", 3) == 0) {
        // we are running on a RaspberryPI board, so let's activate the
        // video hardware acceleration module
        BOOST_LOG_TRIVIAL(info)
            << "detected RaspberryPI, activating omxil_vout";
        // NOTE we're inserting in reverse order as we're inserting at the
        // beginning of the vector
        args.push_back("--vout");
        args.push_back("omxil_vout");
      }
    } else {
      BOOST_LOG_TRIVIAL(warning) << "cannot read system information (uname) "
                                 << errno;
    }
    args.push_back(uri);
    // TODO let the cvlc exec name be configurable
    return std::make_shared<PlayerVlc>(args);
  }

  PlayerVlc(args_vector args)
      : PlayerExec("cvlc", args),
        stdout_pipe_(bp::create_pipe()),
        stdout_sink_(stdout_pipe_.sink, boost::iostreams::close_handle),
        stdout_(ioservice_, stdout_pipe_.source),
        // stdout_source_(stdout_pipe_.source, boost::iostreams::close_handle),
        stdin_pipe_(bp::create_pipe()),
        stdin_source_(stdin_pipe_.source, boost::iostreams::close_handle),
        // stdin_sink_(stdin_pipe_.sink, boost::iostreams::close_handle),
        stdin_(ioservice_, stdin_pipe_.sink) {}

  void start() override {
    PlayerExec::start(stdout_sink_, stdin_source_);
    // io::async_read(
    //     stdout_, io::buffer(stdout_buffer_),
    //     [this](const boost::system::error_code &ec,
    //            std::size_t len) { this->stdout_data_received(ec, len); });
  }

  enum class Command {
    volup,
    voldown,
    volume,
    play,
    pause,
    get_length,
    get_title,
    shutdown
  };

  void send_command(Command cmd) {}

 private:
  void stdout_data_received(const boost::system::error_code &ec,
                            std::size_t len) {
    io::async_read(
        stdout_, io::buffer(stdout_buffer_),
        [this](const boost::system::error_code &ec,
               std::size_t len) { this->stdout_data_received(ec, len); });
  }

  bp::pipe stdout_pipe_;
  boost::iostreams::file_descriptor_sink stdout_sink_;
  // boost::iostreams::file_descriptor_sink stdout_source_;
  bp::pipe_end stdout_;
  boost::array<char, 16384> stdout_buffer_;

  bp::pipe stdin_pipe_;
  bp::pipe_end stdin_;
  // boost::iostreams::file_descriptor_source stdin_sink_;
  boost::iostreams::file_descriptor_source stdin_source_;
  boost::array<char, 16384> stdin_buffer_;
};

bool playSopcast(std::string uri) {
  using namespace std::literals;
  // TODO this should be configurable as it platform dependant
  // protv uri : "sop://broker.sopcast.com:3912/151929"
  PlayerExec::args_vector args = {uri, "1234", "12345"};
  PlayerExecPtr sopcast = std::make_shared<PlayerExec>("sp-sc-auth", args);
  players_.push_back(sopcast);

  auto vlc = PlayerVlc::create("http://localhost:12345/tv.asf");
  players_.push_back(vlc);

  sopcast->start();
  std::this_thread::sleep_for(5s);
  vlc->start();

  return true;
}

void urldecode2(char *dst, const char *src) {
  char a, b;
  while (*src) {
    if ((*src == '%') && ((a = src[1]) && (b = src[2])) &&
        (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a') a -= 'a' - 'A';
      if (a >= 'A')
        a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a') b -= 'a' - 'A';
      if (b >= 'A')
        b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    } else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

bool playFile(std::string uri) {
  auto file_uri = uri.substr(std::string("file://").size());
  BOOST_LOG_TRIVIAL(debug) << "file_uri " << file_uri;
  char *decoded = (char *)malloc(file_uri.size());
  if (nullptr == decoded) {
    BOOST_LOG_TRIVIAL(error) << "Out of memory!";
    return false;
  }
  urldecode2(decoded, file_uri.c_str());
  BOOST_LOG_TRIVIAL(debug) << "file_uri decoded '" << decoded << "'";

  auto vlc = PlayerVlc::create(decoded);
  players_.push_back(vlc);
  vlc->start();

  free(decoded);
  return true;
}

bool play(std::string uri) {
  // NOTE should we use cpp-netlib for URI parsing ?
  BOOST_LOG_TRIVIAL(info) << "playing uri " << uri;
  auto proto_end = uri.find(':');
  if (proto_end == std::string::npos) {
    BOOST_LOG_TRIVIAL(warning) << "uri protocol not recognized. skipping.";
    return false;
  }
  std::string proto = uri.substr(0, proto_end);
  if (proto == "sop") {
    return playSopcast(uri);
  } else if (proto == "file") {
    return playFile(uri);
  } else {
    BOOST_LOG_TRIVIAL(warning) << "uri protocol not recognized. skipping.";
    return false;
  }
  return true;
}

PlaylistItem currentPlaylistItem;

void play() {
  currentPlaylistItem = playlist_.current();
  if (currentPlaylistItem.empty()) {
    BOOST_LOG_TRIVIAL(debug)
        << "Playlist is currently empty, or we reached its end.";
    return;
  }

  for (;;) {
    if (play(currentPlaylistItem.uri_)) break;
    // no need to update status now. it'll get updated when client will ask for
    // it
    // if the item could not be played, then automatically continue with next
    // item
    BOOST_LOG_TRIVIAL(debug) << "Trying next item in the playlist";

    currentPlaylistItem = playlist_.next();
    if (currentPlaylistItem.empty()) {
      BOOST_LOG_TRIVIAL(debug)
          << "Playlist is currently empty, or we reached its end.";
      return;
    }
  }
}

void next() {
  stop();
  currentPlaylistItem = playlist_.next();
  play();
}

void prev() {
  stop();
  currentPlaylistItem = playlist_.prev();
  play();
}

void stop() {
  for (PlayerExecPtr player : players_) {
    player->kill();
  }
  players_.clear();
}

}  // namespace
