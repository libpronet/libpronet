/* Sole is a lightweight C++11 library to generate universally unique identificators.
 * Sole provides interface for UUID versions 0, 1 and 4.

 * https://github.com/r-lyeh/sole
 * Copyright (c) 2013,2014,2015 r-lyeh. zlib/libpng licensed.

 * Based on code by Dmitri Bouianov, Philip O'Toole, Poco C++ libraries and anonymous
 * code found on the net. Thanks guys!

 * Theory: (see Hoylen's answer at [1])
 * - UUID version 1 (48-bit MAC address + 60-bit clock with a resolution of 100ns)
 *   Clock wraps in 3603 A.D.
 *   Up to 10000000 UUIDs per second.
 *   MAC address revealed.
 *
 * - UUID Version 4 (122-bits of randomness)
 *   See [2] or other analysis that describe how very unlikely a duplicate is.
 *
 * - Use v1 if you need to sort or classify UUIDs per machine.
 *   Use v1 if you are worried about leaving it up to probabilities (e.g. your are the
 *   type of person worried about the earth getting destroyed by a large asteroid in your
 *   lifetime). Just use a v1 and it is guaranteed to be unique till 3603 AD.
 *
 * - Use v4 if you are worried about security issues and determinism. That is because
 *   v1 UUIDs reveal the MAC address of the machine it was generated on and they can be
 *   predictable. Use v4 if you need more than 10 million uuids per second, or if your
 *   application wants to live past 3603 A.D.

 * Additionally a custom UUID v0 is provided:
 * - 16-bit PID + 48-bit MAC address + 60-bit clock with a resolution of 100ns since Unix epoch
 * - Format is EPOCH_LOW-EPOCH_MID-VERSION(0)|EPOCH_HI-PID-MAC
 * - Clock wraps in 3991 A.D.
 * - Up to 10000000 UUIDs per second.
 * - MAC address and PID revealed.

 * References:
 * - [1] http://stackoverflow.com/questions/1155008/how-unique-is-uuid
 * - [2] http://en.wikipedia.org/wiki/UUID#Random%5FUUID%5Fprobability%5Fof%5Fduplicates
 * - http://en.wikipedia.org/wiki/Universally_unique_identifier
 * - http://en.cppreference.com/w/cpp/numeric/random/random_device
 * - http://www.itu.int/ITU-T/asn1/uuid.html f81d4fae-7dec-11d0-a765-00a0c91e6bf6

 * - rlyeh ~~ listening to Hedon Cries / Until The Sun Goes up
 */

//////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <stdint.h>
#include <stdio.h>     // for size_t; should be stddef.h instead; however, clang+archlinux fails when compiling it (@Travis-Ci)
#include <sys/types.h> // for uint32_t; should be stdint.h instead; however, GCC 5 on OSX fails when compiling it (See issue #11)
#include <functional>
#include <string>

// public API

#define SOLE_VERSION "1.0.5" /* (2022/07/15): Fix #42 (clang: __thread vs threadlocal)
#define SOLE_VERSION "1.0.4" // (2022/04/09): Fix potential threaded issues (fix #18, PR #39) and a socket leak (fix #38)
#define SOLE_VERSION "1.0.3" // (2022/01/17): Merge fixes by @jasonwinterpixel(emscripten) + @jj-tetraquark(get_any_mac)
#define SOLE_VERSION "1.0.2" // (2021/03/16): Merge speed improvements by @vihangm
#define SOLE_VERSION "1.0.1" // (2017/05/16): Improve UUID4 and base62 performance; fix warnings
#define SOLE_VERSION "1.0.0" // (2016/02/03): Initial semver adherence; Switch to header-only; Remove warnings */

namespace sole {

// 128-bit basic UUID type that allows comparison and sorting.
// Use .str() for printing and .pretty() for pretty printing.
// Also, ostream friendly
struct uuid
{
    uint64_t ab;
    uint64_t cd;

    std::string str() const;
};

// Generators
uuid uuid1(); // UUID v1, pro: unique; cons: MAC revealed, predictable
uuid uuid4(); // UUID v4, pros: anonymous, fast; con: uuids "can clash"

} // ::sole

// implementation

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <cstring>
#include <ctime>

