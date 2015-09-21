#include <map>

#include "mesh.hpp"
#include "ptexutils.hpp"

void count_mesh_elems(int32_t nfaces, int32_t *nverts,
                      int32_t &total_faces, int32_t &total_edges, int32_t &ptex_faces)
{
    ptex_faces = 0;
    total_faces = 0;
    total_edges = 0;
    for (int face = 0; face < nfaces; ++face) {
        int nv = nverts[face];
        if (nv == 4) {
            total_faces += 1;
            total_edges += 4;
            ptex_faces += 1;
        } else {
            ptex_faces += nv;
            total_faces += nv +1;
            total_edges += nv*4 + nv;
        }
    }
}

void count_mesh_vertices(int32_t nfaces, int32_t *nverts, int32_t *verts,
                         int32_t &vcount, int32_t &fvcount)
{
    vcount = 0;
    fvcount = 0;
    for (int face = 0; face < nfaces; ++face) {
        int nv = nverts[face];
        for (int i = fvcount; i < fvcount + nv; ++i) {
            vcount = std::max(verts[i], vcount);
        }
        fvcount += nv;
    }
    ++vcount;
}

using edge_map_t = std::map<std::pair<int,int>, half_edge*>;
struct MeshBuilder {
    half_mesh *mesh;
    edge_map_t edge_map;
    int next_edge = 0;
    int next_face = 0;
    MeshBuilder(half_mesh *m)
        : mesh(m)
    {}
};


static
void mark_opposite(MeshBuilder &builder, half_edge *e) {
    half_edge* n = next_face(e);
    int v = e->v, w = n->v;
    auto it = builder.edge_map.find(std::make_pair(w, v));
    if (it != builder.edge_map.end()) {
        half_edge *op = it->second;
        op->opposite = e;
        e->opposite = op;
    }
    else {
        builder.edge_map.emplace(std::make_pair(v, w), e);
    }
}

static
half_edge* add_edge(MeshBuilder &builder, int v) {
    int idx = builder.next_edge++;
    half_edge& e = builder.mesh->edges[idx];
    e.v = v;
    return &e;
}

template <typename It>
static
half_face* mark_face(MeshBuilder& builder, bool is_subface, int face_id, It first, It last) {
    half_edge* f = *first;
    half_face& face = builder.mesh->faces[face_id];
    face.face_index = face_id;
    face.first = f;
    face.is_subface = is_subface;
    int face_v = 0;

    half_edge* l = *std::prev(last);
    l->next = f;

    f->prev = l;
    f->fv = face_v++;
    f->face = &face;

    for(It eit = first + 1; eit != last; ++eit) {
        half_edge* e = *eit;
        half_edge* p = *std::prev(eit);
        e->prev = p;
        e->fv = face_v++;
        e->face = &face;
        p->next = e;
    }
    for(; first != last; ++first){
        mark_opposite(builder, *first);
    }
    return &face;
}

template <typename It>
static
half_face* add_face(MeshBuilder& builder, It first, It last) {
    std::vector<half_edge*> edges;
    std::transform(first, last, std::back_inserter(edges),
                   [&](int i) -> half_edge* { return add_edge(builder, i); });
    int idx = builder.next_face++;
    return mark_face(builder, false, idx, begin(edges), end(edges));
}

static
int last_vertex(int nfaces, int *nverts, int *verts) {
    int last = 0;
    int off = 0;
    for (int n = 0; n < nfaces; ++n) {
        int nv = nverts[n];
        for (int i = off; i < off+nv; ++i) {
            last = std::max(last, verts[i]);
        }
        off += nv;
    }
    return last;
}

static
void assign_subface_adjacency(half_edge *parent) {
    half_edge *first_adj = opposite(parent);
    if (first_adj && first_adj->sub_face) {
        first_adj = prev_face(next_face(first_adj)->sub_face->first);
    }
    half_edge *last_adj = prev_vert(parent);
    if (last_adj && last_adj->sub_face) {
        last_adj = last_adj->sub_face->first;
    }

    half_edge *e1 = parent->sub_face->first;
    half_edge *e2 = next_face(e1);
    half_edge *e3 = next_face(e2);
    half_edge *e4 = next_face(e3);

    e1->adjacent = first_adj;
    e2->adjacent = opposite(e2);
    e3->adjacent = opposite(e3);
    e4->adjacent = last_adj;
}

