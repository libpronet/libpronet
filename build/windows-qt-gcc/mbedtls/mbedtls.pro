#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

QT       -= core gui

TARGET = mbedtls
TEMPLATE = lib
CONFIG += staticlib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG(debug  , debug | release) : DEFINES += _DEBUG _WIN32_WINNT=0x0501
CONFIG(release, debug | release) : DEFINES += NDEBUG _WIN32_WINNT=0x0501

INCLUDEPATH += ../../../src/mbedtls/include

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../../src/mbedtls/library/aes.c \
    ../../../src/mbedtls/library/aesni.c \
    ../../../src/mbedtls/library/arc4.c \
    ../../../src/mbedtls/library/aria.c \
    ../../../src/mbedtls/library/asn1parse.c \
    ../../../src/mbedtls/library/asn1write.c \
    ../../../src/mbedtls/library/base64.c \
    ../../../src/mbedtls/library/bignum.c \
    ../../../src/mbedtls/library/blowfish.c \
    ../../../src/mbedtls/library/camellia.c \
    ../../../src/mbedtls/library/ccm.c \
    ../../../src/mbedtls/library/certs.c \
    ../../../src/mbedtls/library/chacha20.c \
    ../../../src/mbedtls/library/chachapoly.c \
    ../../../src/mbedtls/library/cipher.c \
    ../../../src/mbedtls/library/cipher_wrap.c \
    ../../../src/mbedtls/library/cmac.c \
    ../../../src/mbedtls/library/constant_time.c \
    ../../../src/mbedtls/library/ctr_drbg.c \
    ../../../src/mbedtls/library/debug.c \
    ../../../src/mbedtls/library/des.c \
    ../../../src/mbedtls/library/dhm.c \
    ../../../src/mbedtls/library/ecdh.c \
    ../../../src/mbedtls/library/ecdsa.c \
    ../../../src/mbedtls/library/ecjpake.c \
    ../../../src/mbedtls/library/ecp.c \
    ../../../src/mbedtls/library/ecp_curves.c \
    ../../../src/mbedtls/library/entropy.c \
    ../../../src/mbedtls/library/entropy_poll.c \
    ../../../src/mbedtls/library/error.c \
    ../../../src/mbedtls/library/gcm.c \
    ../../../src/mbedtls/library/havege.c \
    ../../../src/mbedtls/library/hkdf.c \
    ../../../src/mbedtls/library/hmac_drbg.c \
    ../../../src/mbedtls/library/md2.c \
    ../../../src/mbedtls/library/md4.c \
    ../../../src/mbedtls/library/md5.c \
    ../../../src/mbedtls/library/md.c \
    ../../../src/mbedtls/library/memory_buffer_alloc.c \
    ../../../src/mbedtls/library/mps_reader.c \
    ../../../src/mbedtls/library/mps_trace.c \
    ../../../src/mbedtls/library/net_sockets.c \
    ../../../src/mbedtls/library/nist_kw.c \
    ../../../src/mbedtls/library/oid.c \
    ../../../src/mbedtls/library/padlock.c \
    ../../../src/mbedtls/library/pem.c \
    ../../../src/mbedtls/library/pk.c \
    ../../../src/mbedtls/library/pk_wrap.c \
    ../../../src/mbedtls/library/pkcs5.c \
    ../../../src/mbedtls/library/pkcs11.c \
    ../../../src/mbedtls/library/pkcs12.c \
    ../../../src/mbedtls/library/pkparse.c \
    ../../../src/mbedtls/library/pkwrite.c \
    ../../../src/mbedtls/library/platform.c \
    ../../../src/mbedtls/library/platform_util.c \
    ../../../src/mbedtls/library/poly1305.c \
    ../../../src/mbedtls/library/psa_crypto.c \
    ../../../src/mbedtls/library/psa_crypto_aead.c \
    ../../../src/mbedtls/library/psa_crypto_cipher.c \
    ../../../src/mbedtls/library/psa_crypto_client.c \
    ../../../src/mbedtls/library/psa_crypto_driver_wrappers.c \
    ../../../src/mbedtls/library/psa_crypto_ecp.c \
    ../../../src/mbedtls/library/psa_crypto_hash.c \
    ../../../src/mbedtls/library/psa_crypto_mac.c \
    ../../../src/mbedtls/library/psa_crypto_rsa.c \
    ../../../src/mbedtls/library/psa_crypto_se.c \
    ../../../src/mbedtls/library/psa_crypto_slot_management.c \
    ../../../src/mbedtls/library/psa_crypto_storage.c \
    ../../../src/mbedtls/library/psa_its_file.c \
    ../../../src/mbedtls/library/ripemd160.c \
    ../../../src/mbedtls/library/rsa.c \
    ../../../src/mbedtls/library/rsa_internal.c \
    ../../../src/mbedtls/library/sha1.c \
    ../../../src/mbedtls/library/sha256.c \
    ../../../src/mbedtls/library/sha512.c \
    ../../../src/mbedtls/library/ssl_cache.c \
    ../../../src/mbedtls/library/ssl_ciphersuites.c \
    ../../../src/mbedtls/library/ssl_cli.c \
    ../../../src/mbedtls/library/ssl_cookie.c \
    ../../../src/mbedtls/library/ssl_msg.c \
    ../../../src/mbedtls/library/ssl_srv.c \
    ../../../src/mbedtls/library/ssl_ticket.c \
    ../../../src/mbedtls/library/ssl_tls.c \
    ../../../src/mbedtls/library/ssl_tls13_keys.c \
    ../../../src/mbedtls/library/threading.c \
    ../../../src/mbedtls/library/timing.c \
    ../../../src/mbedtls/library/version.c \
    ../../../src/mbedtls/library/version_features.c \
    ../../../src/mbedtls/library/x509.c \
    ../../../src/mbedtls/library/x509_create.c \
    ../../../src/mbedtls/library/x509_crl.c \
    ../../../src/mbedtls/library/x509_crt.c \
    ../../../src/mbedtls/library/x509_csr.c \
    ../../../src/mbedtls/library/x509write_crt.c \
    ../../../src/mbedtls/library/x509write_csr.c \
    ../../../src/mbedtls/library/xtea.c