#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#   include <winsock2.h>
#   include <process.h>
#   include <iphlpapi.h>
#   ifdef _MSC_VER
#      pragma comment(lib,"iphlpapi.lib")
#   endif
#   define SOLE_WINDOWS
#elif defined(__FreeBSD__) || defined(__NetBSD__) || \
      defined(__OpenBSD__) || defined(__MINT__) || defined(__bsdi__)
#   include <ifaddrs.h>
#   include <net/if_dl.h>
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <unistd.h>
#   define SOLE_BSD
#elif defined(__APPLE__)
#   include <TargetConditionals.h>
#   if TARGET_OS_OSX
#      include <ifaddrs.h>
#      include <net/if_dl.h>
#      include <sys/socket.h>
#      include <sys/time.h>
#      include <sys/types.h>
#      include <unistd.h>
#      define SOLE_APPLE_OSX
#   endif
#elif defined(__linux__)
#   if defined(ANDROID)
#      define SOLE_LINUX_ANDROID
#      define SOLE_LINUX
#   else
#      include <arpa/inet.h>
#      include <ifaddrs.h>
#      include <net/if.h>
#      include <netinet/in.h>
#      include <sys/ioctl.h>
#      include <sys/socket.h>
#      include <sys/time.h>
#      include <unistd.h>
#      define SOLE_LINUX
#   endif
#else // elif defined(__unix__)
#   if defined(__VMS)
#      include <ioctl.h>
#      include <inet.h>
#   else
#      include <sys/ioctl.h>
#      include <arpa/inet.h>
#   endif
#   if defined(sun) || defined(__sun)
#      include <sys/sockio.h>
#   endif
#   include <net/if.h>
#   include <net/if_arp.h>
#   include <netdb.h>
#   include <netinet/in.h>
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <sys/types.h>
#   include <unistd.h>
#   define SOLE_UNIX
#endif

////////////////////////////////////////////////////////////////////////////////////

namespace sole {

std::string uuid::str() const
{
    char uustr[] = "00000000-0000-0000-0000-000000000000";
    constexpr char encode[] = "0123456789abcdef";

    size_t bit = 15;
    for (int i = 0; i < 18; i++)
    {
        if (i == 8 || i == 13)
        {
            continue;
        }
        uustr[i] = encode[ab >> 4 * bit & 0x0f];
        bit--;
    }

    bit = 15;
    for (int i = 18; i < 36; i++)
    {
        if (i == 18 || i == 23)
        {
            continue;
        }
        uustr[i] = encode[cd >> 4 * bit & 0x0f];
        bit--;
    }

    return std::string(uustr);
}

//////////////////////////////////////////////////////////////////////////////////////
// multiplatform clock_gettime()

#if defined(SOLE_WINDOWS)
struct timespec
{
    uint64_t tv_sec;
    uint64_t tv_nsec;
};

int gettimeofday(struct timeval* tv, struct timezone*)
{
    FILETIME ft;
    uint64_t tmpres = 0;

    if (tv != NULL)
    {
        GetSystemTimeAsFileTime(&ft);

        // The GetSystemTimeAsFileTime returns the number of 100 nanosecond
        // intervals since Jan 1, 1601 in a structure. Copy the high bits to
        // the 64 bit tmpres, shift it left by 32 then or in the low 32 bits
        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        // Convert to microseconds by dividing by 10
        tmpres /= 10;

        // The Unix epoch starts on Jan 1 1970. Need to subtract the difference
        // in seconds from Jan 1 1601
        tmpres -= 11644473600000000ULL;

        // Finally change microseconds to seconds and place in the seconds value.
        // The modulus picks up the microseconds
        tv->tv_sec = static_cast<long>(tmpres / 1000000UL);
        tv->tv_usec = (tmpres % 1000000UL);
    }

    return 0;
}
#endif

#if defined(SOLE_WINDOWS) || defined(SOLE_APPLE_OSX)
int clock_gettime(int /*clk_id*/, struct timespec* t)
{
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv)
        return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////
// Timestamp and MAC interfaces

// Returns number of 100ns intervals
uint64_t get_time(uint64_t offset)
{
    struct timespec tp;
    clock_gettime(0 /*CLOCK_REALTIME*/, &tp);

    // Convert to 100-nanosecond intervals
    uint64_t uuid_time;
    uuid_time = tp.tv_sec * 10000000;
    uuid_time = uuid_time + (tp.tv_nsec / 100);

    // If the clock looks like it went backwards, or is the same, increment it
    static uint64_t last_uuid_time = 0;
    if (last_uuid_time >= uuid_time)
        uuid_time = ++last_uuid_time;
    else
        last_uuid_time = uuid_time;

    uuid_time = uuid_time + offset;

    return uuid_time;
}

// Looks for first MAC address of any network device, any size
bool get_any_mac(std::vector<unsigned char>& _node)
{
#if defined(SOLE_WINDOWS)
    {
        PIP_ADAPTER_INFO pAdapterInfo = NULL;
        PIP_ADAPTER_INFO pAdapter = NULL;
        ULONG len    = sizeof(IP_ADAPTER_INFO);
        pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new char[len]);

        // Make an initial call to GetAdaptersInfo to get the necessary size into len
        DWORD rc = GetAdaptersInfo(pAdapterInfo, &len);
        if (rc == ERROR_BUFFER_OVERFLOW)
        {
            delete[] reinterpret_cast<char*>(pAdapterInfo);
            pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new char[len]);
        }
        else if (rc != ERROR_SUCCESS)
        {
            delete[] reinterpret_cast<char*>(pAdapterInfo);
            return false;
        }

