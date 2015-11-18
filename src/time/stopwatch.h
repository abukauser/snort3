//--------------------------------------------------------------------------
// Copyright (C) 2015-2015 Cisco and/or its affiliates. All rights reserved.
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
// stopwatch.h author Joel Cornett <jocornet@cisco.com>

#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <chrono>

#include "clock_defs.h"

class Stopwatch
{
public:
    Stopwatch() :
        elapsed { hr_duration::zero() }, running { false } { }

    void start()
    {
        if ( running )
            return;

        start_time = hr_clock::now();
        running = true;
    }

    void stop()
    {
        if ( !running )
            return;

        elapsed += get_delta();
        running = false;
    }

    hr_duration get() const
    {
        if ( running )
            return elapsed + get_delta();

        return elapsed;
    }

    bool alive() const
    { return running; }

    void reset()
    { running = false; elapsed = hr_duration::zero(); }

    void cancel()
    { running = false; }

private:
    hr_duration get_delta() const
    { return hr_clock::now() - start_time; }

    hr_duration elapsed;
    bool running;
    hr_time start_time;
};

#endif