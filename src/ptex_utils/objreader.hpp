#pragma once

#include <vector>

#include <Ptexture.h>


struct obj_mesh {
    std::vector<float> pos;
    std::vector<int> nverts;
    std::vector<int> verts;
};


int parse_obj(const char* file, obj_mesh & mesh, Ptex::String &err_msg);
