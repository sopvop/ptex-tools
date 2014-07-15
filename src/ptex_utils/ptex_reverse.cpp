#include "ptex_util.hpp"
#include <map>

static Ptex::EdgeId swap_edge(Ptex::EdgeId i) {
    switch((int) i) {
    case Ptex::e_bottom: return Ptex::e_left;
    case Ptex::e_right: return Ptex::e_top;
    case Ptex::e_top: return Ptex::e_right;
    case Ptex::e_left: return Ptex::e_bottom;
    }
    return Ptex::e_bottom;
}
/*
static void
swap_tile(int data_size, const Ptex::Res &r, char *data){
}
*/
static void
swap_data(int data_size, int u_size, int v_size, char *data, char* outdata) {
    for (int i =0; i < u_size; ++i) {
        for (int j = 0; j < v_size; ++j) {
            char *inp = data+(j*u_size+i)*data_size;
            char *outp = outdata+(i*u_size+j)*data_size;
            std::copy(inp, inp+data_size, outp);
        }
    }
}

int ptex_reverse(const char* file,
                 const char *output_file,
                 Ptex::String &err_msg)
{

    PtexTexture* input( PtexTexture::open(file, err_msg, false));
    if (input == 0)
        return -1;

    const Ptex::DataType data_type = input->dataType();
    const int num_channels = input->numChannels();
    const int num_faces = input->numFaces();
    PtexWriter* output = PtexWriter::open(
        output_file,
        input->meshType(),
        data_type,
        num_channels,
        input->alphaChannel(),
        num_faces,
        err_msg, false);

    if (output == 0) {
        input->release();
        return -1;
    }

    output->setBorderModes(input->uBorderMode(), input->vBorderMode());

    int data_block_size = Ptex::DataSize(data_type)*num_channels;
    int data_size = data_block_size*512*512;
    void *data = malloc(data_size);
    void *outdata = malloc(data_size);

    for (int i = 0; i < num_faces; ++i) {
        const Ptex::FaceInfo &face_info = input->getFaceInfo(i);
        Ptex::FaceInfo out_face = face_info;
        out_face.setadjfaces(face_info.adjfaces[3],
                             face_info.adjfaces[2],
                             face_info.adjfaces[1],
                             face_info.adjfaces[0]);
        out_face.setadjedges(swap_edge(face_info.adjedge(3)),
                             swap_edge(face_info.adjedge(2)),
                             swap_edge(face_info.adjedge(1)),
                             swap_edge(face_info.adjedge(0)));

        int stride = data_block_size*out_face.res.size();

        if (stride > data_size) {
            data_size = stride;
            data = realloc(data, data_size);
            outdata = realloc(data, data_size);
        }

        input->getData(i, data, 0);

        if (face_info.isConstant()) {
            output->writeConstantFace(i, out_face, data);
        } else {
            out_face.res.swapuv();
            swap_data(data_block_size, face_info.res.u(), face_info.res.v(), (char *) data,
                      (char *) outdata);
            output->writeFace(i, out_face, outdata);
        }
    }
    free(data);
    free(outdata);
    int res = !output->close(err_msg);
    input->release();
    output->release();
    return res;
}
