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

#include "runtime_config.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/log/trivial.hpp>

#include <iostream>
#include <fstream>
#include <iterator>

#define DEFAULT_LISTEN_PORT 7700
#define DEFAULT_LISTEN_ADDR "127.0.0.1"
#define DEFAULT_VLC_START_DELAY 10
#define DEFAULT_WORKDIR "/var/lib/vpd"

int RuntimeConfig::vlc_start_delay_ = DEFAULT_VLC_START_DELAY;
int RuntimeConfig::listen_port_ = DEFAULT_LISTEN_PORT;
string RuntimeConfig::listen_address_ = DEFAULT_LISTEN_ADDR;
string RuntimeConfig::workdir_ = DEFAULT_WORKDIR;

bool try_read_config_file(string config_file,
        const po::options_description &opts_config,
        po::variables_map &vm) {
    if (config_file.length() == 0)
        return false;
    ifstream ifs(config_file.c_str());
    if (!ifs) {
        BOOST_LOG_TRIVIAL(debug) << "cannot open configuration file: " << config_file << endl;
        return false;
    } else {
        BOOST_LOG_TRIVIAL(info) << "using configuration file " << config_file << endl;
        store(parse_config_file(ifs, opts_config), vm);
        notify(vm);
    }
    return true;
}

bool RuntimeConfig::ReadConfigFromFilesAndCmdLine(int argc, char *argv[]) {
    bool result = true;
    // setup generic options
    string config_file;
    po::options_description opts_generic("Generic options");
    opts_generic.add_options()
        ("help", "display this help message")
        ("config,c", po::value<string>(&config_file), "Configuration file path")
    ;

    // setup configuration options
    // these are typically specified into the configuration file
    po::options_description opts_config("Configuration");
    opts_config.add_options()
        ("vlc-start-delay", po::value<int>(&vlc_start_delay_)->default_value(DEFAULT_VLC_START_DELAY),
         "Delay before VLC startup, used to allow some stream data to arrive before starting it")
        ("port", po::value<int>(&listen_port_)->default_value(DEFAULT_LISTEN_PORT), "VPD will wait the commands on this port")
        ("bind_to_address", po::value<string>(&listen_address_)->default_value(DEFAULT_LISTEN_ADDR, "VPD will bind the commands listening port to this address"))
        ("workdir", po::value<string>(&workdir_)->default_value(DEFAULT_WORKDIR, "The directory where VPD should store playlists and database files"))
    ;

    po::options_description cmd_line_options;
    cmd_line_options.add(opts_generic).add(opts_config);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opts_generic), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << opts_generic << std::endl;
        return 0;
    }

    if (!try_read_config_file(config_file, opts_config, vm)) {
        if (!try_read_config_file("~/.config/vpdrc", opts_config, vm)) {
            if (!try_read_config_file("~/.vpdrc", opts_config, vm)) {
                BOOST_LOG_TRIVIAL(info) << "no configuration file found! Using default values.";
                // load_default_config();
            }
        }
    }
    return result;
}


