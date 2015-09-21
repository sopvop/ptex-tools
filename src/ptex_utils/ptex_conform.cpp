#include <vector>

#include <Ptexture.h>

#include "ptexutils.hpp"
#include "helpers.hpp"


int ptex_utils::ptex_conform(const char* filename,
                             const char* output_filename,
                             int8_t downsteps,
                             int8_t clampsize,
                             bool change_datatype,
                             Ptex::DataType out_dt,
                             Ptex::String &err_msg)
{
    PtxPtr ptx(PtexTexture::open(filename, err_msg, 0));
    if (!ptx) {
        err_msg = "Can't open for reading " + std::string(filename) + ":" + err_msg;
        return -1;
    }

    downsteps = std::max(0, (int) downsteps);
    clampsize = std::max(0, (int) downsteps);

    Ptex::DataType input_dt = ptx->dataType();
    Ptex::DataType dt = change_datatype ? out_dt : input_dt;
    int nfaces = ptx->numFaces();
    int nchannels = ptx->numChannels();

    WriterPtr writer(PtexWriter::open(output_filename,
                                      ptx->meshType(),
                                      dt,
                                      nchannels,
                                      ptx->alphaChannel(),
                                      nfaces,
                                      err_msg));
    if (!writer) {
        err_msg = "Can't open for writing " + std::string(output_filename) + ":" + err_msg;
        return -1;
    }

    int8_t clamp_log = clampsize <= 0 ? 15 : clampsize;
    Ptex::Res clamp_res(clamp_log, clamp_log);

    const size_t input_pixel_size = Ptex::DataSize(ptx->dataType()) * nchannels;
    const size_t pixel_size = Ptex::DataSize(dt)*nchannels;

    std::vector<char> in_buffer(input_pixel_size*128*128);
    std::vector<char> out_buffer(pixel_size*128*128);
    std::vector<float> fdata(nchannels*128*128);

    for (int face_id = 0; face_id < nfaces; ++face_id) {
        Ptex::FaceInfo face_info = ptx->getFaceInfo(face_id);

        if (!face_info.isConstant()) {
            Ptex::Res res = face_info.res;
            res.ulog2 = res.ulog2 > 2 ? std::max(1, res.ulog2 - downsteps) : res.ulog2;
            res.vlog2 = res.vlog2 > 2 ? std::max(1, res.vlog2 - downsteps) : res.vlog2;
            res.clamp(clamp_res);
            face_info.res = res;
        }

        const size_t input_size = input_pixel_size * face_info.res.size();
        if (in_buffer.size() < input_size) {
            in_buffer.resize(input_size);
        }

        ptx->getData(face_id, in_buffer.data(), 0, face_info.res);

        if (dt == input_dt) {
            if (face_info.isConstant()) {
                writer->writeConstantFace(face_id, face_info, in_buffer.data());
            }
            else {
                writer->writeFace(face_id, face_info, in_buffer.data(), 0);
            }
        }
        else {
            const size_t out_size = pixel_size * face_info.res.size();
            const size_t float_size = nchannels*face_info.res.size();
            if (out_buffer.size() < out_size) {
                out_buffer.resize(out_size);
            }
            if (fdata.size() < float_size) {
                fdata.resize(float_size);
            }

            Ptex::ConvertToFloat(fdata.data(), in_buffer.data(), input_dt,
                                 nchannels*face_info.res.size());
            Ptex::ConvertFromFloat(out_buffer.data(), fdata.data(),
                                   dt, nchannels*face_info.res.size());

            if (face_info.isConstant()) {
                writer->writeConstantFace(face_id, face_info, out_buffer.data());
            }
            else {
                writer->writeFace(face_id, face_info, out_buffer.data(), 0);
            }
        }
    }

    MetaPtr meta_ptr(ptx->getMetaData());
    writer->writeMeta(meta_ptr.get());

    writer->setBorderModes(ptx->vBorderMode(), ptx->uBorderMode());

    if (!writer->close(err_msg)){
        err_msg = "Closing writer " + std::string(output_filename) + ":" + err_msg.c_str();
	return -1;
    }

    return 0;
}