HEADERS += \
    ../../../src/mbedtls/include/mbedtls/aes.h \
    ../../../src/mbedtls/include/mbedtls/aesni.h \
    ../../../src/mbedtls/include/mbedtls/arc4.h \
    ../../../src/mbedtls/include/mbedtls/aria.h \
    ../../../src/mbedtls/include/mbedtls/asn1.h \
    ../../../src/mbedtls/include/mbedtls/asn1write.h \
    ../../../src/mbedtls/include/mbedtls/base64.h \
    ../../../src/mbedtls/include/mbedtls/bignum.h \
    ../../../src/mbedtls/include/mbedtls/blowfish.h \
    ../../../src/mbedtls/include/mbedtls/bn_mul.h \
    ../../../src/mbedtls/include/mbedtls/camellia.h \
    ../../../src/mbedtls/include/mbedtls/ccm.h \
    ../../../src/mbedtls/include/mbedtls/certs.h \
    ../../../src/mbedtls/include/mbedtls/chacha20.h \
    ../../../src/mbedtls/include/mbedtls/chachapoly.h \
    ../../../src/mbedtls/include/mbedtls/check_config.h \
    ../../../src/mbedtls/include/mbedtls/cipher.h \
    ../../../src/mbedtls/include/mbedtls/cipher_internal.h \
    ../../../src/mbedtls/include/mbedtls/cmac.h \
    ../../../src/mbedtls/include/mbedtls/compat-1.3.h \
    ../../../src/mbedtls/include/mbedtls/config.h \
    ../../../src/mbedtls/include/mbedtls/config_psa.h \
    ../../../src/mbedtls/include/mbedtls/constant_time.h \
    ../../../src/mbedtls/include/mbedtls/ctr_drbg.h \
    ../../../src/mbedtls/include/mbedtls/debug.h \
    ../../../src/mbedtls/include/mbedtls/des.h \
    ../../../src/mbedtls/include/mbedtls/dhm.h \
    ../../../src/mbedtls/include/mbedtls/ecdh.h \
    ../../../src/mbedtls/include/mbedtls/ecdsa.h \
    ../../../src/mbedtls/include/mbedtls/ecjpake.h \
    ../../../src/mbedtls/include/mbedtls/ecp.h \
    ../../../src/mbedtls/include/mbedtls/ecp_internal.h \
    ../../../src/mbedtls/include/mbedtls/entropy.h \
    ../../../src/mbedtls/include/mbedtls/entropy_poll.h \
    ../../../src/mbedtls/include/mbedtls/error.h \
    ../../../src/mbedtls/include/mbedtls/gcm.h \
    ../../../src/mbedtls/include/mbedtls/havege.h \
    ../../../src/mbedtls/include/mbedtls/hkdf.h \
    ../../../src/mbedtls/include/mbedtls/hmac_drbg.h \
    ../../../src/mbedtls/include/mbedtls/md.h \
    ../../../src/mbedtls/include/mbedtls/md2.h \
    ../../../src/mbedtls/include/mbedtls/md4.h \
    ../../../src/mbedtls/include/mbedtls/md5.h \
    ../../../src/mbedtls/include/mbedtls/md_internal.h \
    ../../../src/mbedtls/include/mbedtls/memory_buffer_alloc.h \
    ../../../src/mbedtls/include/mbedtls/net.h \
    ../../../src/mbedtls/include/mbedtls/net_sockets.h \
    ../../../src/mbedtls/include/mbedtls/nist_kw.h \
    ../../../src/mbedtls/include/mbedtls/oid.h \
    ../../../src/mbedtls/include/mbedtls/padlock.h \
    ../../../src/mbedtls/include/mbedtls/pem.h \
    ../../../src/mbedtls/include/mbedtls/pk.h \
    ../../../src/mbedtls/include/mbedtls/pk_internal.h \
    ../../../src/mbedtls/include/mbedtls/pkcs11.h \
    ../../../src/mbedtls/include/mbedtls/pkcs12.h \
    ../../../src/mbedtls/include/mbedtls/pkcs5.h \
    ../../../src/mbedtls/include/mbedtls/platform.h \
    ../../../src/mbedtls/include/mbedtls/platform_time.h \
    ../../../src/mbedtls/include/mbedtls/platform_util.h \
    ../../../src/mbedtls/include/mbedtls/poly1305.h \
    ../../../src/mbedtls/include/mbedtls/psa_util.h \
    ../../../src/mbedtls/include/mbedtls/ripemd160.h \
    ../../../src/mbedtls/include/mbedtls/rsa.h \
    ../../../src/mbedtls/include/mbedtls/rsa_internal.h \
    ../../../src/mbedtls/include/mbedtls/sha1.h \
    ../../../src/mbedtls/include/mbedtls/sha256.h \
    ../../../src/mbedtls/include/mbedtls/sha512.h \
    ../../../src/mbedtls/include/mbedtls/ssl.h \
    ../../../src/mbedtls/include/mbedtls/ssl_cache.h \
    ../../../src/mbedtls/include/mbedtls/ssl_ciphersuites.h \
    ../../../src/mbedtls/include/mbedtls/ssl_cookie.h \
    ../../../src/mbedtls/include/mbedtls/ssl_internal.h \
    ../../../src/mbedtls/include/mbedtls/ssl_ticket.h \
    ../../../src/mbedtls/include/mbedtls/threading.h \
    ../../../src/mbedtls/include/mbedtls/threading_alt.h \
    ../../../src/mbedtls/include/mbedtls/timing.h \
    ../../../src/mbedtls/include/mbedtls/version.h \
    ../../../src/mbedtls/include/mbedtls/x509.h \
    ../../../src/mbedtls/include/mbedtls/x509_crl.h \
    ../../../src/mbedtls/include/mbedtls/x509_crt.h \
    ../../../src/mbedtls/include/mbedtls/x509_csr.h \
    ../../../src/mbedtls/include/mbedtls/xtea.h \
    ../../../src/mbedtls/include/psa/crypto.h \
    ../../../src/mbedtls/include/psa/crypto_builtin_composites.h \
    ../../../src/mbedtls/include/psa/crypto_builtin_primitives.h \
    ../../../src/mbedtls/include/psa/crypto_compat.h \
    ../../../src/mbedtls/include/psa/crypto_config.h \
    ../../../src/mbedtls/include/psa/crypto_driver_common.h \
    ../../../src/mbedtls/include/psa/crypto_driver_contexts_composites.h \
    ../../../src/mbedtls/include/psa/crypto_driver_contexts_primitives.h \
    ../../../src/mbedtls/include/psa/crypto_extra.h \
    ../../../src/mbedtls/include/psa/crypto_platform.h \
    ../../../src/mbedtls/include/psa/crypto_se_driver.h \
    ../../../src/mbedtls/include/psa/crypto_sizes.h \
    ../../../src/mbedtls/include/psa/crypto_struct.h \
    ../../../src/mbedtls/include/psa/crypto_types.h \
    ../../../src/mbedtls/include/psa/crypto_values.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
