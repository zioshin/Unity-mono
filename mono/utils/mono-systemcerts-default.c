#include <config.h>
#include <stddef.h>
#include "mono-systemcerts.h"

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data)
{
	return 0;
}

void* OpenSystemRootStore()
{
	return NULL;
}

void CloseSystemRootStore(void* cStore)
{
}