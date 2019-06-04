/*
 * Copyright (C) 2018 Eric Tung <libpronet@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of LibProNet (http://www.libpro.org)
 */

/*
 * RFC-1889/1890, RFC-3550/3551
 */

#if !defined(RTP_PACKET_H)
#define RTP_PACKET_H

#include "rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"

/////////////////////////////////////////////////////////////////////////////
////

#if defined(PRO_TCP2_PAYLOAD_SIZE)
#undef  PRO_TCP2_PAYLOAD_SIZE
#endif
#define PRO_TCP2_PAYLOAD_SIZE (1024 * 63)

#if !defined(PRO_TCP4_PAYLOAD_SIZE)
#define PRO_TCP4_PAYLOAD_SIZE (1024 * 1024 * 64)
#endif

struct RTP_EXT
{
    PRO_UINT32    mmId                  ;
    RTP_MM_TYPE   mmType                ;

#if defined(PRO_WORDS_BIGENDIAN)
    unsigned char keyFrame           : 1;
    unsigned char firstPacketOfFrame : 1;
    unsigned char reserved           : 6;
#else
    unsigned char reserved           : 6;
    unsigned char firstPacketOfFrame : 1;
    unsigned char keyFrame           : 1;
#endif

    PRO_UINT16    hdrAndPayloadSize     ; /* tcp message boundary */

    DECLARE_SGI_POOL(0);
};

struct RTP_HEADER
{
#if defined(PRO_WORDS_BIGENDIAN)
    unsigned char      v  : 2;
    unsigned char      p  : 1;
    unsigned char      x  : 1;
    unsigned char      cc : 4;

    unsigned char      m  : 1;
    unsigned char      pt : 7;
#else
    unsigned char      cc : 4;
    unsigned char      x  : 1;
    unsigned char      p  : 1;
    unsigned char      v  : 2;

    unsigned char      pt : 7;
    unsigned char      m  : 1;
#endif

    PRO_UINT16         seq   ;
    PRO_UINT32         ts    ;
    union
    {
        PRO_UINT32     ssrc  ;
        struct
        {
            PRO_UINT16 dummy ;
            PRO_UINT16 len2  ;
        };
        PRO_UINT32     len4  ;
    };

    DECLARE_SGI_POOL(0);
};

struct RTP_PACKET
{
    RTP_EXT*    ext;
    RTP_HEADER* hdr;
    char        dummyBuffer[sizeof(RTP_EXT) + sizeof(RTP_HEADER)];

    DECLARE_SGI_POOL(0);
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpPacket : public IRtpPacket, public CProRefCount
{
public:

    static CRtpPacket* CreateInstance(
        const void*       payloadBuffer,
        unsigned long     payloadSize,
        RTP_EXT_PACK_MODE packMode
        );

    static CRtpPacket* CreateInstance(
        unsigned long     payloadSize,
        RTP_EXT_PACK_MODE packMode
        );

    static CRtpPacket* Clone(const IRtpPacket* packet);

    static bool ParseRtpBuffer(
        const char*  buffer,
        PRO_UINT16   size,
        RTP_HEADER&  hdr, /* network byte order */
        const char*& payloadBuffer,
        PRO_UINT16&  payloadSize
        );

    static bool ParseExtBuffer(
        const char* buffer,
        PRO_UINT16  size
        );

    virtual unsigned long PRO_CALLTYPE AddRef();

    virtual unsigned long PRO_CALLTYPE Release();

    virtual void PRO_CALLTYPE SetMarker(bool m);

    virtual bool PRO_CALLTYPE GetMarker() const;

    virtual void PRO_CALLTYPE SetPayloadType(char pt);

    virtual char PRO_CALLTYPE GetPayloadType() const;

    virtual void PRO_CALLTYPE SetSequence(PRO_UINT16 seq);

    virtual PRO_UINT16 PRO_CALLTYPE GetSequence() const;

    virtual void PRO_CALLTYPE SetTimeStamp(PRO_UINT32 ts);

    virtual PRO_UINT32 PRO_CALLTYPE GetTimeStamp() const;

    virtual void PRO_CALLTYPE SetSsrc(PRO_UINT32 ssrc);

    virtual PRO_UINT32 PRO_CALLTYPE GetSsrc() const;

    virtual void PRO_CALLTYPE SetMmId(PRO_UINT32 mmId);

    virtual PRO_UINT32 PRO_CALLTYPE GetMmId() const;

    virtual void PRO_CALLTYPE SetMmType(RTP_MM_TYPE mmType);

    virtual RTP_MM_TYPE PRO_CALLTYPE GetMmType() const;

    virtual void PRO_CALLTYPE SetKeyFrame(bool keyFrame);

    virtual bool PRO_CALLTYPE GetKeyFrame() const;

    virtual void PRO_CALLTYPE SetFirstPacketOfFrame(bool firstPacket);

    virtual bool PRO_CALLTYPE GetFirstPacketOfFrame() const;

    virtual const void* PRO_CALLTYPE GetPayloadBuffer() const;

    virtual void* PRO_CALLTYPE GetPayloadBuffer();

    virtual unsigned long PRO_CALLTYPE GetPayloadSize() const;

    virtual PRO_UINT16 PRO_CALLTYPE GetPayloadSize16() const;

    virtual RTP_EXT_PACK_MODE PRO_CALLTYPE GetPackMode() const;

    virtual void PRO_CALLTYPE SetTick(PRO_INT64 tick);

    virtual PRO_INT64 PRO_CALLTYPE GetTick() const;

    /*
     * Don't use this method unless you know why and how to use it.
     */
    const RTP_PACKET& GetPacket() const;

    /*
     * Don't use this method unless you know why and how to use it.
     */
    RTP_PACKET& GetPacket();

private:

    CRtpPacket(RTP_EXT_PACK_MODE packMode);

    virtual ~CRtpPacket();

    void Init(
        const void*   payloadBuffer,
        unsigned long payloadSize
        );

private:

    const RTP_EXT_PACK_MODE m_packMode;
    PRO_UINT32              m_ssrc;
    PRO_INT64               m_tick;
    RTP_PACKET*             m_packet;
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_PACKET_H */
