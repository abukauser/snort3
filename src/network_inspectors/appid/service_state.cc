//--------------------------------------------------------------------------
// Copyright (C) 2014-2016 Cisco and/or its affiliates. All rights reserved.
// Copyright (C) 2005-2013 Sourcefire, Inc.
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

// service_state.cc author Sourcefire Inc.

#include "service_state.h"

#include "hash/sfxhash.h"
#include "log/messages.h"
#include "service_plugins/service_base.h"
#include "sfip/sf_ip.h"
#include "utils/util.h"

//#define DEBUG_SERVICE_STATE 1

static THREAD_LOCAL SFXHASH* serviceStateCache4;
static THREAD_LOCAL SFXHASH* serviceStateCache6;

#define SERVICE_STATE_CACHE_ROWS    65536

static int AppIdServiceStateFree(void*, void* data)
{
    AppIdServiceIDState* id_state = (AppIdServiceIDState*)data;

    if (id_state->service_list)
    {
        AppIdFreeServiceMatchList(id_state->service_list);
        id_state->service_list = nullptr;
    }

    return 0;
}

int init_service_state(unsigned long memcap)
{
    serviceStateCache4 = sfxhash_new(SERVICE_STATE_CACHE_ROWS,
        sizeof(AppIdServiceStateKey4), sizeof(AppIdServiceIDState), memcap >> 1, 1,
        &AppIdServiceStateFree, &AppIdServiceStateFree, 1);
    if (!serviceStateCache4)
    {
        ErrorMessage("Failed to allocate a hash table");
        return -1;
    }
    serviceStateCache6 = sfxhash_new(SERVICE_STATE_CACHE_ROWS,
        sizeof(AppIdServiceStateKey6), sizeof(AppIdServiceIDState), memcap >> 1, 1,
        &AppIdServiceStateFree, &AppIdServiceStateFree, 1);
    if (!serviceStateCache6)
    {
        ErrorMessage("Failed to allocate a hash table");
        return -1;
    }
    return 0;
}

void clean_service_state(void)
{
    if (serviceStateCache4)
    {
        sfxhash_delete(serviceStateCache4);
        serviceStateCache4 = nullptr;
    }

    if (serviceStateCache6)
    {
        sfxhash_delete(serviceStateCache6);
        serviceStateCache6 = nullptr;
    }
}

void remove_service_id_state(const sfip_t* ip, IpProtocol proto, uint16_t port, uint32_t level)
{
    AppIdServiceStateKey k;
    SFXHASH* cache;

    if (sfip_family(ip) == AF_INET6)
    {
        k.key6.proto = proto;
        k.key6.port = port;
        memcpy(k.key6.ip, ip->ip32, sizeof(k.key6.ip));
        k.key6.level = level;
        cache = serviceStateCache6;
    }
    else
    {
        k.key4.proto = proto;
        k.key4.port = port;
        k.key4.ip = ip->ip32[0];
        k.key4.level = level;
        cache = serviceStateCache4;
    }
    if (sfxhash_remove(cache, &k) != SFXHASH_OK)
    {
        char ipstr[INET6_ADDRSTRLEN];

        ipstr[0] = 0;
        sfip_ntop(ip, ipstr, sizeof(ipstr));
        ErrorMessage("Failed to remove from hash: %s:%u:%u\n", ipstr, (unsigned)proto,
            (unsigned)port);
    }
}

AppIdServiceIDState* get_service_id_state(const sfip_t* ip, IpProtocol proto, uint16_t port,
        uint32_t level)
{
    SFXHASH* cache;
    AppIdServiceStateKey k;
    char ipstr[INET6_ADDRSTRLEN];   // FIXIT-M ASAN reports mem leak on ServiceMatch* objects if this is not defined
                                    //  which makes no sense, need to investigate further

    if (sfip_family(ip) == AF_INET6)
    {
        k.key6.proto = proto;
        k.key6.port = port;
        memcpy(k.key6.ip, ip->ip32, sizeof(k.key6.ip));
        k.key6.level = level;
        cache = serviceStateCache6;
    }
    else
    {
        k.key4.proto = proto;
        k.key4.port = port;
        k.key4.ip = ip->ip32[0];
        k.key4.level = level;
        cache = serviceStateCache4;
    }
    AppIdServiceIDState* ss = (AppIdServiceIDState*)sfxhash_find(cache, &k);

#ifdef DEBUG_SERVICE_STATE
    ipstr[0] = 0;
    sfip_ntop(ip, ipstr, sizeof(ipstr));
    DebugFormat(DEBUG_APPID, "ServiceState: Read from hash: %s:%u:%u:%u %p %u %p\n", ipstr, (unsigned)proto,
        (unsigned)port, level, (void*)ss, ss ? ss->state : 0, ss ? (void*)ss->svc : nullptr);
#else
    UNUSED(ipstr);
#endif

    if (ss && ss->svc && !ss->svc->ref_count)
    {
        ss->svc = nullptr;
        ss->state = SERVICE_ID_NEW;
    }

    return ss;
}

AppIdServiceIDState* add_service_id_state(const sfip_t* ip, IpProtocol proto, uint16_t port,
    uint32_t level)
{
    AppIdServiceStateKey k;
    AppIdServiceIDState* ss;
    SFXHASH* cache;

    if (sfip_family(ip) == AF_INET6)
    {
        k.key6.proto = proto;
        k.key6.port = port;
        memcpy(k.key6.ip, ip->ip32, sizeof(k.key6.ip));
        k.key6.level = level;
        cache = serviceStateCache6;
    }
    else
    {
        k.key4.proto = proto;
        k.key4.port = port;
        k.key4.ip = ip->ip32[0];
        k.key4.level = level;
        cache = serviceStateCache4;
    }

    if ((sfxhash_add_return_data_ptr(cache, &k, (void**)&ss) < 0) || !ss)
    {
        char ipstr[INET6_ADDRSTRLEN];

        sfip_ntop(ip, ipstr, sizeof(ipstr));
        ErrorMessage("ServiceState: Failed to add to hash: %s:%u:%u:%u\n", ipstr, (unsigned)proto,
            (unsigned)port, level);
        return nullptr;
    }

#ifdef DEBUG_SERVICE_STATE
    char ipstr[INET6_ADDRSTRLEN];

    ipstr[0] = 0;
    sfip_ntop(ip, ipstr, sizeof(ipstr));
    DebugFormat(DEBUG_APPID, "ServiceState: Added to hash: %s:%u:%u:%u %p\n", ipstr, (unsigned)proto,
        (unsigned)port, level, (void*)ss);
#endif

    return ss;
}

void dump_service_state_stats(void)
{
    LogMessage("Service State:\n");
    if (serviceStateCache4)
    {
        LogMessage("           IPv4 Count: %u\n", sfxhash_count(serviceStateCache4));
        LogMessage("    IPv4 Memory Limit: %lu\n", serviceStateCache4->mc.memcap);
        LogMessage("     IPv4 Memory Used: %lu\n", serviceStateCache4->mc.memused);
    }
    if (serviceStateCache6)
    {
        LogMessage("           IPv6 Count: %u\n", sfxhash_count(serviceStateCache6));
        LogMessage("    IPv6 Memory Limit: %lu\n", serviceStateCache6->mc.memcap);
        LogMessage("     IPv6 Memory Used: %lu\n", serviceStateCache6->mc.memused);
    }
}