        bool found = false;
        if (GetAdaptersInfo(pAdapterInfo, &len) == NO_ERROR)
        {
            pAdapter = pAdapterInfo;
            while (pAdapter && !found)
            {
                if (pAdapter->Type == MIB_IF_TYPE_ETHERNET && pAdapter->AddressLength > 0)
                {
                    _node.resize(pAdapter->AddressLength);
                    std::memcpy(_node.data(), pAdapter->Address, _node.size());
                    found = true;
                }
                pAdapter = pAdapter->Next;
            }
        }

        delete[] reinterpret_cast<char*>(pAdapterInfo);
        return found;
    }
#endif

#if defined(SOLE_BSD)
    {
        struct ifaddrs* ifaphead = NULL;
        int rc = getifaddrs(&ifaphead);
        if (rc)
            return false;

        bool foundAdapter = false;
        for (struct ifaddrs* ifap = ifaphead; ifap; ifap = ifap->ifa_next)
        {
            if (ifap->ifa_addr && ifap->ifa_addr->sa_family == AF_LINK)
            {
                struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(ifap->ifa_addr);
                caddr_t ap = (caddr_t)(sdl->sdl_data + sdl->sdl_nlen);
                int alen = sdl->sdl_alen;
                if (ap && alen > 0)
                {
                    _node.resize(alen);
                    std::memcpy(_node.data(), ap, _node.size());
                    foundAdapter = true;
                    break;
                }
            }
        }

        freeifaddrs(ifaphead);
        return foundAdapter;
    }
#endif

#if defined(SOLE_APPLE_OSX)
    {
        struct ifaddrs* ifaphead = NULL;
        int rc = getifaddrs(&ifaphead);
        if (rc)
            return false;

        bool foundAdapter = false;
        for (struct ifaddrs* ifap = ifaphead; ifap; ifap = ifap->ifa_next)
        {
            if (ifap->ifa_addr && ifap->ifa_addr->sa_family == AF_LINK)
            {
                struct sockaddr_dl* sdl = reinterpret_cast<struct sockaddr_dl*>(ifap->ifa_addr);
                caddr_t ap = (caddr_t)(sdl->sdl_data + sdl->sdl_nlen);
                int alen = sdl->sdl_alen;
                if (ap && alen > 0)
                {
                    _node.resize(alen);
                    std::memcpy(_node.data(), ap, _node.size());
                    foundAdapter = true;
                    break;
                }
            }
        }

        freeifaddrs(ifaphead);
        return foundAdapter;
    }
#endif

