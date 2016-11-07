//--------------------------------------------------------------------------
// Copyright (C) 2014-2016 Cisco and/or its affiliates. All rights reserved.
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
// http_flow_data.h author Tom Peters <thopeter@cisco.com>

#ifndef HTTP_FLOW_DATA_H
#define HTTP_FLOW_DATA_H

#include <stdio.h>
#include <zlib.h>

#include "flow/flow.h"
#include "mime/file_mime_process.h"
#include "utils/util_utf.h"

#include "http_cutter.h"
#include "http_infractions.h"
#include "http_event_gen.h"

class HttpTransaction;

class HttpFlowData : public FlowData
{
public:
    HttpFlowData();
    ~HttpFlowData();
    static unsigned http_flow_id;
    static void init() { http_flow_id = FlowData::get_flow_id(); }

    friend class HttpInspect;
    friend class HttpMsgSection;
    friend class HttpMsgStart;
    friend class HttpMsgRequest;
    friend class HttpMsgStatus;
    friend class HttpMsgHeader;
    friend class HttpMsgHeadShared;
    friend class HttpMsgTrailer;
    friend class HttpMsgBody;
    friend class HttpMsgBodyChunk;
    friend class HttpMsgBodyCl;
    friend class HttpMsgBodyOld;
    friend class HttpStreamSplitter;
    friend class HttpTransaction;
#ifdef REG_TEST
    friend class HttpUnitTestSetup;
#endif

private:
    // Convenience routines
    void half_reset(HttpEnums::SourceId source_id);
    void trailer_prep(HttpEnums::SourceId source_id);

    // 0 element refers to client request, 1 element refers to server response

    // FIXIT-P reorganize HttpFlowData to minimize void space

    // *** StreamSplitter internal data - scan()
    HttpCutter* cutter[2] = { nullptr, nullptr };

    // *** StreamSplitter internal data - reassemble()
    uint8_t* section_buffer[2] = { nullptr, nullptr };
    uint32_t section_offset[2] = { 0, 0 };
    HttpEnums::ChunkState chunk_state[2] = { HttpEnums::CHUNK_NUMBER, HttpEnums::CHUNK_NUMBER };
    uint32_t chunk_expected_length[2] = { 0, 0 };

    // *** StreamSplitter internal data - scan() => reassemble()
    uint32_t num_excess[2] = { 0, 0 };
    bool is_broken_chunk[2] = { false, false };
    uint32_t num_good_chunks[2] = { 0, 0 };

    // *** StreamSplitter => Inspector (facts about the most recent message section)
    HttpEnums::SectionType section_type[2] = { HttpEnums::SEC__NOT_COMPUTE,
                                                HttpEnums::SEC__NOT_COMPUTE };
    bool tcp_close[2] = { false, false };
    HttpInfractions infractions[2];
    HttpEventGen events[2];
    int32_t num_head_lines[2] = { HttpEnums::STAT_NOT_PRESENT, HttpEnums::STAT_NOT_PRESENT };

    // *** Inspector => StreamSplitter (facts about the message section that is coming next)
    HttpEnums::SectionType type_expected[2] = { HttpEnums::SEC_REQUEST, HttpEnums::SEC_STATUS };
    // length of the data from Content-Length field
    int64_t data_length[2] = { HttpEnums::STAT_NOT_PRESENT, HttpEnums::STAT_NOT_PRESENT };
    uint32_t section_size_target[2] = { 0, 0 };
    uint32_t section_size_max[2] = { 0, 0 };
    HttpEnums::CompressId compression[2] = { HttpEnums::CMP_NONE, HttpEnums::CMP_NONE };
    z_stream* compress_stream[2] = { nullptr, nullptr };
    uint64_t zero_nine_expected = 0;

    // *** Inspector's internal data about the current message
    HttpEnums::VersionId version_id[2] = { HttpEnums::VERS__NOT_PRESENT,
                                            HttpEnums::VERS__NOT_PRESENT };
    HttpEnums::MethodId method_id = HttpEnums::METH__NOT_PRESENT;
    int32_t status_code_num = HttpEnums::STAT_NOT_PRESENT;
    int64_t file_depth_remaining[2] = { HttpEnums::STAT_NOT_PRESENT,
        HttpEnums::STAT_NOT_PRESENT };
    int64_t detect_depth_remaining[2] = { HttpEnums::STAT_NOT_PRESENT,
        HttpEnums::STAT_NOT_PRESENT };
    MimeSession* mime_state = nullptr;  // SRC_CLIENT only
    UtfDecodeSession* utf_state = nullptr; //SRC_SERVER only
    class HttpMsgSection* latest_section = nullptr;
    uint64_t expected_trans_num[2] = { 1, 1 };

    // number of user data octets seen so far (regular body or chunks)
    int64_t body_octets[2] = { HttpEnums::STAT_NOT_PRESENT, HttpEnums::STAT_NOT_PRESENT };

    // Transaction management including pipelining
    // FIXIT-L pipeline deserves to be its own class
    HttpTransaction* transaction[2] = { nullptr, nullptr };
    static const int MAX_PIPELINE = 100;  // requests seen - responses seen <= MAX_PIPELINE
    HttpTransaction** pipeline = nullptr;
    int pipeline_front = 0;
    int pipeline_back = 0;
    bool pipeline_overflow = false;
    bool pipeline_underflow = false;

    bool add_to_pipeline(HttpTransaction* latest);
    HttpTransaction* take_from_pipeline();
    void delete_pipeline();

#ifdef REG_TEST
    void show(FILE* out_file) const;

    static uint64_t instance_count;
    uint64_t seq_num;
#endif
};

#endif

