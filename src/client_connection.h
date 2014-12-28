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

#ifndef  CLIENT_CONNECTION_INC
#define  CLIENT_CONNECTION_INC

#include "runtime_config.h"
#include <string>
#include <boost/asio.hpp>
#include <memory>

using namespace std;
namespace io = boost::asio;

namespace ClientConnection
{

void start(io::ip::tcp::socket);

} // namespace ClientConnection

#endif // CLIENT_CONNECTION_INC