#if defined(SOLE_LINUX) && !defined(SOLE_LINUX_ANDROID)
    {
        struct ifaddrs* ifaphead = NULL;
        if (getifaddrs(&ifaphead) == -1)
            return false;

        bool foundAdapter = false;
        for (struct ifaddrs* ifap = ifaphead; ifap; ifap = ifap->ifa_next)
        {
            struct ifreq ifr;
            int s = socket(PF_INET, SOCK_DGRAM, 0);
            if (s == -1)
                continue;

            if (std::strcmp("lo", ifap->ifa_name) == 0) // loopback address is zero
            {
                close(s);
                continue;
            }

            std::strcpy(ifr.ifr_name, ifap->ifa_name);
            int rc = ioctl(s, SIOCGIFHWADDR, &ifr);
            close(s);
            if (rc < 0)
                continue;
            struct sockaddr* sa = reinterpret_cast<struct sockaddr*>(&ifr.ifr_addr);
            _node.resize(sizeof(sa->sa_data));
            std::memcpy(_node.data(), sa->sa_data, _node.size());
            foundAdapter = true;
            break;
        }

        freeifaddrs(ifaphead);
        return foundAdapter;
    }
#endif

#if defined(SOLE_UNIX)
    {
        char name[HOST_NAME_MAX];
        if (gethostname(name, sizeof(name)))
            return false;

        struct hostent* pHost = gethostbyname(name);
        if (!pHost)
            return false;

        int s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (s == -1)
            return false;

        struct arpreq ar;
        std::memset(&ar, 0, sizeof(ar));
        struct sockaddr_in* pAddr = reinterpret_cast<struct sockaddr_in*>(&ar.arp_pa);
        pAddr->sin_family = AF_INET;
        std::memcpy(&pAddr->sin_addr, *pHost->h_addr_list, sizeof(struct in_addr));
        int rc = ioctl(s, SIOCGARP, &ar);
        close(s);
        if (rc < 0)
            return false;
        _node.resize(sizeof(ar.arp_ha.sa_data));
        std::memcpy(_node.data(), ar.arp_ha.sa_data, _node.size());
        return true;
    }
#endif

    return false;
}

// Looks for first MAC address of any network device, size truncated to 48bits
uint64_t get_any_mac48()
{
    std::vector<unsigned char> node;
    if (get_any_mac(node))
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        node.resize(6);
        for (int i = 0; i < 6; ++i)
            ss << std::setw(2) << int(node[i]);
        uint64_t t;
        if (ss >> t)
            return t;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
// UUID implementations

uuid uuid4()
{
    static thread_local std::random_device rd;
    static thread_local std::uniform_int_distribution<uint64_t> dist(0, (uint64_t)(~0));

    uuid my;

    my.ab = dist(rd);
    my.cd = dist(rd);

    my.ab = (my.ab & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    my.cd = (my.cd & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    return my;
}

uuid uuid1()
{
    // UUID UTC base time is October 15 1582, Unix UTC base time is January 01 1970
    uint64_t time_offset = 0x01b21dd213814000ULL;

    // Number of 100-ns intervals since 00:00:00.00 15 October 1582; [ref] uuid.py
    uint64_t ns100_intervals = get_time(time_offset);
    uint16_t clock_seq = (uint16_t)(ns100_intervals & 0x3fff); // 14-bits max
    uint64_t mac = get_any_mac48();                            // 48-bits max

    if (mac == 0)
    {
        return uuid4();
    }

    uint32_t time_low = ns100_intervals & 0xffffffff;
    uint16_t time_mid = (ns100_intervals >> 32) & 0xffff;
    uint16_t time_hi_version = (ns100_intervals >> 48) & 0x0fff;
    uint8_t clock_seq_low = clock_seq & 0xff;
    uint8_t clock_seq_hi_variant = (clock_seq >> 8) & 0x3f;

    uuid u;
    uint64_t& upper_ = u.ab;
    uint64_t& lower_ = u.cd;

    // Build the high 32 bytes
    upper_  = (uint64_t)time_low << 32;
    upper_ |= (uint64_t)time_mid << 16;
    upper_ |= (uint64_t)time_hi_version;

    // Build the low 32 bytes, using the clock sequence number
    lower_  = (uint64_t)((clock_seq_hi_variant << 8) | clock_seq_low) << 48;
    lower_ |= mac;

    // Set the variant to RFC 4122
    lower_ &= ~((uint64_t)0xc000 << 48);
    lower_ |=   (uint64_t)0x8000 << 48;

    // Set the version number
    enum { version = 1 };
    upper_ &= ~0xf000;
    upper_ |= version << 12;

    return u;
}

} // ::sole
