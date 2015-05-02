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
#include "client_engine.h"
#include "config.h"
#include "play_engine.h"

#include <boost/log/trivial.hpp>
#include <boost/filesystem/path.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/regex.hpp>

namespace ClientEngine {

int ClientSession::nextSessionNumber = 0;

const char* errorMessages_[static_cast<std::size_t>(Error::LastError)] = {
  "No error",
  "unknown command",
  "command not implemented",
  "too many arguments",
  "missing parameter",
  "the given URI is invalid",
  "the given file name is invalid"};

std::string AckStatus::toString() const {
  std::stringbuf sb;
  std::ostream os(&sb);
  if (error_ == Error::NoError) {
    if (!results_.empty()) {
      os << results_;
      // FIXME I don't like putting this as it's CPU intensive
      // if (results_.substr(results_.length() -2) != "\r\n")
      //     os << "\r\n";
    }
    os << "OK\r\n";
  } else {
    os << "ACK [" << static_cast<std::size_t>(error_) << "@" << cmdNumber_
      << "] {" << currentCmd_ << "} "
      << explainHandler_(errorMessages_[static_cast<std::size_t>(error_)])
      << "\r\n";
  }
  return sb.str();
}

void ClientMessage::setResponse(AckStatusPtr ackStatus) {
  setResponse(ackStatus->toString());
}

struct ClientCommand : public std::enable_shared_from_this<ClientCommand> {
  using CommandParamsPtr = std::shared_ptr<CommandParams>;
  using ClientCommandPtr = std::shared_ptr<ClientCommand>;
  using CommandFunc_t =
    std::function<AckStatusPtr(ClientCommandPtr, ClientSessionPtr)>;
  ClientCommand(ClientMessagePtr& clientMsg, CommandFunc_t action)
    : clientMsg_(clientMsg), command_(action) {}
  AckStatusPtr execute(ClientSessionPtr session) {
    return command_(shared_from_this(), session);
  }
  void extractParams(const std::string& opcode, std::istream& is) {
    opcode_ = opcode;
    std::string param;
    while (is >> param) {
      params_.push_back(param);
    }
  }
  const CommandParams& params() const noexcept { return params_; }
  const std::string& opcode() const noexcept { return opcode_; }

  private:
  ClientMessagePtr& clientMsg_;
  CommandFunc_t command_;
  std::string opcode_;
  CommandParams params_;
};

using ClientCommandPtr = std::shared_ptr<ClientCommand>;

#define START_CMD(cmdname) auto cmdname##_factory = [](ClientMessagePtr msg) {\
  return std::make_shared<ClientCommand>(msg,\
      [](ClientCommandPtr cmd, ClientSessionPtr session) {
#define START_CMD_NOARGS(cmdname)                                   \
      START_CMD(cmdname)                                                \
      if (cmd->params().size() > 0) {                                   \
      return AckStatus::fromError(cmd->opcode(), Error::TooManyArgs); \
      }

#define START_CMD_ARGS1(cmdname)                                         \
      START_CMD(cmdname)                                                     \
      if (cmd->params().size() == 0) {                                       \
      return AckStatus::fromError(cmd->opcode(), Error::MissingParameter); \
      }                                                                      \
      if (cmd->params().size() > 1) {                                        \
      return AckStatus::fromError(cmd->opcode(), Error::TooManyArgs);      \
      }

#define END_CMD()        \
      } ); \
}                      \
;

#define REGISTER_CMD(name) knownFactories.emplace(#name, name##_factory);

#define RETURN_OK() return AckStatus::ok();

template <typename T>
void writeProp(std::ostream& os, const char* propName, T const& val) {
  os << propName << ": " << val << "\r\n";
}

template <>
void writeProp<bool>(std::ostream& os, const char* propName, bool const& val) {
  os << propName << ": " << (val ? "1" : "0") << "\r\n";
}

template <>
void writeProp<boost::posix_time::time_duration>(
    std::ostream& os, const char* propName,
    boost::posix_time::time_duration const& val) {
  os << propName << ": " << val.hours() << ":" << val.seconds() << "\r\n";
}

START_CMD_NOARGS(close)
  session->closeSession();
  return AckStatus::fromError(cmd->opcode(), Error::LastError);
  END_CMD()
START_CMD_NOARGS(status)
  std::stringbuf buf;
  std::ostream os(&buf);
  auto si = session -> status();
  writeProp(os, "volume", si.volume_);
  writeProp(os, "repeat", si.repeat_);
  writeProp(os, "random", si.random_);
  writeProp(os, "single", si.single_);
  writeProp(os, "consume", si.consume_);
  writeProp(os, "playlist", si.playlist_);
  writeProp(os, "playlistlength", si.playlistLength_);
  writeProp(os, "videoclip", si.videoClip_);
  writeProp(os, "videoclipid", si.videoClipId_);
  writeProp(os, "time", si.time_);
  writeProp(os, "elapsed", si.elapsed_);
  writeProp(os, "nextvideoclip", si.nextVideo_);
  writeProp(os, "nextvideoid", si.nextVideoId_);
  return AckStatus::ok(buf.str());
  END_CMD()
