#include <config.h>
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
	*format = DATATYPE_STRING;
	// Order matters when it comes to certificates need to read in reverse
	int currentCert = numCerts;
	if (*iter != NULL)
	{
		currentCert = (int)*iter;
	}

	SecCertificateRef cert = (SecCertificateRef)CFArrayGetValueAtIndex((CFDataRef)certStore, (currentCert - 1));

	s = SecItemExport(cert, kSecFormatPEMSequence, kSecItemPemArmour, NULL, &certData);
	if (s != errSecSuccess)
	{
		printf("CertFetch cert export failed %i\n", s);
		return FALSE;
	}
	char* certPEMStr = (char*)CFDataGetBytePtr(certData);
	*iter = currentCert - 1;
	*data = certPEMStr;
	*size = sizeof(certPEMStr);
	if ((currentCert - 1) > 0)
	{
		return TRUE;
	}

	return FALSE;
}

void* OpenSystemRootStore()
{
	CFDataRef anchors;
	OSStatus s;

	s = SecTrustCopyAnchorCertificates(&anchors);

	if (s != noErr)
	{
		printf("Error: %i\n", s);
	}

	printf("Returning pointer to anchors\n");

	return anchors;
}

void CloseSystemRootStore(void* cStore)
{
	CFRelease(cStore);
}
