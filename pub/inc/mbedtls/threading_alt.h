#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

#if defined(__cplusplus)
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
////

typedef struct mbedtls_threading_mutex_t
{
    void* mutex;
} mbedtls_threading_mutex_t;

/////////////////////////////////////////////////////////////////////////////
////

#if defined(__cplusplus)
}
#endif

#endif /* MBEDTLS_THREADING_ALT_H */
