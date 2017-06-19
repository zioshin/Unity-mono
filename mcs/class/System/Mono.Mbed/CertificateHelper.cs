#if SECURITY_DEP
#if MONO_SECURITY_ALIAS
extern alias MonoSecurity;
#endif

using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

#if MONO_SECURITY_ALIAS
using MonoSecurity::Mono.Security.Cryptography;
#else
using Mono.Security.Cryptography;
#endif

namespace Mono.Mbed
{
	class CertificateHelper
	{
		static string SSL_ROOT_CA = @"-----BEGIN CERTIFICATE-----
MIIDVDCCAjygAwIBAgIDAjRWMA0GCSqGSIb3DQEBBQUAMEIxCzAJBgNVBAYTAlVT
MRYwFAYDVQQKEw1HZW9UcnVzdCBJbmMuMRswGQYDVQQDExJHZW9UcnVzdCBHbG9i
YWwgQ0EwHhcNMDIwNTIxMDQwMDAwWhcNMjIwNTIxMDQwMDAwWjBCMQswCQYDVQQG
EwJVUzEWMBQGA1UEChMNR2VvVHJ1c3QgSW5jLjEbMBkGA1UEAxMSR2VvVHJ1c3Qg
R2xvYmFsIENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2swYYzD9
9BcjGlZ+W988bDjkcbd4kdS8odhM+KhDtgPpTSEHCIjaWC9mOSm9BXiLnTjoBbdq
fnGk5sRgprDvgOSJKA+eJdbtg/OtppHHmMlCGDUUna2YRpIuT8rxh0PBFpVXLVDv
iS2Aelet8u5fa9IAjbkU+BQVNdnARqN7csiRv8lVK83Qlz6cJmTM386DGXHKTubU
1XupGc1V3sjs0l44U+VcT4wt/lAjNvxm5suOpDkZALeVAjmRCw7+OC7RHQWa9k0+
bw8HHa8sHo9gOeL6NlMTOdReJivbPagUvTLrGAMoUgRx5aszPeE4uwc2hGKceeoW
MPRfwCvocWvk+QIDAQABo1MwUTAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQWBBTA
ephojYn7qwVkDBF9qn1luMrMTjAfBgNVHSMEGDAWgBTAephojYn7qwVkDBF9qn1l
uMrMTjANBgkqhkiG9w0BAQUFAAOCAQEANeMpauUvXVSOKVCUn5kaFOSPeCpilKIn
Z57QzxpeR+nBsqTP3UEaBU6bS+5Kb1VSsyShNwrrZHYqLizz/Tt1kL/6cdjHPTfS
tQWVYrmm3ok9Nns4d0iXrKYgjy6myQzCsplFAMfOEVEiIuCl6rYVSAlk6l5PdPcF
PseKUgzbFbS9bZvlxrFUaKnjaZC2mqUPuLk/IH2uSrW4nOQdtqvmlKXBx4Ot2/Un
hw4EbNX/3aBd7YdStysVAq45pmp06drE57xNNB6pXE0zX5IJL4hmXXeXxx12E6nV
5fEWCRE11azbJHFwLJhWC9kXtNHjUStedejV0NxPNO3CBWaAocvmMw==
-----END CERTIFICATE-----";

		static NativeBuffer nativeBuffer;
		static CertificateHelper()
		{
			nativeBuffer = new NativeBuffer();
		}

		public static void AddSystemCertificates(ref MonoMbedTlsAPI.mbedtls_x509_crt chain)
		{
			AddToChain(ref chain, SSL_ROOT_CA);
		}

		public static void AddToChain (ref MonoMbedTlsAPI.mbedtls_x509_crt chain, X509Certificate crt)
		{
			if (crt == null)
				return;
			lock (nativeBuffer)
			{
				int crtLength = nativeBuffer.ToNative (crt.GetRawCertData ());
				AddToChain (ref chain, nativeBuffer.DataPtr, crtLength);
			}
		}

		public static void AddToChain (ref MonoMbedTlsAPI.mbedtls_x509_crt chain, string crt)
		{
			if (crt == null)
				return;
			lock (nativeBuffer)
			{
				int crtLength = nativeBuffer.ToNative (crt);
				AddToChain (ref chain, nativeBuffer.DataPtr, crtLength);
			}
		}

		public static void AddToChain (ref MonoMbedTlsAPI.mbedtls_x509_crt chain, IntPtr crt, size_t crtLength)
		{
			if (crt == IntPtr.Zero)
				return;
			MonoMbedTlsAPI.mbedtls_x509_crt_parse (ref chain, crt, crtLength);
		}

		public static void SetPrivateKey (ref MonoMbedTlsAPI.mbedtls_pk_context ctx, X509Certificate crt)
		{
			X509Certificate2 crt2 = crt as X509Certificate2;
			if (crt2 == null)
				return;

			SetPrivateKey (ref ctx, crt2.PrivateKey);
		}

		public static void SetPrivateKey (ref MonoMbedTlsAPI.mbedtls_pk_context ctx, AsymmetricAlgorithm key)
		{
			if (key == null)
				return;

			int keyLength, result;
			lock (nativeBuffer)
			{
				keyLength = nativeBuffer.ToNative (PKCS8.PrivateKeyInfo.Encode (key));
				result = MonoMbedTlsAPI.mbedtls_pk_parse_key (ref ctx, nativeBuffer.DataPtr, keyLength, IntPtr.Zero, 0);
			}
			Debug.CheckAndThrow (result, "Unable to parse private key");
		}

		public static X509Certificate2 AsX509 (IntPtr mbedtls_x509_crt)
		{
			if (mbedtls_x509_crt == IntPtr.Zero)
				return null;

			return AsX509 (Marshal.PtrToStructure<MonoMbedTlsAPI.mbedtls_x509_buf> (mbedtls_x509_crt));
		}

		public static X509Certificate2 AsX509 (ref MonoMbedTlsAPI.mbedtls_x509_crt crt)
		{
			return AsX509 (crt.raw);
		}


		public static X509Certificate2 AsX509 (MonoMbedTlsAPI.mbedtls_x509_buf crt)
		{
			Debug.WriteLine (9, "X509 tag {0} len {1}", crt.tag, crt.len);
			Debug.WriteBlock (99, "X509 DER", crt.data, (int)crt.len);

			byte [] certificateData = new byte [(int)crt.len];
			Marshal.Copy (crt.data, certificateData, 0, (int)crt.len);
			return new X509Certificate2 (certificateData);
		}
	}
}
#endif