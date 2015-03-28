/*
 * This file is part of the VPD project
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

#ifndef  CLIENT_ENGINE_H
#define  CLIENT_ENGINE_H

#include "runtime_config.h"
#include "playlist.h"
#include "player.h"

#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <memory>

using namespace std;
namespace io = boost::asio;

namespace ClientEngine
{

// when adding error codes here, never forget to also add the corresponding string into the errorMessages_ table
enum class Error : std::size_t {
    NoError =0,
    UnknownCommand,
    CommandNotImplemented,
    TooManyArgs,
    MissingParameter,
    InvalidUri,
    InvalidFilename,
    LastError
};

auto nullExplainHandler_ = [](const char* errmsg) { return std::string(errmsg); };

struct AckStatus
{
    using AckStatusPtr = std::shared_ptr<AckStatus>;
    using EH = std::function< std::string(const char*) >;
    /**
     * @note results should contain the necessary \r\n end-of-line characters
     */
    static AckStatusPtr ok(std::string results = "", int cmdNumber =0) {
        AckStatus status;
        status.results_ = results;
        status.error_ = Error::NoError;
        status.cmdNumber_ = cmdNumber;
        return std::make_shared<AckStatus>(status);
    }
    static AckStatusPtr fromError(std::string currentCmd, Error error, EH&& explainHandler =nullExplainHandler_, int cmdNumber =0) {
        AckStatus status;
        status.currentCmd_ = currentCmd;
        status.error_ = error;
        status.cmdNumber_ = cmdNumber;
        status.explainHandler_ = explainHandler;
        return std::make_shared<AckStatus>(status);
    }
    std::string toString() const;
private:
    std::string currentCmd_;
    std::string results_;
    Error error_;
    int cmdNumber_;
    EH explainHandler_;
};

using AckStatusPtr = std::shared_ptr<AckStatus>;

struct ClientMessage
{
    static constexpr auto max_cmd_length = 1024;
    static constexpr auto max_response_length = 1024;
    ClientMessage() : commandBuffer_(max_cmd_length) {}
    void setResponse(std::string response) {
        response_ = response;
    }
    void setResponse(AckStatusPtr ackStatus);
    io::streambuf & commandBuffer() {
        return commandBuffer_;
    }
    const std::string & response() const {
        return response_;
    }
private:
    io::streambuf commandBuffer_;
    std::string response_;
};

using ClientMessagePtr = std::shared_ptr<ClientMessage>;


using socket_t = io::ip::tcp::socket;
using CommandParams = std::vector<std::string>;
using CommandParamsArg = const CommandParams&;

struct ClientSession : public std::enable_shared_from_this<ClientSession>
{
    explicit ClientSession(socket_t&& socket);
    ~ClientSession();
    void start();
    socket_t & socket() { return socket_; }

    // follow the client commands
    void closeSession();
    void play(int pos);
    void stop();
    void next();
    void prev();
    void add(const std::string &uri);
    bool isValidUri(const std::string &uri) const noexcept;
    void enumeratePlaylist(EnumPlaylistFn fn);
    const Player::Status& status() const;
    void clear() const;
    void save(const std::string& filename);
    void enumeratePlaylists(EnumPlaylistsFn fn);
    void load(const std::string& listname);
private:
    void readNextCommand();
    void handleMessage(ClientMessagePtr msg);

    void clientMessageHandled(ClientMessagePtr msg);
    void handleSocketError(boost::system::error_code ec) {
        BOOST_LOG_TRIVIAL(debug) << "socket error " << ec.message();
        closeSession();
    }

    static int nextSessionNumber;
    const int sessionNumber_;
    socket_t socket_;
};

using ClientSessionPtr = std::shared_ptr<ClientSession>;

} // namespace ClientEngine

#endif // CLIENT_ENGINE_H
