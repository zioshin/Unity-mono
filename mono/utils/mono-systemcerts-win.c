#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mono-systemcerts.h"

#include <stdio.h>
#include <Windows.h>
#include <wincrypt.h>


int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data)
{
	HCERTSTORE hStore = (HCERTSTORE)certStore;
	*format = DATATYPE_INTPTR;

	// Build list of system certificates
	PCCERT_CONTEXT pContext = *iter;
	if (pContext = CertEnumCertificatesInStore(hStore, pContext))
	{
		*iter = pContext;
		*data = pContext->pbCertEncoded;
		*size = pContext->cbCertEncoded;
		return TRUE;
	} else if (*iter) {
		CertFreeCertificateContext(*iter);
	}

	return FALSE;
}

void* OpenSystemRootStore()
{
	HCERTSTORE hStore = CertOpenSystemStore(0, L"ROOT");
	if (hStore == NULL)
		return 0;

	return hStore;
}

void CloseSystemRootStore(void* cStore)
{
	CertCloseStore((HCERTSTORE)cStore, 0);
}