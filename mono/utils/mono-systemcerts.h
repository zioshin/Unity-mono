#pragma once

typedef enum
{
	DATATYPE_STRING = 0,
	DATATYPE_INTPTR = 1
} MonoCertDataFormat;

typedef struct
{
	void* certdata;
	int certsize;
} MonoCertObj;

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data);
void* OpenSystemRootStore();
void CloseSystemRootStore(void* cStore);

