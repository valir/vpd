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

#include <string>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <memory>

using namespace std;
namespace io = boost::asio;

namespace ClientEngine
{

struct ClientMessage
{
    static constexpr auto max_cmd_length = 100;
    static constexpr auto max_response_length = 1024;
    ClientMessage() : commandBuffer_(max_cmd_length) {}
    void setResponse(std::string response) {
        std::strncpy(response_, response.c_str(), max_response_length -1);
    }
    io::streambuf & commandBuffer() {
        commandBuffer_.consume(commandBuffer_.size());
        return commandBuffer_;
    }
    auto responseBuffer() {
        return io::buffer(response_);
    }
private:
    io::streambuf commandBuffer_;
    char response_[max_response_length];
};

using ClientMessagePtr = std::shared_ptr<ClientMessage>;

enum class Error : std::size_t {
    NoError =0,
    UnknownCommand,
    LastError
};

auto nullExplainHandler_ = [](const char* errmsg) { return std::string(errmsg); };

struct AckStatus
{
    static AckStatus fromError(std::string currentCmd, Error error, int cmdNumber =0) {
        AckStatus status;
        status.currentCmd_ = currentCmd;
        status.error_ = error;
        status.cmdNumber_ = cmdNumber;
        return std::move(status);
    }
    using EH = std::function< std::string(const char*) >;
    std::string toString(EH&& explainHandler =nullExplainHandler_) const;
private:
    std::string currentCmd_;
    Error error_;
    int cmdNumber_;

};

#define VPDN(n) #n
#define VPDNN(n) VPDN(n)
#define VPD_WELCOME_LINE "VPD " VPDNN(VPD_MAJOR_VERSION) "." VPDNN(VPD_MINOR_VERSION) " ready\r\n"
struct ClientCommand
{
    ClientCommand(ClientMessagePtr& clientMsg) : clientMsg_(clientMsg) {}
    bool isValid() {
        std::istream is(&clientMsg_->commandBuffer());
        is >> opcode_;
        if (!checkOpcode()) {
            clientMsg_->setResponse(AckStatus::fromError(opcode_, Error::UnknownCommand).toString());
        }
        return true;
    }
    bool checkOpcode() {
        return false;
    }
private:
    ClientMessagePtr &clientMsg_;
    std::string opcode_;
};

using ClientCommandPtr = std::shared_ptr<ClientCommand>;

using socket_t = io::ip::tcp::socket;

struct ClientSession : public std::enable_shared_from_this<ClientSession>
{
    explicit ClientSession(socket_t&& socket) :
        socket_(std::move(socket))
        , sessionNumber_(++nextSessionNumber) {
    }

    void start() {
        BOOST_LOG_TRIVIAL(info) << "Starting client session " << sessionNumber_;
        auto welcomeMsg = std::make_shared<ClientMessage>();
        welcomeMsg->setResponse(VPD_WELCOME_LINE);
        clientMessageHandled(welcomeMsg);
    }

    void readNextCommand() {
        auto thisPtr(shared_from_this());
        auto nextCommand = std::make_shared<ClientMessage>();
        io::async_read_until(socket_,
                nextCommand->commandBuffer(), std::string("\r\n"),
                [this, thisPtr, nextCommand](boost::system::error_code ec, std::size_t cmdlen) {
                    if (!ec)
                    {
                        handleCommand(nextCommand);
                    }
                    else
                    {
                        // FIXME handle the case where the error code io::error::not_found is sent
                        // that means that the client didn't sent the required \r\n
                        handleSocketError(ec);
                    }
                });
    }

    void handleCommand(ClientMessagePtr msg) {
        BOOST_LOG_TRIVIAL(debug) << "SESS " << sessionNumber_ << " CMD";
        auto clientCmd = std::make_shared<ClientCommand>(msg);
        if (clientCmd->isValid()) {
            executeCommand(clientCmd);
        }
        clientMessageHandled(msg);
    }

    void executeCommand(ClientCommandPtr clientCmd) {
    }

    void clientMessageHandled(ClientMessagePtr msg) {
        auto thisPtr(shared_from_this());
        io::async_write(socket_, msg->responseBuffer(),
            [this, thisPtr](boost::system::error_code ec, std::size_t){
                if (!ec) {
                    readNextCommand();
                } else {
                    handleSocketError(ec);
                }
            });
    }

    void handleSocketError(boost::system::error_code ec) {
        BOOST_LOG_TRIVIAL(debug) << "socket error " << ec.message();
        closeSession();
    }

    void closeSession() {
        socket_.close();
    }

    socket_t & socket() { return socket_; }
private:
    static int nextSessionNumber;
    const int sessionNumber_;
    socket_t socket_;
};

} // namespace ClientEngine

#endif // CLIENT_ENGINE_H