START_CMD_NOARGS(play)
  int pos = 0;
  // if (cmd->params().size() >0) {
  //     pos = std::stoi(cmd->params()[0]);
  // }
  session->play(pos);
  RETURN_OK()
  END_CMD()
START_CMD_NOARGS(stop)
  session->stop();
  RETURN_OK()
  END_CMD()
START_CMD_NOARGS(clear)
  session->clear();
  RETURN_OK()
END_CMD()

START_CMD_NOARGS(next)
  session->next();
  RETURN_OK()
END_CMD()

START_CMD_NOARGS(previous)
  session->prev();
  RETURN_OK()
END_CMD()

START_CMD_ARGS1(add)
  std::string uri = cmd->params()[0];
  if (!session->isValidUri(uri)) {
    return AckStatus::fromError(cmd->opcode(), Error::InvalidUri);
  }
session->add(uri);
  RETURN_OK()
  END_CMD()
START_CMD(playlistinfo)
  if (cmd->params().size() > 0) {
    // TODO handle the parameter VIDEOPOS and START:END
    return AckStatus::fromError(cmd->opcode(), Error::TooManyArgs);
  }
std::stringbuf obuf;
std::ostream os(&obuf);
int pos = 0;
session->enumeratePlaylist([&](const PlaylistItem& item) {
    writeProp(os, "file", item.uri_);
    if (!item.name_.empty()) {
    writeProp(os, "name", item.name_);
    }
    writeProp(os, "Pos", pos++);
    if (item.id_ >= 0) {
    writeProp(os, "Id", item.id_);
    }
    });
auto results = obuf.str();
return AckStatus::ok(results);
END_CMD()

START_CMD_ARGS1(save)
  std::string listname = cmd->params()[0];
  if (!boost::filesystem::portable_name(listname)) {
    BOOST_LOG_TRIVIAL(debug) << "invalid name give for playlist " << listname;
    return AckStatus::fromError(cmd->opcode(), Error::InvalidFilename);
  }
session->save(listname);
  RETURN_OK()
END_CMD()

START_CMD_NOARGS(listplaylists)
  std::stringbuf obuf;
  std::ostream os(&obuf);
  int pos = 0;
  session->enumeratePlaylists([&](const PlaylistInfo& info) {
      writeProp(os, "playlist", info.name());
      writeProp(os, "Last-Modified", info.lastModified());
      });
auto results = obuf.str();
return AckStatus::ok(results);
END_CMD()

START_CMD_ARGS1(load)
  std::string listname = cmd->params()[0];
  if (!boost::filesystem::portable_name(listname)) {
    BOOST_LOG_TRIVIAL(debug) << "invalid name give for playlist " << listname;
    return AckStatus::fromError(cmd->opcode(), Error::InvalidFilename);
  }
session->load(listname);
  RETURN_OK()
END_CMD()
  using Factory_t = std::function<ClientCommandPtr(ClientMessagePtr)>;
  std::map<std::string, Factory_t> knownFactories;

  /** brace initialization won't work for the knownFactories map, so use this
   * classical init function */
  bool initKnownFactories() {
    REGISTER_CMD(status)
      REGISTER_CMD(close)
      REGISTER_CMD(play)
      REGISTER_CMD(stop)
      REGISTER_CMD(next)
      REGISTER_CMD(previous)
      REGISTER_CMD(clear)
      REGISTER_CMD(add)
      REGISTER_CMD(playlistinfo)
      REGISTER_CMD(save)
      REGISTER_CMD(listplaylists)
      REGISTER_CMD(load)
      return true;
  };

ClientCommandPtr clientCommandFactory(ClientMessagePtr clientMsg) {
  static bool initFactories = initKnownFactories();
  ClientCommandPtr command;
  std::istream is(&clientMsg->commandBuffer());
  std::string opcode;
  is >> opcode;
  auto factory = knownFactories.find(opcode);
  if (factory != knownFactories.end()) {
    BOOST_LOG_TRIVIAL(debug) << "factory found for " << opcode << " command";
    command = (*factory).second(clientMsg);
    command->extractParams(opcode, is);
  } else {
    clientMsg->setResponse(AckStatus::fromError(opcode, Error::UnknownCommand));
  }
  return command;
}

