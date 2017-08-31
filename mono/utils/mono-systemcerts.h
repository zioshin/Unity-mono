#pragma once

typedef enum
{
	DATATYPE_STRING = 0,
	DATATYPE_INTPTR = 1
} CertDataFormat;

typedef struct
{
	void* certdata;
	int certsize;
} CertObj;

int EnumSystemCertificates(void* certStore, void** iter, int *format, int* size, void** data);
void* OpenSystemRootStore();
void CloseSystemRootStore(void* cStore);

