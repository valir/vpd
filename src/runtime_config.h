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
#ifndef  runtime_config_INC
#define  runtime_config_INC

#include <string>

using namespace std;

struct RuntimeConfig {
    static int vlc_start_delay_;
    static int listen_port_;
    static string listen_address_;
    static string workdir_;

    static bool ReadConfigFromFilesAndCmdLine(int argc, char* argv[]);
};

#endif   /* ----- #ifndef runtime_config_INC  ----- */
