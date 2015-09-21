#pragma once

#include <Ptexture.h>

#if defined _WIN32 || defined __CYGWIN__
  #define PTEXUTILS_IMPORT __declspec(dllimport)
  #define PTEXUTILS_EXPORT __declspec(dllexport)
#else
  #define PTEXUTILS_IMPORT __attribute__ ((visibility ("default")))
  #define PTEXUTILS_EXPORT __attribute__ ((visibility ("default")))
#endif

#ifdef PTEXUTILS_SHARED
  #ifdef PTEXUTILS_EXPORTS
    #define PTEXUTILS_API PTEXUTILS_EXPORT
  #else
    #define PTEXUTILS_API PTEXUTILS_IMPORT
  #endif
#else
  #define PTEXUTILS_API
#endif

namespace ptex_utils {

struct PtexInfo {
    Ptex::DataType data_type;
    Ptex::MeshType mesh_type;
    int num_channels;
    int alpha_channel;
    Ptex::BorderMode u_border_mode;
    Ptex::BorderMode v_border_mode;
    int num_faces;
    bool has_edits;
    bool has_mip_maps;
    int num_meta_keys;
    uint64_t texels;
};

struct PtexMeta
{
    int32_t size;
    Ptex::MetaDataType *types;
    const char **keys;
    int *counts;
    const void **data;
};

struct PtexMergeOptions
{
    Ptex::DataType data_type = Ptex::dt_uint8;
    Ptex::MeshType mesh_type = Ptex::mt_quad;
    int num_channels = 1;
    int alpha_channel = -1;
    Ptex::BorderMode u_border_mode = Ptex::m_clamp;
    Ptex::BorderMode v_border_mode = Ptex::m_clamp;
    bool merge_mesh = true;
    bool (*callback) (int, void*) = 0;
    void *callback_data = 0;
    const char *root = 0;
    PtexMeta * meta = 0;
};

PTEXUTILS_API
int ptex_merge(int nfiles, const char** files,
	       const char* output_file, int *offsets,
	       Ptex::String &err_msg);

PTEXUTILS_API
int ptex_merge(const PtexMergeOptions &opts,
               int nfiles, const char** files,
	       const char* output_file, int *offsets,
	       Ptex::String &err_msg);

PTEXUTILS_API
int ptex_remerge(const char *file,
                 const char *dir,
                 Ptex::String &err_msg);

PTEXUTILS_API
int ptex_reverse(const char* file,
                 const char* output_file,
                 Ptex::String &err_msg);

PTEXUTILS_API
int make_constant(const char* file,
                  Ptex::DataType dt, int nchannels, int alphachan,
                  const void* data,
                  int nfaces, int32_t *nverts, int32_t *verts,
                  float* pos, Ptex::String &err_msg);

PTEXUTILS_API
int ptex_info(const char* file, PtexInfo &info, Ptex::String &err_msg);

PTEXUTILS_API
int32_t count_ptex_faces(int32_t nfaces, int32_t *nverts);

PTEXUTILS_API
void compute_adjacency(int32_t nfaces, int32_t *nverts, int32_t *verts,
                       Ptex::FaceInfo *out);

PTEXUTILS_API
bool ptex_topology_match(int32_t n, const Ptex::FaceInfo *nfaces,
                         const Ptex::FaceInfo *mfaces,
                         int32_t noffset = 0, int32_t moffset = 0);

PTEXUTILS_API
int ptex_conform(const char* filename,
                 const char* output_filename,
                 int8_t downsteps,
                 int8_t clampsize,
                 bool change_datatype,
                 Ptex::DataType out_dt,
                 Ptex::String &err_msg);


}
