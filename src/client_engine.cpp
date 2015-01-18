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

#include <boost/log/trivial.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <boost/asio.hpp>

namespace ClientEngine {


int ClientSession::nextSessionNumber = 0;

const char* errorMessages_[static_cast<std::size_t>(Error::LastError)] = {
    "No error",
    "unknown command",
};

std::string AckStatus::toString(EH&& explainHandler) const {
    std::stringbuf sb;
    std::ostream os(&sb);
    os << "ACK [" << static_cast<std::size_t>(error_) << "@" << cmdNumber_ << "] {"
        << currentCmd_ << "} " << explainHandler(errorMessages_[static_cast<std::size_t>(error_)])
        << "\r\n";
    return sb.str();
}


struct ClientCommand
{
    ClientCommand(ClientMessagePtr& clientMsg) : clientMsg_(clientMsg) {}
private:
    ClientMessagePtr &clientMsg_;
    std::string opcode_;
};

using ClientCommandPtr = std::shared_ptr<ClientCommand>;

struct CloseCommand : public ClientCommand
{
    CloseCommand(ClientMessagePtr msg) : ClientCommand(msg) {}
};


auto closeFactory = [](ClientMessagePtr msg){ return std::make_shared<CloseCommand>(msg); };

typedef decltype(closeFactory) Factory_t; // this typef picks one of the factories declarations, but NOTE all of the factories declarations must have the same type

std::map< std::string, Factory_t> knownFactories = {
    {  "close", closeFactory  }
};

void executeCommand(ClientCommandPtr clientCmd) {
    // TODO
}
ClientCommandPtr clientCommandFactory(ClientMessagePtr &clientMsg) {
    ClientCommandPtr command;
    std::istream is(&clientMsg->commandBuffer());
    std::string opcode;
    is >> opcode;
    auto factory = knownFactories.find(opcode);
    if (factory != knownFactories.end()) {
        BOOST_LOG_TRIVIAL(debug) << "factory found for " << opcode << " command";
        command = (*factory).second(clientMsg);
    }
    return command;
}

void ClientSession::readNextCommand() {
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
void ClientSession::handleCommand(ClientMessagePtr msg) {
    BOOST_LOG_TRIVIAL(debug) << "SESS " << sessionNumber_ << " CMD";
    auto clientCmd = clientCommandFactory(msg);
    if (clientCmd) {
        executeCommand(clientCmd);
    }
    clientMessageHandled(msg);
}



void ClientSession::clientMessageHandled(ClientMessagePtr msg) {
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
} // ClientEngine

