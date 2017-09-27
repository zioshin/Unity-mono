#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mono-systemcerts.h"

#include <stdio.h>
#include <stdlib.h>

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data)
{
	if (*iter == 0)
	{
		*data = (char*)certStore;
		*size = sizeof((char*)certStore);
		*format = DATATYPE_STRING;
		*iter = 1;
		return 1;
	}

	return 0;
}

void* OpenSystemRootStore()
{
	FILE *fp;
	char* buffer = 0;
	long length;

	// For now open the default ubuntu CA location
	fp = fopen ("/etc/ssl/certs/ca-certificates.crt", "rb");

	if (fp)
	{
		fseek (fp, 0, SEEK_END);
		length = ftell(fp);
		fseek (fp, 0, SEEK_SET);
		buffer = malloc (length);

		if (buffer)
		{
			fread (buffer, 1, length, fp);
		}

		fclose(fp);
	}

	return buffer;
}

void CloseSystemRootStore(void* cStore)
{
	free(cStore);
}