//--------------------------------------------------------------------------
// Copyright (C) 2014-2016 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2005-2013 Sourcefire, Inc.
// Copyright (C) 1998-2005 Martin Roesch <roesch@sourcefire.com>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------

#ifndef SNORT_H
#define SNORT_H

// Snort is the top-level application class.

#include <assert.h>
#include <sys/types.h>
#include <stdio.h>

extern "C" {
#include <daq_common.h>
}

#include "main/snort_types.h"

class Flow;
struct Packet;
struct SnortConfig;

typedef void (* MainHook_f)(Packet*);

// FIXIT-H this needs to move to detection
class SO_PUBLIC DetectionContext
{
public:
    DetectionContext();
    ~DetectionContext();

    Packet* get_packet();
};

class SO_PUBLIC Snort
{
public:
    static SnortConfig* get_reload_config(const char* fname);
    static void setup(int argc, char* argv[]);
    static void drop_privileges();
    static void cleanup();

    static bool is_starting();
    static bool is_reloading();
    static bool has_dropped_privileges();

    static bool thread_init_privileged(const char* intf);
    static void thread_init_unprivileged();
    static void thread_term();

    static void thread_idle();
    static void thread_rotate();

    static void capture_packet();

    // FIXIT-H these need to move to detection
    static Packet* set_detect_packet();
    static Packet* get_detect_packet();
    static void clear_detect_packet();
    static void detect_rebuilt_packet(Packet*);

    static struct SF_EVENTQ* get_event_queue();

    static DAQ_Verdict process_packet(
        Packet*, const DAQ_PktHdr_t*, const uint8_t* pkt, bool is_frag=false);

    static DAQ_Verdict packet_callback(void*, const DAQ_PktHdr_t*, const uint8_t*);

    static void set_main_hook(MainHook_f);

private:
    static void init(int, char**);
    static void term();
    static void clean_exit(int);

private:
    static bool initializing;
    static bool reloading;
    static bool privileges_dropped;
};

#endif

