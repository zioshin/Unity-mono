#ifndef _MONO_METADATA_UNITY_BASELIB_H_
#define _MONO_METADATA_UNITY_BASELIB_H_

void unity_baselib_init(void);
void unity_baselib_cleanup(void);

// To add a new function named `function_name` from baselib that is used in Mono:
// 1. Create a typedef for the function signature named `function_name_Type`
// 2. Create an extern of the type from (1) named `function_name`
// 3. In unity-baselib.c define a global for the extern created in (2)
// 4. In the unity_baselib_init function, load the function
// 4. In the unity_baselib_cleanup function, unload the function
//
// Now `function_name` can be used anywhere that unity-baselib.h is included.

typedef const char* (*PAL_Identification_GetPlatformName_Type) ();
extern PAL_Identification_GetPlatformName_Type PAL_Identification_GetPlatformName;

#endif // _MONO_METADATA_UNITY_BASELIB_H_