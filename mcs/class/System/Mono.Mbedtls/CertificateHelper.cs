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

namespace Mono.Mbedtls
{
	class CertificateHelper
	{
		static NativeBuffer nativeBuffer;
		static CertificateHelper()
		{
			nativeBuffer = new NativeBuffer();
		}

		enum CertDataFormat
		{
			DATATYPE_STRING = 0,
			DATATYPE_INTPTR = 1
		}

		[DllImport("__Internal")]
		static extern bool EnumSystemCertificates(IntPtr certStore, ref IntPtr iter, out CertDataFormat format, out int size, out IntPtr data);
		[DllImport("__Internal")]
		static extern IntPtr OpenSystemRootStore();
		[DllImport("__Internal")]
		static extern void CloseSystemRootStore(IntPtr cStore);

		public static void AddSystemCertificates(ref Mbedtls.mbedtls_x509_crt chain)
		{
			IntPtr store = OpenSystemRootStore();
			CertDataFormat format;
			int size;
			IntPtr data;
			IntPtr iter = IntPtr.Zero;
			while (EnumSystemCertificates(store, ref iter, out format, out size, out data)) {
				// TODO: assert format == DATATYPE_INTPTR
				AddToChain(ref chain, data, size);
			}

			CloseSystemRootStore(store);
		}

		public static void AddToChain (ref Mbedtls.mbedtls_x509_crt chain, X509Certificate crt)
		{
			if (crt == null)
				return;
			lock (nativeBuffer)
			{
				int crtLength = nativeBuffer.ToNative (crt.GetRawCertData ());
				AddToChain (ref chain, nativeBuffer.DataPtr, crtLength);
			}
		}

		public static void AddToChain (ref Mbedtls.mbedtls_x509_crt chain, string crt)
		{
			if (crt == null)
				return;
			lock (nativeBuffer)
			{
				int crtLength = nativeBuffer.ToNative (crt);
				AddToChain (ref chain, nativeBuffer.DataPtr, crtLength);
			}
		}

		public static void AddToChain (ref Mbedtls.mbedtls_x509_crt chain, IntPtr crt, size_t crtLength)
		{
			if (crt == IntPtr.Zero)
				return;
			Mbedtls.mbedtls_x509_crt_parse (ref chain, crt, crtLength);
		}

		public static void SetPrivateKey (ref Mbedtls.mbedtls_pk_context ctx, X509Certificate crt)
		{
			X509Certificate2 crt2 = crt as X509Certificate2;
			if (crt2 == null)
				return;

			SetPrivateKey (ref ctx, crt2.PrivateKey);
		}

		public static void SetPrivateKey (ref Mbedtls.mbedtls_pk_context ctx, AsymmetricAlgorithm key)
		{
			if (key == null)
				return;

			int keyLength, result;
			lock (nativeBuffer)
			{
				keyLength = nativeBuffer.ToNative (PKCS8.PrivateKeyInfo.Encode (key));
				result = Mbedtls.mbedtls_pk_parse_key (ref ctx, nativeBuffer.DataPtr, keyLength, IntPtr.Zero, 0);
			}
			Debug.CheckAndThrow (result, "Unable to parse private key");
		}

		public static X509Certificate2 AsX509 (IntPtr mbedtls_x509_crt)
		{
			if (mbedtls_x509_crt == IntPtr.Zero)
				return null;

			return AsX509 (Marshal.PtrToStructure<Mbedtls.mbedtls_x509_buf> (mbedtls_x509_crt));
		}

		public static X509Certificate2 AsX509 (ref Mbedtls.mbedtls_x509_crt crt)
		{
			return AsX509 (crt.raw);
		}


		public static X509Certificate2 AsX509 (Mbedtls.mbedtls_x509_buf crt)
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