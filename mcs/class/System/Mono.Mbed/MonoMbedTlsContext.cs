#if SECURITY_DEP
#if MONO_SECURITY_ALIAS
extern alias MonoSecurity;
#endif

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Security.Authentication;

#if MONO_SECURITY_ALIAS
using MonoSecurity::Mono.Security.Interface;
#else
using Mono.Security.Interface;
#endif

using Mono.Net.Security;

namespace Mono.Mbed
{
	class MonoMbedTlsContext : MobileTlsContext
	{
		private const int MaxIOBufferSize = 16384;

		readonly static string SSL_SEED_STR = "MonoMbedTls";

		// Native structs
		MonoMbedTlsAPI.mbedtls_ssl_context       m_SslContext;
		MonoMbedTlsAPI.mbedtls_entropy_context   m_EntropyContext;
		MonoMbedTlsAPI.mbedtls_ctr_drbg_context  m_RandomGeneratorContext;
		MonoMbedTlsAPI.mbedtls_ssl_config        m_SslConfig;
		MonoMbedTlsAPI.mbedtls_x509_crt          m_RootCertificateChain;
		MonoMbedTlsAPI.mbedtls_x509_crt          m_OwnCertificateChain;
		MonoMbedTlsAPI.mbedtls_pk_context        m_OwnPrivateKeyContext;

		// Delegate references to guard against garbage collection
		MonoMbedTlsAPI.mbedtls_entropy_t         m_EntropyCallback;
		MonoMbedTlsAPI.mbedtls_ctr_drbg_random_t m_RandomCallback;
		MonoMbedTlsAPI.mbedtls_ssl_send_t        m_BIOWriteCallback;
		MonoMbedTlsAPI.mbedtls_ssl_recv_t        m_BIOReadCallback;
		MonoMbedTlsAPI.mbedtls_ssl_dbg_t         m_DebugCallback;
		MonoMbedTlsAPI.mbedtls_verify_t          m_VerifyCallback;

		// States and certificates
		X509Certificate       m_LocalClientCertificate;
		X509Certificate2      m_RemoteCertificate;
		MonoTlsConnectionInfo m_Connectioninfo;
		bool                  m_IsAuthenticated;

		NativeBuffer m_NativeBuffer;

		NativeBuffer m_NativeIOReadBuffer;
		NativeBuffer m_NativeIOWriteBuffer;

		byte [] m_ManagedBuffer = new byte[0];

