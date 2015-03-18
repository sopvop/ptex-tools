#include <string.h>
#include "objreader.hpp"

static
char* skip_space(char *str) {
    size_t s = strspn(str, " \t\r");
    return str + s;
}

static
char* skip_to_space(char *str) {
    size_t s = strcspn(str, " \t\r");
    return str + s;
}

static
std::string get_token(char **str) {
    *str = skip_space(*str);
    size_t n = strcspn(*str, " \t\r");
    std::string res(*str, n);
    *str += n;
    *str = skip_space(*str);
    return res;
}

static
int parseDouble(char **str, double *v) {
    char *end;
    char *pos = skip_space(*str);
    double x = strtod(pos, &end);
    if (end != pos) {
        *v = x;
        *str = end;
        return 0;
    }
    return -1;
}

static
int parseInt(char **str, int32_t *v) {
    char *end;
    char *pos = skip_space(*str);
    long x = strtol(pos, &end,10);
    if (end != pos) {
        *v = x;
        *str = end;
        return 0;
    }
    return -1;
}

static
int parseV(obj_mesh &mesh, char **str) {
    char* pos = *str;
    double v[3];
    if (parseDouble(&pos, &v[0]))
        return -1;
    if (parseDouble(&pos, &v[1]))
        return -1;
    if (parseDouble(&pos, &v[2]))
        return -1;
    mesh.pos.insert(mesh.pos.end(), &v[0], &v[0]+3);
    *str = pos;
    return 0;
}

static
int parseF(obj_mesh &mesh, char **str) {
    char* pos = skip_space(*str);
    std::vector<int> face;
    while (pos[0] && pos[0] != '\n') {
        int v;
        if (parseInt(&pos, &v)) {
            return -1;
        }
        face.push_back(v-1);
        pos = skip_to_space(pos);
    }
    mesh.nverts.push_back(face.size());
    mesh.verts.insert(end(mesh.verts), begin(face), end(face));
    *str = pos;
    return 0;
}

static
bool line_end(char *buf) {
    return buf[0] == 0 || buf[0] == '\n';
}

static
int parse_line(obj_mesh &mesh, char *buf, Ptex::String &err_msg) {
    buf = skip_space(buf);
    if (line_end(buf))
        return 0;
    std::string tok = get_token(&buf);
    if (tok.empty())
        return 0;
    if (tok == "v") {
        if (parseV(mesh, &buf)) {
            err_msg = "Error parsing vertex coords";
            return -1;
        }
    }
    else if (tok == "f") {
        if (parseF(mesh, &buf)) {
            err_msg = "Error parsing face data";
            return -1;
        }
    }
    return 0;
}

static
int check_consistency(const obj_mesh &mesh, Ptex::String &err_msg) {
    if (mesh.nverts.size() == 0) {
        err_msg = "Mesh has no faces";
        return -1;
    }
    int vcount = 0;
    int fvcount = 0;
    for (int nv : mesh.nverts) {
        if (nv < 3) {
            err_msg = "Mesh has face with less than 3 vertices";
            return -1;
        }
        for (int v = fvcount; v < fvcount + nv; ++v) {
            vcount = std::max(mesh.verts[v], vcount);
        }
        fvcount += nv;
    }
    ++vcount;
    if ((int) mesh.pos.size() < vcount*3) {
        err_msg = "Not enough positions specified";
        return -1;
    }
    return 0;
}

int parse_obj(const char* file, obj_mesh & mesh, Ptex::String &err_msg) {
    FILE* inp = fopen(file, "r");
    if (inp == 0) {
        err_msg = "Cant open file";
        return -1;
    }
    char buf[8192];
    char *r = fgets(buf, 8192, inp);
    int status = 0;
    while (r) {
        status = parse_line(mesh, buf, err_msg);
        if (status)
            break;
        r = fgets(buf, 8192, inp);
    }
    if (ferror(inp)) {
        status = -1;
        err_msg = "Read error";
    }
    fclose(inp);
    if (!status)
        status = check_consistency(mesh, err_msg);
    return status;
};
