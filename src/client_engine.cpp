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
} // ClientEngine

