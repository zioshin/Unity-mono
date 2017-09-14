#include <config.h>
#include "mbedtls.h"
#include "mbedtls/ssl.h"
#include "mbedtls/compat-1.3.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

void
unity_mbedtls_entropy_init (mbedtls_entropy_context *ctx)
{
	 mbedtls_entropy_init (ctx);
}

void
unity_mbedtls_entropy_free (mbedtls_entropy_context *ctx)
{
	mbedtls_entropy_free (ctx);
}

void
unity_mbedtls_ssl_init (mbedtls_ssl_context *ssl)
{
	mbedtls_ssl_init (ssl);
}

void
unity_mbedtls_ssl_free (mbedtls_ssl_context *ssl)
{
	mbedtls_ssl_free (ssl);
}

void
unity_mbedtls_ssl_config_init (mbedtls_ssl_config *conf)
{
	mbedtls_ssl_config_init(conf);
}

void
unity_mbedtls_ssl_config_free (mbedtls_ssl_config *conf)
{
	mbedtls_ssl_config_free (conf);
}

void
unity_mbedtls_ctr_drbg_init (mbedtls_ctr_drbg_context *ctx)
{
	mbedtls_ctr_drbg_init (ctx);
}

void
unity_mbedtls_ctr_drbg_free (mbedtls_ctr_drbg_context *ctx)
{
	mbedtls_ctr_drbg_free (ctx);
}

int
unity_mbedtls_ctr_drbg_random (void *p_rng, unsigned char *output, size_t output_len)
{
	return mbedtls_ctr_drbg_random (p_rng, output, output_len);
}

int
unity_mbedtls_ssl_config_defaults (mbedtls_ssl_config *conf, int endpoint, int transport, int preset)
{
	return mbedtls_ssl_config_defaults (conf, endpoint, transport, preset);
}

void
unity_mbedtls_ssl_conf_rng (mbedtls_ssl_config *conf, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
	mbedtls_ssl_conf_rng (conf, f_rng, p_rng);
}

int
unity_mbedtls_ctr_drbg_seed (mbedtls_ctr_drbg_context *ctx,
                   int (*f_entropy)(void *, unsigned char *, size_t),
                   void *p_entropy,
                   const unsigned char *custom,
                   size_t len)
{
	return mbedtls_ctr_drbg_seed(ctx, f_entropy, p_entropy, custom, len);
}

void
unity_mbedtls_ssl_set_bio (mbedtls_ssl_context *ssl,
                          void *p_bio,
                          mbedtls_ssl_send_t *f_send,
                          mbedtls_ssl_recv_t *f_recv,
                          mbedtls_ssl_recv_timeout_t *f_recv_timeout)
{
	mbedtls_ssl_set_bio (ssl, p_bio, f_send, f_recv, f_recv_timeout);
}

int
unity_mbedtls_entropy_func (void *data, unsigned char *output, size_t len)
{
	return mbedtls_entropy_func (data, output, len);
}

int
unity_mbedtls_ssl_setup (mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf)
{
	return mbedtls_ssl_setup (ssl, conf);
}

int
unity_mbedtls_ssl_handshake (mbedtls_ssl_context *ssl)
{
	return mbedtls_ssl_handshake(ssl);
}

int
unity_mbedtls_ssl_handshake_step (mbedtls_ssl_context *ssl)
{
	return mbedtls_ssl_handshake_step(ssl);
}

int
unity_mbedtls_ssl_set_hostname (mbedtls_ssl_context *ssl, const char *hostname)
{
	return mbedtls_ssl_set_hostname(ssl, hostname);
}

void
unity_mbedtls_ssl_conf_dbg( mbedtls_ssl_config *conf, void (*f_dbg)(void *, int, const char *, int, const char *), void  *p_dbg )
{
	mbedtls_ssl_conf_dbg (conf, f_dbg, p_dbg);
}

void
unity_mbedtls_debug_set_threshold (int threshold)
{
	mbedtls_debug_set_threshold(threshold);
}

