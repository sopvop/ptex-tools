#pragma once

#include <Ptexture.h>

#include "ptexutils_export.h"

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

PTEXUTILS_EXPORT
int ptex_merge(int nfiles, const char** files,
	       const char* output_file, int *offsets,
	       Ptex::String &err_msg);

PTEXUTILS_EXPORT
int ptex_merge(const PtexMergeOptions &opts,
               int nfiles, const char** files,
	       const char* output_file, int *offsets,
	       Ptex::String &err_msg);

PTEXUTILS_EXPORT
int ptex_remerge(const char *file,
                 const char *dir,
                 Ptex::String &err_msg);

PTEXUTILS_EXPORT
int ptex_reverse(const char* file,
                 const char* output_file,
                 Ptex::String &err_msg);

PTEXUTILS_EXPORT
int make_constant(const char* file,
                  Ptex::DataType dt, int nchannels, int alphachan,
                  const void* data,
                  int nfaces, int32_t *nverts, int32_t *verts,
                  float* pos, Ptex::String &err_msg);

PTEXUTILS_EXPORT
int ptex_info(const char* file, PtexInfo &info, Ptex::String &err_msg);

}
