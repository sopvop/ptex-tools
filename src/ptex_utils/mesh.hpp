#pragma once

#include <algorithm>
#include <stdint.h>
#include <vector>

#include <Ptexture.h>

struct half_face;

struct half_edge {
    half_face* face;
    int32_t v;
    int32_t fv;
    half_edge* opposite = 0;
    half_edge* next = 0;
    half_edge* prev  = 0;
    half_face* sub_face = 0;
    half_edge* adjacent = 0;
    half_edge(half_face *face, int32_t face_v, int32_t v)
        : face(face)
        , v(v)
        , fv(face_v)
        , opposite(0), next(0), prev(0)
        {};
    half_edge() = default;
};


struct half_face {
    int32_t face_index = -1;
    int32_t ptex_index = -1;
    int32_t is_subface = false;
    half_edge* first = 0;
};

struct half_mesh {
    std::vector<half_edge> edges;
    std::vector<half_face> faces;
    half_mesh(int32_t nfaces, int32_t nedges)
        : edges(nedges)
        , faces(nfaces)
        {}
};

inline
half_edge* next_face(const half_edge *e) { return e->next; };

inline
half_edge* prev_face(const half_edge *e) { return e->prev; };

inline
half_edge* opposite(const half_edge *e) { return e->opposite; };


inline
half_edge* next_vert(const half_edge *e) {
    half_edge *op = opposite(e);
    return op == 0 ? 0 : next_face(op);
}

inline
half_edge* prev_vert(half_edge *e) {
    return opposite(prev_face(e));
}

inline
int adjacent_face(const half_edge *e){
    if (e->adjacent)
        return e->adjacent->face->ptex_index;
    else
        return -1;
}

inline
int adjacent_edge(const half_edge *e) {
   if (e->adjacent)
       return e->adjacent->fv;
   else
       return 0;
}


void count_mesh_elems(int32_t nfaces, int32_t *nverts,
                      int32_t &total_faces, int32_t &total_edges, int32_t &ptex_faces);

void count_mesh_vertices(int32_t nfaces, int32_t *nverts, int32_t *verts,
                         int32_t &vcount, int32_t &fvcount);

void build_mesh(half_mesh &mesh, int nfaces, int *nverts, int *verts);

void fill_faceinfos(const half_mesh &mesh, Ptex::FaceInfo *faces);