		internal MonoMbedTlsContext (
		    MobileAuthenticatedStream parent,
		    bool serverMode, string targetHost,
		    SslProtocols enabledProtocols, X509Certificate serverCertificate,
		    X509CertificateCollection clientCertificates, bool askForClientCert)
		    : base (parent, serverMode, targetHost, enabledProtocols,
			    serverCertificate, clientCertificates, askForClientCert)
		{
			// Setup native buffers
			m_NativeBuffer        = new NativeBuffer();
			m_NativeIOReadBuffer  = new NativeBuffer();
			m_NativeIOWriteBuffer = new NativeBuffer();

			// Initialize native structs
			MonoMbedTlsAPI.mbedtls_ssl_init (out m_SslContext);
			MonoMbedTlsAPI.mbedtls_ssl_config_init (out m_SslConfig);
			MonoMbedTlsAPI.mbedtls_ctr_drbg_init (out m_RandomGeneratorContext);
			MonoMbedTlsAPI.mbedtls_entropy_init (out m_EntropyContext);
			MonoMbedTlsAPI.mbedtls_x509_crt_init (out m_RootCertificateChain);
			MonoMbedTlsAPI.mbedtls_x509_crt_init (out m_OwnCertificateChain);
			MonoMbedTlsAPI.mbedtls_pk_init (out m_OwnPrivateKeyContext);

			Mono.Mbed.Debug.CheckAndThrow (MonoMbedTlsAPI.mbedtls_ssl_config_defaults (ref m_SslConfig,
			    IsServer ? MonoMbedTlsAPI.MBEDTLS_SSL_IS_SERVER : MonoMbedTlsAPI.MBEDTLS_SSL_IS_CLIENT,
			    MonoMbedTlsAPI.MBEDTLS_SSL_TRANSPORT_STREAM,
			    MonoMbedTlsAPI.MBEDTLS_SSL_PRESET_DEFAULT),
				"SSL default configuration failed");

			int seedStringLength = m_NativeBuffer.ToNative (SSL_SEED_STR);
			Mono.Mbed.Debug.CheckAndThrow (MonoMbedTlsAPI.mbedtls_ctr_drbg_seed (ref m_RandomGeneratorContext,
			    m_EntropyCallback = MonoMbedTlsAPI.mbedtls_entropy_func,
			    ref m_EntropyContext, m_NativeBuffer.DataPtr, seedStringLength),
			    "Unable to create random generator");
			Mono.Mbed.Debug.CheckAndThrow (MonoMbedTlsAPI.mbedtls_ssl_conf_rng (ref m_SslConfig,
			    m_RandomCallback = MonoMbedTlsAPI.mbedtls_ctr_drbg_random,
			    ref m_RandomGeneratorContext),
			    "Unable to configure random generator");

			MonoMbedTlsAPI.mbedtls_ssl_conf_dbg (ref m_SslConfig, m_DebugCallback = Mono.Mbed.Debug.Callback, IntPtr.Zero);
			MonoMbedTlsAPI.mbedtls_ssl_conf_ca_chain (ref m_SslConfig, ref m_RootCertificateChain, IntPtr.Zero);
			MonoMbedTlsAPI.mbedtls_ssl_conf_own_cert (ref m_SslConfig, ref m_OwnCertificateChain, ref m_OwnPrivateKeyContext);
			MonoMbedTlsAPI.mbedtls_ssl_set_bio (ref m_SslContext, IntPtr.Zero, m_BIOWriteCallback = BIOWrite, m_BIOReadCallback = BIORead, null);
			MonoMbedTlsAPI.mbedtls_ssl_conf_verify (ref m_SslConfig, m_VerifyCallback = Verify, IntPtr.Zero);

			Mono.Mbed.Debug.CheckAndThrow (MonoMbedTlsAPI.mbedtls_ssl_setup (ref m_SslContext, ref m_SslConfig),
			    "SSL context setup failed");
		}

		public override bool HasContext {
			get { return true; }
		}

		public override bool IsAuthenticated {
			get { return m_IsAuthenticated; }
		}

		public override MonoTlsConnectionInfo ConnectionInfo {
			get { return m_Connectioninfo; }
		}
		internal override bool IsRemoteCertificateAvailable {
			get { return m_RemoteCertificate != null; }
		}
		internal override X509Certificate LocalClientCertificate {
			get { return m_LocalClientCertificate; }
		}
		public override X509Certificate RemoteCertificate {
			get { return m_RemoteCertificate; }
		}
		public override TlsProtocols NegotiatedProtocol {
			get { return ConnectionInfo.ProtocolVersion; }
		}

		[MonoTODO ("Handle intermediate certificates")]
		void SetPrivateCertificate (X509Certificate cert)
		{
			if (cert == null)
				return;

			CertificateHelper.AddToChain (ref m_OwnCertificateChain, cert);

			X509Certificate2 privateKeyCert = cert as X509Certificate2;
			if (privateKeyCert == null)
				return;

			CertificateHelper.SetPrivateKey (ref m_OwnPrivateKeyContext, privateKeyCert.PrivateKey);
		}

		public override void Flush ()
		{
			// NO-OP
		}

		public override int Read (byte [] buffer, int offset, int count, out bool wouldBlock)
		{
			wouldBlock = false;

			int bufferSize = System.Math.Min(MaxIOBufferSize, count);
			m_NativeIOReadBuffer.EnsureSize (bufferSize);
			int result = MonoMbedTlsAPI.mbedtls_ssl_read (ref m_SslContext, m_NativeIOReadBuffer.DataPtr, bufferSize);
			if (result == MonoMbedTlsAPI.MBEDTLS_ERR_SSL_WANT_READ) {
				wouldBlock = true;
				return 0;
			}
			Mono.Mbed.Debug.CheckAndThrow (result, "Read error");
			m_NativeIOReadBuffer.ToManaged(buffer, offset, result);
			return result;
		}

