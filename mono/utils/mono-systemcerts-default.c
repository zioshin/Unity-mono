#include <config.h>
#include "mono-systemcerts.h"

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data)
{
	return FALSE;
}

void* OpenSystemRootStore()
{
	return NULL;
}

void CloseSystemRootStore(void* cStore)
{
}