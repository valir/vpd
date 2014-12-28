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

#include "runtime_config.h"
#include "play_engine.h"
#include "remote_engine.h"

#include <iostream>
#include <thread>
#include <future>
#include <boost/log/trivial.hpp>
#include <boost/test/minimal.hpp>

using namespace std;

int test_main(int argc, char* argv[]) {
    try {
        RuntimeConfig config;
        config.ReadConfigFromFilesAndCmdLine(argc, argv);

        auto pef = async(launch::async, PlayEngine::start, config);
        // auto cef = async(launch::async, RemoteEngine::start, config);
        BOOST_LOG_TRIVIAL(debug) << "main thread waiting for play engine quit";
        return pef.get(); // wait until the player receives the quit command
    }
    catch (exception &e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch (...){
        cerr << "Exception of unknown type!\n";
    }
    return 0;
}