		public override int Write (byte [] buffer, int offset, int count, out bool wouldBlock)
		{
			wouldBlock = false;

			int bufferSize = System.Math.Min(MaxIOBufferSize, count);
			m_NativeIOWriteBuffer.EnsureSize (bufferSize);
			Marshal.Copy (buffer, offset, m_NativeIOWriteBuffer.DataPtr, bufferSize);
			int result = MonoMbedTlsAPI.mbedtls_ssl_write (ref m_SslContext, m_NativeIOWriteBuffer.DataPtr, bufferSize);
			if (result == MonoMbedTlsAPI.MBEDTLS_ERR_SSL_WANT_WRITE) {
				wouldBlock = true;
				return 0;
			}
			Mono.Mbed.Debug.CheckAndThrow (result, "Write error");
			return result;
		}

		[MonoTODO ("Implement this")]
		public override void Close ()
		{
//			throw new NotImplementedException();
		}

		protected override void Dispose (bool disposing)
		{
			try {
				if (disposing) {

					// free native resources
					MonoMbedTlsAPI.mbedtls_ssl_free (ref m_SslContext);
					MonoMbedTlsAPI.mbedtls_ssl_config_free (ref m_SslConfig);
					MonoMbedTlsAPI.mbedtls_ctr_drbg_free (ref m_RandomGeneratorContext);
					MonoMbedTlsAPI.mbedtls_entropy_free (ref m_EntropyContext);
					MonoMbedTlsAPI.mbedtls_x509_crt_free (ref m_RootCertificateChain);
					MonoMbedTlsAPI.mbedtls_x509_crt_free (ref m_OwnCertificateChain);
					MonoMbedTlsAPI.mbedtls_pk_free (ref m_OwnPrivateKeyContext);

					// reset callbacks
		            m_EntropyCallback = null;
		            m_RandomCallback = null;
		            m_BIOWriteCallback = null;
		            m_BIOReadCallback = null;
		            m_DebugCallback = null;

		            // reset states
					m_LocalClientCertificate = null;
					m_RemoteCertificate = null;
					m_Connectioninfo = null;
					m_IsAuthenticated = false;

					m_NativeBuffer.Dispose();
					m_NativeIOWriteBuffer.Dispose();
					m_NativeIOReadBuffer.Dispose();
					m_ManagedBuffer = null;
				}

			} finally {
				base.Dispose (disposing);
			}
		}

		private int Verify (IntPtr p_vrfy, ref MonoMbedTlsAPI.mbedtls_x509_crt _crt, int depth, ref uint flags)
		{
			// Skip intermediate and root certificates
			if (depth != 0)
				return 0;

			// Assemble a collection of the entire trust chain
			X509CertificateCollection certificates = new X509CertificateCollection ();
			MonoMbedTlsAPI.mbedtls_x509_crt crt    = _crt;
			while (true)
			{
				certificates.Add(CertificateHelper.AsX509(ref crt));
				if (crt.next == IntPtr.Zero)
					break;
				crt = Marshal.PtrToStructure<MonoMbedTlsAPI.mbedtls_x509_crt> (crt.next);
			}
			if (ValidateCertificate (certificates))
				flags = 0;
			else
				flags |= MonoMbedTlsAPI.MBEDTLS_X509_BADCERT_NOT_TRUSTED;
			return 0;
		}

		[MonoTODO ("Honor explicit protocol selection")]
		[MonoTODO ("Honor explicit cipher selection")]
		public override void StartHandshake ()
		{
			if (m_SslContext.state > 0)
				Mono.Mbed.Debug.Throw (AlertDescription.InternalError, "Handshake started already");

			if (IsServer) {
				SetPrivateCertificate (LocalServerCertificate);
			}
			if (!IsServer) {
				m_NativeBuffer.ToNative (ServerName);
				Mono.Mbed.Debug.CheckAndThrow (MonoMbedTlsAPI.mbedtls_ssl_set_hostname (ref m_SslContext, m_NativeBuffer.DataPtr), "Unable to set hostname");
			}
			if (!IsServer || AskForClientCertificate) {
				MonoMbedTlsAPI.mbedtls_ssl_conf_authmode (ref m_SslConfig, MonoMbedTlsAPI.MBEDTLS_SSL_VERIFY_REQUIRED);
			}
			CertificateHelper.AddSystemCertificates(ref m_RootCertificateChain);
		}

