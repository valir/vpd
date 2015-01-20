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


struct ClientCommand : public std::enable_shared_from_this<ClientCommand>
{
    using ClientCommandPtr = std::shared_ptr<ClientCommand>;
    using CommandFunc_t = std::function<void(ClientCommandPtr)>;
    ClientCommand(ClientMessagePtr& clientMsg, CommandFunc_t action) : clientMsg_(clientMsg) {}
    void execute(ClientSessionPtr session);
protected:
    virtual void collectParameters();
    ClientMessagePtr &clientMsg_;
    std::string opcode_;
};

using ClientCommandPtr = std::shared_ptr<ClientCommand>;

auto closeFactory = [](ClientMessagePtr msg){
    return std::make_shared<ClientCommand>(msg,
                [](ClientCommandPtr) { } );
    };

auto statusFactory = [](ClientMessagePtr msg){
    return std::make_shared<ClientCommand>(msg,
                [](ClientCommandPtr) { } );
    };

using Factory_t = std::function<ClientCommandPtr(ClientMessagePtr)>;
std::map< std::string, Factory_t > knownFactories;

/** brace initialization won't work for the knownFactories map, so use this classical init function */
void initKnownFactories() {
    knownFactories.emplace( "status", statusFactory );
    knownFactories.emplace( "close" , closeFactory );
};

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

ClientSession::ClientSession(socket_t&& socket) :
    socket_(std::move(socket))
    , sessionNumber_(++nextSessionNumber)
    {
    }

void ClientSession::readNextCommand() {
    auto thisPtr = shared_from_this();
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
        clientCmd->execute(shared_from_this());
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

