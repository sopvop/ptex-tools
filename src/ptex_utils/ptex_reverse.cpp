#include "ptex_util.hpp"
#include <map>
#include <algorithm>
#include <vector>
#include <iterator>

static Ptex::EdgeId swap_edge(Ptex::EdgeId i) {
    switch((int) i) {
    case Ptex::e_bottom: return Ptex::e_left;
    case Ptex::e_right: return Ptex::e_top;
    case Ptex::e_top: return Ptex::e_right;
    case Ptex::e_left: return Ptex::e_bottom;
    }
    return Ptex::e_bottom;
}

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

static void
reverse_meta(PtexTexture* input, PtexWriter *output){
    PtexMetaData *meta = input->getMetaData();
    if (!meta)
        return;
    const int32_t *counts = 0, *indices = 0;
    int ncounts, nindices;
    meta->getValue("PtexFaceVertCounts", counts, ncounts);
    meta->getValue("PtexFaceVertIndices", indices, nindices);
    if (!(counts && indices && ncounts && nindices))
        return;
    int curr = 0;
    std::vector<int32_t> rev_indices;
    rev_indices.reserve(nindices);
    for (int i = 0; i < ncounts; ++i) {
        const int32_t *base = indices+curr;
        rev_indices.push_back(*base);
        std::reverse_copy(base+1, base+counts[i],
                          std::back_inserter(rev_indices));
        curr += counts[i];
    }
    output->writeMeta("PtexFaceVertCounts", counts, ncounts);
    output->writeMeta("PtexFaceVertIndices", rev_indices.data(), nindices);

    const float *positions = 0;
    int npositions;
    meta->getValue("PtexVertPositions", positions, npositions);
    if (positions && npositions)
        output->writeMeta("PtexVertPositions", positions, npositions);

}

typedef std::map<int,int> face_map;

static bool
is_adjacent(int faceid, const Ptex::FaceInfo &face){
    return face.adjface(0) == faceid
        || face.adjface(1) == faceid
        || face.adjface(2) == faceid
        || face.adjface(3) == faceid;
}

// reverse order of subfaces 0,1,2 -> 0,2,1
static void
build_subface_map(PtexTexture *ptex, face_map &map){
    const int num_faces = ptex->numFaces();
    std::vector<int> sub_group;
    int i = 0;
    while (i < num_faces) {
        const Ptex::FaceInfo &face_info = ptex->getFaceInfo(i);
        if (!face_info.isSubface()){
            ++i;
            continue;
        }
        int first_face = i;
        ++i;
        while (is_adjacent(first_face, ptex->getFaceInfo(i))) {
            ++i;
        }
        map.insert(std::make_pair(first_face, first_face));
        for (int j = i-1; j > first_face; --j) {
            map.insert(std::make_pair(first_face + i-j, j));
        }
    }
};

int subface_id(face_map &subface_map, int face){
    if (face == -1)
        return -1;
    face_map::const_iterator it = subface_map.find(face);
    if (it == subface_map.end())
        return face;
    return it->second;
}

static Ptex::FaceInfo
prepare_faceinfo(face_map &subface_map, const Ptex::FaceInfo &face_info)
{
    Ptex::FaceInfo out_face = face_info;
    out_face.setadjfaces(subface_id(subface_map, face_info.adjfaces[3]),
                         subface_id(subface_map, face_info.adjfaces[2]),
                         subface_id(subface_map, face_info.adjfaces[1]),
                         subface_id(subface_map, face_info.adjfaces[0]));
    out_face.setadjedges(swap_edge(face_info.adjedge(3)),
                         swap_edge(face_info.adjedge(2)),
                         swap_edge(face_info.adjedge(1)),
                         swap_edge(face_info.adjedge(0)));
    return out_face;
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
    face_map subface_map;
    build_subface_map(input, subface_map);
    for (int i = 0; i < num_faces; ++i) {
        const Ptex::FaceInfo &face_info = input->getFaceInfo(i);
        Ptex::FaceInfo out_face = prepare_faceinfo(subface_map, face_info);
        int stride = data_block_size*out_face.res.size();

        if (stride > data_size) {
            data_size = stride;
            data = realloc(data, data_size);
            outdata = realloc(data, data_size);
        }

        input->getData(i, data, 0);
        int out_id = subface_id(subface_map, i);
        if (face_info.isConstant()) {
            output->writeConstantFace(out_id, out_face, data);
        } else {
            out_face.res.swapuv();
            swap_data(data_block_size, face_info.res.u(), face_info.res.v(), (char *) data,
                      (char *) outdata);
            output->writeFace(out_id, out_face, outdata);
        }
    }
    free(data);
    free(outdata);
    reverse_meta(input, output);
    int res = !output->close(err_msg);
    input->release();
    output->release();
    return res;
}
