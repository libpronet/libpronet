/*
 * Copyright (C) 2018-2019 Eric Tung <libpronet@gmail.com>
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
 * This file is part of LibProNet (https://github.com/libpronet/libpronet)
 */

/*
 * RFC-1889/1890, RFC-3550/3551
 */

#if !defined(RTP_PACKET_H)
#define RTP_PACKET_H

#include "rtp_base.h"
#include "../pro_util/pro_memory_pool.h"
#include "../pro_util/pro_ref_count.h"
#include "../pro_util/pro_z.h"

/////////////////////////////////////////////////////////////////////////////
////

struct RTP_EXT
{
    uint32_t      mmId;
    RTP_MM_TYPE   mmType;

#if defined(PRO_WORDS_BIGENDIAN)
    unsigned char keyFrame           : 1;
    unsigned char firstPacketOfFrame : 1;
    unsigned char udpxSync           : 1;
    unsigned char reserved           : 5;
#else
    unsigned char reserved           : 5;
    unsigned char udpxSync           : 1;
    unsigned char firstPacketOfFrame : 1;
    unsigned char keyFrame           : 1;
#endif

    uint16_t      hdrAndPayloadSize; /* tcp message boundary */

    DECLARE_SGI_POOL(0)
};

struct RTP_HEADER
{
#if defined(PRO_WORDS_BIGENDIAN)
    unsigned char    v  : 2;
    unsigned char    p  : 1;
    unsigned char    x  : 1;
    unsigned char    cc : 4;

    unsigned char    m  : 1;
    unsigned char    pt : 7;
#else
    unsigned char    cc : 4;
    unsigned char    x  : 1;
    unsigned char    p  : 1;
    unsigned char    v  : 2;

    unsigned char    pt : 7;
    unsigned char    m  : 1;
#endif

    uint16_t         seq;
    uint32_t         ts;
    union
    {
        uint32_t     ssrc;
        struct
        {
            uint16_t dummy;
            uint16_t len2;
        };
        uint32_t     len4;
    };

    DECLARE_SGI_POOL(0)
};

struct RTP_PACKET
{
    RTP_EXT*    ext;
    RTP_HEADER* hdr;
    char        dummyBuffer[sizeof(RTP_EXT) + sizeof(RTP_HEADER)];

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

class CRtpPacket : public IRtpPacket, public CProRefCount
{
public:

    static CRtpPacket* CreateInstance(
        const void*       payloadBuffer,
        size_t            payloadSize,
        RTP_EXT_PACK_MODE packMode
        );

    static CRtpPacket* CreateInstance(
        size_t            payloadSize,
        RTP_EXT_PACK_MODE packMode
        );

    static CRtpPacket* Clone(const IRtpPacket* packet);

    static bool ParseRtpBuffer(
        const char*  buffer,
        uint16_t     size,
        RTP_HEADER&  hdr, /* network byte order */
        const char*& payloadBuffer,
        uint16_t&    payloadSize
        );

    static bool ParseExtBuffer(
        const char* buffer,
        uint16_t    size
        );

    virtual unsigned long AddRef();

    virtual unsigned long Release();

    virtual void SetMarker(bool m);

    virtual bool GetMarker() const;

    virtual void SetPayloadType(char pt);

    virtual char GetPayloadType() const;

    virtual void SetSequence(uint16_t seq);

    virtual uint16_t GetSequence() const;

    virtual void SetTimeStamp(uint32_t ts);

    virtual uint32_t GetTimeStamp() const;

    virtual void SetSsrc(uint32_t ssrc);

    virtual uint32_t GetSsrc() const;

    virtual void SetMmId(uint32_t mmId);

    virtual uint32_t GetMmId() const;

    virtual void SetMmType(RTP_MM_TYPE mmType);

    virtual RTP_MM_TYPE GetMmType() const;

    virtual void SetKeyFrame(bool keyFrame);

    virtual bool GetKeyFrame() const;

    virtual void SetFirstPacketOfFrame(bool firstPacket);

    virtual bool GetFirstPacketOfFrame() const;

    virtual const void* GetPayloadBuffer() const;

    virtual void* GetPayloadBuffer();

    virtual size_t GetPayloadSize() const;

    virtual uint16_t GetPayloadSize16() const;

    virtual RTP_EXT_PACK_MODE GetPackMode() const
    {
        return m_packMode;
    }

    virtual void SetMagic(int64_t magic)
    {
        m_magic = magic;
    }

    virtual int64_t GetMagic() const
    {
        return m_magic;
    }

    void SetMagic2(int64_t magic2)
    {
        m_magic2 = magic2;
    }

    int64_t GetMagic2() const
    {
        return m_magic2;
    }

    void SetUdpxSync(bool sync);

    bool GetUdpxSync() const;

    /*
     * Don't use this method unless you know why and how to use it.
     */
    const RTP_PACKET& GetPacket() const
    {
        return *m_packet;
    }

    /*
     * Don't use this method unless you know why and how to use it.
     */
    RTP_PACKET& GetPacket()
    {
        return *m_packet;
    }

private:

    CRtpPacket(RTP_EXT_PACK_MODE packMode);

    virtual ~CRtpPacket();

    void Init(
        const void* payloadBuffer,
        size_t      payloadSize
        );

private:

    const RTP_EXT_PACK_MODE m_packMode;
    uint32_t                m_ssrc; /* for RTP_EPM_TCP2, RTP_EPM_TCP4 */
    int64_t                 m_magic;
    int64_t                 m_magic2;
    RTP_PACKET*             m_packet;

    DECLARE_SGI_POOL(0)
};

/////////////////////////////////////////////////////////////////////////////
////

#endif /* RTP_PACKET_H */
