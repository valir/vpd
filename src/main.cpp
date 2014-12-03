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

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/log/trivial.hpp>

#include <iostream>
#include <fstream>
#include <iterator>
using namespace std;

bool try_read_config_file(string config_file, 
        const po::options_description &opts_config,
        po::variables_map &vm) { 
    if (config_file.length() == 0)
        return false;
    ifstream ifs(config_file.c_str());
    if (!ifs) {
        BOOST_LOG_TRIVIAL(info) << "cannot open configuration file: " << config_file << std::endl;
        return false;
    } else {
        store(parse_config_file(ifs, opts_config), vm);
        notify(vm);
    }
    return true;
}

int main(int argc, char* argv[]) {
    try {
        string config_file;
        po::options_description opts_generic("Generic options");
        opts_generic.add_options()
            ("help", "display this help message")
            ("config,c", po::value<string>(&config_file), "Configuration file path")
        ;

        int vlc_start_delay;
        string streams_file;
        po::options_description opts_config("Configuration");
        opts_config.add_options()
            ("streams-file,s", po::value<string>(&streams_file)->default_value("streams.json"), "Path to the json file containing streams definition")
            ("vlc-start-delay", po::value<int>(&vlc_start_delay)->default_value(10), 
             "Delay before VLC startup, used to allow some stream data to arrive before starting it")
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
                    BOOST_LOG_TRIVIAL(error) << "no configuration file found!";
                    return 1;
                }
            }
        }
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