		public override bool ProcessHandshake ()
		{
			if (m_SslContext.state == MonoMbedTlsAPI.MBEDTLS_SSL_HANDSHAKE_OVER)
				Mono.Mbed.Debug.Throw (AlertDescription.InternalError, "Handshake is over");

			Int32 result = MonoMbedTlsAPI.mbedtls_ssl_handshake_step (ref m_SslContext);
			if (result == MonoMbedTlsAPI.MBEDTLS_ERR_SSL_WANT_READ ||
			    result == MonoMbedTlsAPI.MBEDTLS_ERR_SSL_WANT_WRITE)
				return false;

			Mono.Mbed.Debug.CheckAndThrow (result, "Handshake failed");
			Mono.Mbed.Debug.WriteLine (1, "State {0}", m_SslContext.state);
			switch (m_SslContext.state) {
			case MonoMbedTlsAPI.MBEDTLS_SSL_CERTIFICATE_REQUEST:
					m_RemoteCertificate = CertificateHelper.AsX509 (MonoMbedTlsAPI.mbedtls_ssl_get_peer_cert (ref m_SslContext));
					m_LocalClientCertificate = SelectClientCertificate (m_RemoteCertificate, null);
					SetPrivateCertificate (m_LocalClientCertificate);
					break;

			case MonoMbedTlsAPI.MBEDTLS_SSL_HANDSHAKE_OVER:
				return true;
			}
			return false;
		}

		public override void FinishHandshake ()
		{
			if (IsServer && AskForClientCertificate) {
				if (!ValidateCertificate (null, null))
					Mono.Mbed.Debug.Throw (AlertDescription.CertificateUnknown, "unknown certificate");
			}

			IntPtr cipher = MonoMbedTlsAPI.mbedtls_ssl_get_ciphersuite (ref m_SslContext);
			m_Connectioninfo = new MonoTlsConnectionInfo () {
				CipherSuiteCode = (CipherSuiteCode)MonoMbedTlsAPI.mbedtls_ssl_get_ciphersuite_id (cipher),
				ProtocolVersion = GetProtocol (m_SslContext.minor_ver),
				PeerDomainName = ServerName
			};
			m_IsAuthenticated = true;
		}

		static TlsProtocols GetProtocol (int protocol)
		{
			switch (protocol) {
			case 1:
				return TlsProtocols.Tls10;
			case 2:
				return TlsProtocols.Tls11;
			case 3:
				return TlsProtocols.Tls12;
			default:
				throw new NotSupportedException ();
			}
		}


		int BIOWrite (IntPtr stream, IntPtr buffer, size_t len)
		{
			bool wouldBlock = false;
			ManagedBufferEnsureSize (len);

			Marshal.Copy (buffer, m_ManagedBuffer, 0, len);

			if (!Parent.InternalWrite (m_ManagedBuffer, 0, len))
				return -1;
			int result = len;
			if (result < 0)
				return MonoMbedTlsAPI.MBEDTLS_ERR_NET_SEND_FAILED;
			if (wouldBlock)
				return MonoMbedTlsAPI.MBEDTLS_ERR_SSL_WANT_WRITE;
			return result;
		}

		int BIORead (IntPtr stream, IntPtr buffer, size_t len)
		{
			bool wouldBlock;
			ManagedBufferEnsureSize (len);

			int result = Parent.InternalRead (m_ManagedBuffer, 0, len, out wouldBlock);
			if (result < 0)
				return MonoMbedTlsAPI.MBEDTLS_ERR_NET_RECV_FAILED;
			if (wouldBlock)
				return MonoMbedTlsAPI.MBEDTLS_ERR_SSL_WANT_READ;

			Marshal.Copy (m_ManagedBuffer, 0, buffer, result);
			return result;
		}

		void ManagedBufferEnsureSize(int size)
		{
			if (size <= m_ManagedBuffer.Length)
				return;
			m_ManagedBuffer = new byte[size];
		}
	}
}
#endif