void
unity_mbedtls_ssl_conf_authmode (mbedtls_ssl_config *conf, int authmode)
{
	mbedtls_ssl_conf_authmode(conf, authmode);
}

int
unity_mbedtls_ssl_read (mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
	return mbedtls_ssl_read (ssl, buf, len);
}

int
unity_mbedtls_ssl_write (mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
	return mbedtls_ssl_write (ssl, buf, len);
}

void
unity_mbedtls_strerror (int errnum, char *buffer, size_t buflen)
{
	mbedtls_strerror (errnum, buffer, buflen);
}

const mbedtls_x509_crt *
unity_mbedtls_ssl_get_peer_cert (const mbedtls_ssl_context *ssl)
{
	return mbedtls_ssl_get_peer_cert(ssl);
}

const char *
unity_mbedtls_ssl_get_ciphersuite (const mbedtls_ssl_context *ssl)
{
	return mbedtls_ssl_get_ciphersuite (ssl);
}

int
unity_mbedtls_ssl_get_ciphersuite_id (const char *ciphersuite_name)
{
	return mbedtls_ssl_get_ciphersuite_id (ciphersuite_name);
}

int
unity_mbedtls_ssl_conf_own_cert (mbedtls_ssl_config *conf, mbedtls_x509_crt *own_cert, mbedtls_pk_context *pk_key)
{
	return mbedtls_ssl_conf_own_cert (conf, own_cert, pk_key);
}

void
unity_mbedtls_ssl_conf_ca_chain (mbedtls_ssl_config *conf, mbedtls_x509_crt *ca_chain, mbedtls_x509_crl *ca_crl)
{
	mbedtls_ssl_conf_ca_chain (conf, ca_chain, ca_crl);
}

void
unity_mbedtls_ssl_conf_verify (mbedtls_ssl_config *conf, int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *), void *p_vrfy)
{
	mbedtls_ssl_conf_verify (conf, f_vrfy, p_vrfy);
}

void
unity_mbedtls_ssl_conf_min_version (mbedtls_ssl_config *conf, int major, int minor)
{
	mbedtls_ssl_conf_min_version (conf, major, minor);
}

void
unity_mbedtls_ssl_conf_max_version (mbedtls_ssl_config *conf, int major, int minor)
{
	mbedtls_ssl_conf_max_version (conf, major, minor);
}

void
unity_mbedtls_ssl_conf_ciphersuites (mbedtls_ssl_config *conf, const int *ciphersuites)
{
	mbedtls_ssl_conf_ciphersuites (conf, ciphersuites);
}

void
unity_mbedtls_x509_crt_init (mbedtls_x509_crt *crt)
{
	mbedtls_x509_crt_init (crt);
}

void
unity_mbedtls_x509_crt_free (mbedtls_x509_crt *crt)
{
	mbedtls_x509_crt_free (crt);
}

int
unity_mbedtls_x509_crt_parse (mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen)
{
	return mbedtls_x509_crt_parse (chain, buf, buflen);
}

void
unity_mbedtls_pk_init (mbedtls_pk_context *ctx)
{
	mbedtls_pk_init (ctx);
}

void
unity_mbedtls_pk_free (mbedtls_pk_context *ctx)
{
	mbedtls_pk_free (ctx);
}

int
unity_mbedtls_pk_parse_key (mbedtls_pk_context *ctx, const unsigned char *key, size_t keylen, const unsigned char *pwd, size_t pwdlen)
{
	return mbedtls_pk_parse_key (ctx, key, keylen, pwd, pwdlen);
}

int
unity_mbedtls_x509_crt_verify (mbedtls_x509_crt *crt,
                     mbedtls_x509_crt *trust_ca,
                     mbedtls_x509_crl *ca_crl,
                     const char *cn, uint32_t *flags,
                     int (*f_vrfy)(void *, mbedtls_x509_crt *, int, uint32_t *),
                     void *p_vrfy)
{
	return mbedtls_x509_crt_verify (crt, trust_ca, ca_crl, cn, flags, f_vrfy, p_vrfy);
}