static
void subdiv_mesh(MeshBuilder &builder, int nfaces, int *nverts, int *vertices) {
    int next_v = last_vertex(nfaces, nverts, vertices)+1;
    auto split = [&](const half_edge *e) -> int {
        half_edge * op = opposite(e);
        if (op && op->sub_face) {
            return next_face(op->sub_face->first)->v;
        }
        else {
            return next_v++;
        }
    };

    std::array<int,4> verts;

    int ptex_index = 0;
    for (int face_id = 0; face_id < nfaces; ++face_id) {
        int nv = nverts[face_id];
        half_face &face = builder.mesh->faces[face_id];
        half_edge *first = face.first;
        if (nv == 4) {
            face.ptex_index = ptex_index++;
        }
        else {
            const int center_v = next_v++;
            const int first_split = split(first);
            int prev_v = first_split;
            half_edge* e = next_face(first);
            while (e != first) {
                int split_v = split(e);
                verts[0] = e->v;
                verts[1] = split_v;
                verts[2] = center_v;
                verts[3] = prev_v;
                half_face *f = add_face(builder, begin(verts), end(verts));
                f->is_subface = true;
                f->ptex_index = ptex_index++;
                e->sub_face = f;

                e = next_face(e);
                prev_v = split_v;

            }

            verts[0] = first->v;
            verts[1] = first_split;
            verts[2] = center_v;
            verts[3] = prev_v;

            half_face *f = add_face(builder, begin(verts), end(verts));
            f->is_subface = true;
            f->ptex_index = ptex_index++;
            first->sub_face = f;
        }
    }
    for (int face_id = 0; face_id < nfaces; ++face_id) {
        half_face &face = builder.mesh->faces[face_id];
        half_edge *first = face.first;
        half_edge *e = first;
        do {
            if (e->sub_face) {
                assign_subface_adjacency(e);
            } else {
                half_edge *adj = opposite(e);
                if (adj && adj->sub_face) {
                    adj = prev_face(next_face(adj)->sub_face->first);
                }
                if (adj) {
                    e->adjacent = adj;
                }
            }
            e = next_face(e);
        } while (e != first);
    }
}


void fill_faceinfos(const half_mesh &mesh, Ptex::FaceInfo *faces)
{
    Ptex::Res res(0,0);
    int adjfaces[4];
    int adjedges[4];

    for (const half_face &face : mesh.faces) {
        if (face.ptex_index >= 0) {
            half_edge *e1 = face.first;
            half_edge *e2 = next_face(e1);
            half_edge *e3 = next_face(e2);
            half_edge *e4 = next_face(e3);
            adjfaces[0] = adjacent_face(e1);
            adjfaces[1] = adjacent_face(e2);
            adjfaces[2] = adjacent_face(e3);
            adjfaces[3] = adjacent_face(e4);
            adjedges[0] = adjacent_edge(e1);
            adjedges[1] = adjacent_edge(e2);
            adjedges[2] = adjacent_edge(e3);
            adjedges[3] = adjacent_edge(e4);

            faces[face.ptex_index] = Ptex::FaceInfo(res, adjfaces,
                                                    adjedges,
                                                    face.is_subface);
        }
    }

}


void build_mesh(half_mesh &mesh, int nfaces, int *nverts, int *verts) {
    MeshBuilder builder(&mesh);

    int first = 0;
    for (int i = 0; i < nfaces; ++i) {
        int nv = nverts[i];
        add_face(builder, verts+first, verts+first+nv);
        first += nv;
    }
    subdiv_mesh(builder, nfaces, nverts, verts);
}


int32_t ptex_utils::count_ptex_faces(int32_t nfaces, int32_t *nverts)
{
    int32_t ptex_faces = 0;
    for (int face = 0; face < nfaces; ++face) {
        int nv = nverts[face];
        ptex_faces += nv == 4 ? 1 : nv;
    }
    return ptex_faces;
}

void ptex_utils::compute_adjacency(int32_t nfaces, int32_t *nverts, int32_t *verts,
                                   Ptex::FaceInfo *out)
{

    int ptex_faces = 0;  //total faces in ptex file
    int total_faces = 0; //faces count after subdivision
    int total_edges = 0; //edge count after subdivision
    count_mesh_elems(nfaces, nverts, total_faces, total_edges, ptex_faces);

    half_mesh mesh(total_faces, total_edges);
    build_mesh(mesh, nfaces, nverts, verts);

    fill_faceinfos(mesh, out);
}

bool ptex_utils::ptex_topology_match(int32_t n, const Ptex::FaceInfo *nfaces,
                                    const Ptex::FaceInfo *mfaces,
                                    int32_t noffset, int32_t moffset)
{
    for (int i = 0; i < n; ++i) {
        const Ptex::FaceInfo& nface = nfaces[i];
        const Ptex::FaceInfo& mface = mfaces[i];
        if (nface.adjedges != mface.adjedges
            || nface.isSubface() != mface.isSubface())
            return false;
        for (int f = 0; f < 4; ++f) {
            if (nface.adjfaces[f] == -1) {
                if (nface.adjfaces[f] == -1)
                    continue;
                else
                    return false;
            }
            if (nface.adjfaces[f] + noffset != mface.adjfaces[f] + moffset)
                return false;
        }
    }
    return true;
}

