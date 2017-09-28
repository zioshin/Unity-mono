#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mono-systemcerts.h"

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data)
{
	// Default location for linux CA
	const char* path = "/etc/ssl/certs/ca-certificates.crt";

	if (*iter == 0)
	{
		*data = path;
		*size = sizeof((char*)path);
		*format = DATATYPE_FILE;
		*iter = 1;
		return 1;
	}

	return 0;
}

void* OpenSystemRootStore()
{
	return 0;
}

void CloseSystemRootStore(void* cStore)
{
}