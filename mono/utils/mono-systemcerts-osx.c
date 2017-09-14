#include "mono-systemcerts.h"
#include <Security/SecTrust.h>
#include <Security/SecCertificate.h>
#include <Security/SecImportExport.h>
#include <mono/utils/mono-logger-internals.h>

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data)
{
	OSStatus s;
	CFDataRef certData;
	int numCerts = (int)CFArrayGetCount((CFDataRef)certStore);
	int nextCert = ((int)*iter) + 1;
	*format = DATATYPE_STRING;

	if (nextCert < numCerts)
	{
		SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex((CFDataRef)certStore, nextCert);

		s = SecItemExport(cert, kSecFormatPEMSequence, kSecItemPemArmour, NULL, &certData);
		if (s != errSecSuccess)
		{
			printf("CertFetch cert export failed %i\n", s);
			return FALSE;
		}
		char* certPEMStr = (char*)CFDataGetBytePtr(certData);
		iter = &nextCert;
		*data = certPEMStr;
		*size = sizeof(certPEMStr);
		return TRUE;
	}

	return FALSE;
}

void* OpenSystemRootStore()
{
	mono_trace (G_LOG_LEVEL_INFO, MONO_TRACE_SECURITY,"In OpenSystemRootStore");
	CFDataRef anchors;
	OSStatus s;

	s = SecTrustCopyAnchorCertificates(&anchors);

	if (s != noErr)
	{
		printf("Error: %i\n", s);
	}

	printf("Returning pointer to anchors\n");

	mono_trace (G_LOG_LEVEL_INFO, MONO_TRACE_SECURITY,"Attempting to return anchors");

	return anchors;
}

void CloseSystemRootStore(void* cStore)
{
	CFRelease(&cStore);
}