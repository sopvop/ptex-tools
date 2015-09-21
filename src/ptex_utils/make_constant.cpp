#include <algorithm>
#include <numeric>
#include <map>
#include <utility>
#include <vector>

#include "ptexutils.hpp"
#include "mesh.hpp"


static
void write_data(PtexWriter *w, int num_ptex_faces, const half_mesh &mesh, const void* data)
{
    std::vector<Ptex::FaceInfo> face_infos(num_ptex_faces);
    fill_faceinfos(mesh, face_infos.data());
    for (int i = 0 ; i < num_ptex_faces; ++i) {
        w->writeConstantFace(i, face_infos[i], data);
    }
}

int ptex_utils::make_constant(const char* file,
                              Ptex::DataType dt,
                              int nchannels,
                              int alphachan,
                              const void* data,
                              int nfaces, int32_t *nverts, int32_t *verts,
                              float* pos, Ptex::String &err_msg)
{

    int ptex_faces = 0;  //total faces in ptex file
    int total_faces = 0; //faces count after subdivision
    int total_edges = 0; //edge count after subdivision
    count_mesh_elems(nfaces, nverts, total_faces, total_edges, ptex_faces);
    int vcount = 0;      //vertex count
    int fvcount = 0;     //face-vertex count
    count_mesh_vertices(nfaces, nverts, verts, vcount, fvcount);
    half_mesh mesh(total_faces, total_edges);
    build_mesh(mesh, nfaces, nverts, verts);
    PtexWriter *w = PtexWriter::open(file, Ptex::mt_quad, dt, nchannels, alphachan,
                                     ptex_faces, err_msg, true);
    write_data(w, ptex_faces, mesh, data);
    if (pos) {
        w->writeMeta("PtexFaceVertCounts",  nverts, nfaces);
        w->writeMeta("PtexFaceVertIndices", verts,  fvcount);
        w->writeMeta("PtexVertPositions",   pos,    vcount*3);
    }
    w->release();
    return 0;
}
