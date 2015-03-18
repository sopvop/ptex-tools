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


namespace ptex_tools {
SMAPI
int ptex_merge(int nfiles, const char** files,
	       const char* output_file, int *offsets,
	       Ptex::String &err_msg);

SMAPI
int ptex_reverse(const char* file,
                 const char* output_file,
                 Ptex::String &err_msg);

SMAPI
int make_constant(const char* file,
                  Ptex::DataType dt, int nchannels, int alphachan,
                  const void* data,
                  int nfaces, int32_t *nverts, int32_t *verts,
                  float* pos, Ptex::String &err_msg);
}
