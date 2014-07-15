#pragma once

#include <Ptexture.h>

#ifdef _MSC_VER
#define SMIMPORT __declspec(dllinport)
#define SMEXPORT __declspec(dllexport)
#else
#define SMIMPORT
#define SMEXPORT __attribute__((visibility("default")))
#endif

#ifdef SMBUILD_SHARED
#define SMAPI SMEXPORT
#else
#define SMAPI SMIMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ptex_merge(int nfiles, const char** files,
	       const char* output_file, int *offsets,
	       Ptex::String &err_msg);

int ptex_reverse(const char* file,
                 const char* output_file,
                 Ptex::String &err_msg);
#ifdef __cplusplus
}
#endif

