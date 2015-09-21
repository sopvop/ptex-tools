#include "ptexutils.hpp"

#include "helpers.hpp"

int ptex_utils::ptex_info(const char* file, PtexInfo &info, Ptex::String &err_msg)
{

    PtxPtr ptx(PtexTexture::open(file, err_msg, 0));
    if (!ptx) {
	return -1;
    }

    info.data_type     = ptx->dataType();
    info.mesh_type     = ptx->meshType();

    info.num_channels  = ptx->numChannels();
    info.alpha_channel = ptx->alphaChannel();

    info.u_border_mode = ptx->uBorderMode();
    info.v_border_mode = ptx->vBorderMode();

    info.num_faces     = ptx->numFaces();

    info.has_edits     = ptx->hasEdits();
    info.has_mip_maps  = ptx->hasMipMaps();

    MetaPtr meta(ptx->getMetaData());
    info.num_meta_keys = meta->numKeys();

    info.texels = 0;
    for (int face_id = 0; face_id < info.num_faces; ++face_id) {
        const Ptex::FaceInfo &face_info = ptx->getFaceInfo(face_id);
        info.texels += face_info.res.size();
    }

    return 0;
}