ClientSession::ClientSession(socket_t&& socket)
  : socket_(std::move(socket)), sessionNumber_(++nextSessionNumber) {}

  ClientSession::~ClientSession() {
    BOOST_LOG_TRIVIAL(debug) << "session " << sessionNumber_ << " destroy";
  }

#define VPDN(n) #n
#define VPDNN(n) VPDN(n)
#define VPD_WELCOME_LINE \
  "VPD " VPDNN(VPD_MAJOR_VERSION) "." VPDNN(VPD_MINOR_VERSION) " ready\r\n"

void ClientSession::start() {
  BOOST_LOG_TRIVIAL(info) << "Starting client session " << sessionNumber_;
  auto welcomeMsg = std::make_shared<ClientMessage>();
  welcomeMsg->setResponse(VPD_WELCOME_LINE);
  clientMessageHandled(welcomeMsg);
}

void ClientSession::readNextCommand() {
  auto thisPtr = shared_from_this();
  auto nextMsg = std::make_shared<ClientMessage>();
  io::async_read_until(socket_, nextMsg->commandBuffer(), std::string("\r\n"),
      [this, thisPtr, nextMsg](boost::system::error_code ec,
        std::size_t cmdlen) {
      if (!ec) {
      handleMessage(nextMsg);
      } else {
      // FIXME handle the case where the error code io::error::not_found is sent
      // that means that the client didn't sent the required \r\n
      handleSocketError(ec);
      }
      });
}

void ClientSession::handleMessage(ClientMessagePtr msg) {
  auto clientCmd = clientCommandFactory(msg);
  if (clientCmd) {
    BOOST_LOG_TRIVIAL(debug) << "SESS " << sessionNumber_ << " CMD";
    msg->setResponse(clientCmd->execute(shared_from_this()));
  }
  clientMessageHandled(msg);
}

void ClientSession::clientMessageHandled(ClientMessagePtr msg) {
  auto thisPtr(shared_from_this());
  std::string response = msg->response();
  BOOST_LOG_TRIVIAL(debug) << "sending " << response.length() << " bytes";
  io::async_write(socket_, io::buffer(response),
      [ this, thisPtr, resplen = response.length() ](
        boost::system::error_code ec, std::size_t bytes) {
      if (!ec) {
      if (bytes != resplen) {
      BOOST_LOG_TRIVIAL(info) << "incomplete write " << bytes << " required "
      << resplen;
      }
      readNextCommand();
      } else {
      handleSocketError(ec);
      }
      });
}

void ClientSession::closeSession() {
  socket_.get_io_service().post([self = shared_from_this()]() {
      self->socket_.close();
      PlayEngine::sessionClosed(self);
      });
}

void ClientSession::play(int pos) {
  socket_.get_io_service().post([ self = shared_from_this(), pos ]() {
      PlayEngine::play(pos);
      });
}

void ClientSession::stop() {
  socket_.get_io_service().post([]() { PlayEngine::stop(); });
}

void ClientSession::next() {
  socket_.get_io_service().post([]() { PlayEngine::next(); });
}

void ClientSession::prev() {
  socket_.get_io_service().post([]() { PlayEngine::prev(); });
}

void ClientSession::add(const std::string& uri) {
  socket_.get_io_service().post([ self = shared_from_this(), uri ]() {
      PlayEngine::add(uri);
      });
}

bool ClientSession::isValidUri(const std::string& uri) const noexcept {
  boost::regex uriex("^(.*)\\:\\/\\/(.*)$");
  boost::smatch what;
  try {
    if (!boost::regex_match(uri, what, uriex, boost::match_default)) {
      // TODO here we should first match against the database before rejecting
      // the URI
      BOOST_LOG_TRIVIAL(debug) << "invalid URI received " << uri;
      return false;
    }
  } catch (...) {
    BOOST_LOG_TRIVIAL(debug) << "exception caught when matching URI";
    return false;
  }
  return true;
}

void ClientSession::enumeratePlaylist(EnumPlaylistFn fn) {
  PlayEngine::enumeratePlaylist(fn);
}

const Player::Status& ClientSession::status() const {
  return PlayEngine::status();
}

void ClientSession::clear() const { PlayEngine::clearPlaylist(); }

void ClientSession::save(const std::string& filename) {
  socket_.get_io_service().post([ self = shared_from_this(), filename ]() {
      PlayEngine::save(filename);
      });
}

void ClientSession::enumeratePlaylists(EnumPlaylistsFn fn) {
  return PlayEngine::enumeratePlaylists(fn);
}

void ClientSession::load(const std::string& listname) {
  socket_.get_io_service().post([ self = shared_from_this(), listname ]() {
      PlayEngine::load(listname);
      });
}
}  // ClientEngine
