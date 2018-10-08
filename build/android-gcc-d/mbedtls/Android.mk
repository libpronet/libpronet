LOCAL_PATH := $(PRO_ROOT_DIR)/src/mbedtls/library
include $(CLEAR_VARS)

LOCAL_MODULE    := mbedtls
LOCAL_SRC_FILES := aes.c                 \
                   aesni.c               \
                   arc4.c                \
                   aria.c                \
                   asn1parse.c           \
                   asn1write.c           \
                   base64.c              \
                   bignum.c              \
                   blowfish.c            \
                   camellia.c            \
                   ccm.c                 \
                   certs.c               \
                   chacha20.c            \
                   chachapoly.c          \
                   cipher.c              \
                   cipher_wrap.c         \
                   cmac.c                \
                   ctr_drbg.c            \
                   debug.c               \
                   des.c                 \
                   dhm.c                 \
                   ecdh.c                \
                   ecdsa.c               \
                   ecjpake.c             \
                   ecp.c                 \
                   ecp_curves.c          \
                   entropy.c             \
                   entropy_poll.c        \
                   error.c               \
                   gcm.c                 \
                   havege.c              \
                   hkdf.c                \
                   hmac_drbg.c           \
                   md.c                  \
                   md2.c                 \
                   md4.c                 \
                   md5.c                 \
                   md_wrap.c             \
                   memory_buffer_alloc.c \
                   net_sockets.c         \
                   nist_kw.c             \
                   oid.c                 \
                   padlock.c             \
                   pem.c                 \
                   pk.c                  \
                   pk_wrap.c             \
                   pkcs11.c              \
                   pkcs12.c              \
                   pkcs5.c               \
                   pkparse.c             \
                   pkwrite.c             \
                   platform.c            \
                   platform_util.c       \
                   poly1305.c            \
                   ripemd160.c           \
                   rsa.c                 \
                   rsa_internal.c        \
                   sha1.c                \
                   sha256.c              \
                   sha512.c              \
                   ssl_cache.c           \
                   ssl_ciphersuites.c    \
                   ssl_cli.c             \
                   ssl_cookie.c          \
                   ssl_srv.c             \
                   ssl_ticket.c          \
                   ssl_tls.c             \
                   threading.c           \
                   timing.c              \
                   version.c             \
                   version_features.c    \
                   x509.c                \
                   x509_create.c         \
                   x509_crl.c            \
                   x509_crt.c            \
                   x509_csr.c            \
                   x509write_crt.c       \
                   x509write_csr.c       \
                   xtea.c

LOCAL_C_INCLUDES    := $(PRO_ROOT_DIR)/src/mbedtls/include
LOCAL_CFLAGS        := -fno-strict-aliasing -fvisibility=hidden
LOCAL_CPPFLAGS      :=
LOCAL_CPP_EXTENSION := .cpp .cxx .cc

LOCAL_LDFLAGS                :=
LOCAL_LDLIBS                 :=
LOCAL_STATIC_LIBRARIES       :=
LOCAL_WHOLE_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES       :=

include $(BUILD_STATIC_LIBRARY)
