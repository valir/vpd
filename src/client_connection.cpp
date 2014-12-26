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
#include "client_connection.h"
#include "config.h"

#include <boost/log/trivial.hpp>
#include <thread>
#include <chrono>
#include <iostream>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <boost/asio.hpp>

namespace ClientConnection {

namespace msm = boost::msm;
namespace mpl = boost::mpl;
using namespace boost::msm::front;


struct Connection : public msm::front::state_machine_def<Connection> {
    struct StateHandshake : public msm::front::state<> {
    };

    typedef Connection c; // makes transition table cleaner
    // struct transition_table : mpl::vector<
    //         //      Start           Event       Next        Action      Guard 
    //         //  +-----------+-----------+-----------+------------+---------+
    //     a_row <     StateHandshake,      invalid_client,     Disconnected,             >
    // > {};
};

#define VPD_WELCOME_LINE(VERS_MAJ, VERS_MIN) "VPD #VERS_MAJ.#VERS_MIN ready\r\n"

struct ClientSession;

struct ClientDialog : public msm::front::state_machine_def<ClientDialog> {
    ClientDialog(ClientSession &session) : session_(session) {}
    
    struct EventConnectionError {
        EventConnectionError(boost::system::error_code ec) : ec_(ec) {}
        boost::system::error_code ec_;
    };

    struct EventCommandReceived {
        EventCommandReceived(string cmd) : cmd_(cmd) {}
        string cmd_;
    };

    struct StateWaitCommand : public msm::front::state<> {
        template <class Event, class Fsm>
        void on_entry(Event const&, Fsm& fsm) {
            io::async_write(fsm.session_.socket_, io::buffer(VPD_WELCOME_LINE(VPD_MAJOR_VERSION, VPD_MINOR_VERSION)),
                [this, &fsm](boost::system::error_code ec, std::size_t){
                    if (!ec) {
                        io::async_read_until(fsm.session_.socket_, data_, "\r\n",
                            [this, &fsm](boost::system::error_code ec, std::size_t bytes){
                                if (!ec) {
                                    std::istream is(&data_);
                                    fsm.process_event(EventCommandReceived(""));
                                } else {
                                    fsm.process_event(EventConnectionError(ec));
                                } 
                            });
                    } else {
                        fsm.process_event(EventConnectionError(ec));
                    }
                });
        }
        io::streambuf data_;
    };

    struct StateExecuteCommand : public msm::front::state<> {
        template <class Event, class Fsm>
        void on_entry(Event const &cmdEvent, Fsm& fsm) {
        }
    };

    typedef StateWaitCommand initial_state;

    // transition actions
    

    // transitions table
    typedef ClientDialog cd;
    struct transition_table : mpl::vector<
        Row< StateWaitCommand, EventCommandReceived, StateExecuteCommand  >
    > {};

private:
    ClientSession &session_;
};

struct ClientSession {
    ClientSession(io::ip::tcp::socket socket) : socket_(std::move(socket)) {}
    void start() {
        msm::back::state_machine<ClientDialog> clientDialog(boost::ref(*this)); 
        clientDialog.start();
    }
    io::ip::tcp::socket socket_;
};

void start(io::ip::tcp::socket socket) {
    BOOST_LOG_TRIVIAL(debug) << "New client connected";

    ClientSession session(std::move(socket));
    session.start();
}

} // ClientConnection

