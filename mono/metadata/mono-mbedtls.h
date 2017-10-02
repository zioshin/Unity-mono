#include "mbedtls/ssl.h"
#include "mbedtls/compat-1.3.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

mbedtls_entropy_context* unity_mbedtls_entropy_init ();
void unity_mbedtls_entropy_free (mbedtls_entropy_context *ctx);
mbedtls_ssl_context* unity_mbedtls_ssl_init ();
void unity_mbedtls_ssl_free (mbedtls_ssl_context *ssl);
mbedtls_ssl_config* unity_mbedtls_ssl_config_init ();
void unity_mbedtls_ssl_config_free (mbedtls_ssl_config *conf);
mbedtls_ctr_drbg_context* unity_mbedtls_ctr_drbg_init ();
void unity_mbedtls_ctr_drbg_free (mbedtls_ctr_drbg_context *ctx);
int unity_mbedtls_ssl_config_defaults (mbedtls_ssl_config *conf, int endpoint, int transport, int preset);
void unity_mbedtls_ssl_conf_rng (mbedtls_ssl_config *conf, void *p_rng);
int unity_mbedtls_ctr_drbg_seed (mbedtls_ctr_drbg_context *ctx,
                   void *p_entropy,
                   const unsigned char *custom,
                   size_t len);
void unity_mbedtls_ssl_set_bio (mbedtls_ssl_context *ssl,
                          void *p_bio,
                          mbedtls_ssl_send_t *f_send,
                          mbedtls_ssl_recv_t *f_recv,
                          mbedtls_ssl_recv_timeout_t *f_recv_timeout);
int unity_mbedtls_ssl_setup (mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf);
int unity_mbedtls_ssl_handshake (mbedtls_ssl_context *ssl);
int unity_mbedtls_ssl_handshake_step (mbedtls_ssl_context *ssl);
int unity_mbedtls_ssl_set_hostname (mbedtls_ssl_context *ssl, const char *hostname);
void unity_mbedtls_ssl_conf_dbg( mbedtls_ssl_config *conf, void (*f_dbg)(void *, int, const char *, int, const char *), void  *p_dbg );
void unity_mbedtls_debug_set_threshold (int threshold);
void unity_mbedtls_ssl_conf_authmode (mbedtls_ssl_config *conf, int authmode);
int unity_mbedtls_ssl_read (mbedtls_ssl_context *ssl, unsigned char *buf, size_t len);
int unity_mbedtls_ssl_write (mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len);
void unity_mbedtls_strerror (int errnum, char *buffer, size_t buflen);
const mbedtls_x509_crt *unity_mbedtls_ssl_get_peer_cert (const mbedtls_ssl_context *ssl);
const char *unity_mbedtls_ssl_get_ciphersuite (const mbedtls_ssl_context *ssl);
int unity_mbedtls_ssl_get_ciphersuite_id (const char *ciphersuite_name);
int unity_mbedtls_ssl_conf_own_cert (mbedtls_ssl_config *conf, mbedtls_x509_crt *own_cert, mbedtls_pk_context *pk_key);
void unity_mbedtls_ssl_conf_ca_chain (mbedtls_ssl_config *conf, mbedtls_x509_crt *ca_chain, mbedtls_x509_crl *ca_crl);
void unity_mbedtls_ssl_conf_verify (mbedtls_ssl_config *conf, int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *), void *p_vrfy);
void unity_mbedtls_ssl_conf_min_version (mbedtls_ssl_config *conf, int major, int minor);
void unity_mbedtls_ssl_conf_max_version (mbedtls_ssl_config *conf, int major, int minor);
void unity_mbedtls_ssl_conf_ciphersuites (mbedtls_ssl_config *conf, const int *ciphersuites);
mbedtls_x509_crt* unity_mbedtls_x509_crt_init ();
void unity_mbedtls_x509_crt_free (mbedtls_x509_crt *crt);
int unity_mbedtls_x509_crt_parse (mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen);
int unity_mbedtls_x509_crt_parse_file (mbedtls_x509_crt *chain, const char *path);
mbedtls_pk_context* unity_mbedtls_pk_init ();
void unity_mbedtls_pk_free (mbedtls_pk_context *ctx);
int unity_mbedtls_pk_parse_key (mbedtls_pk_context *ctx, const unsigned char *key, size_t keylen, const unsigned char *pwd, size_t pwdlen);
int unity_mbedtls_x509_crt_verify (mbedtls_x509_crt *crt,
                     mbedtls_x509_crt *trust_ca,
                     mbedtls_x509_crl *ca_crl,
                     const char *cn, uint32_t *flags);

int unity_mbedtls_ssl_get_state (mbedtls_ssl_context* ssl);
int unity_mbedtls_ssl_get_minor_ver (mbedtls_ssl_context* ssl);

mbedtls_x509_crt* unity_mbedtls_x509_crt_get_next (mbedtls_x509_crt* crt);
mbedtls_x509_buf unity_mbedtls_x509_crt_get_raw (mbedtls_x509_crt* crt);

