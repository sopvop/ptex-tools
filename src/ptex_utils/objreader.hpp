#pragma once

#include <vector>

#include <Ptexture.h>


struct obj_mesh {
    std::vector<int32_t> nverts;
    std::vector<int32_t> verts;
    std::vector<float> pos;
};


int check_consistency(const obj_mesh &mesh, Ptex::String &err_msg);
int parse_obj(const char* file, obj_mesh & mesh, Ptex::String &err_msg);
