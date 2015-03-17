#include <string.h>

#include "objreader.hpp"

static
char* skip_space(char *str) {
    size_t s = strspn(str, " \t\r");
    return str + s;
}

std::string get_token(char **str) {
    *str = skip_space(*str);
    size_t n = strcspn(*str, " \t\r");
    std::string res(*str, n);
    *str += n;
    *str = skip_space(*str);
    return res;
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
        //parsePos
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
        if (parse_line(mesh, buf, err_msg)) {
            break;
        }
        r = fgets(buf, 8192, inp);
    }
    if (ferror(inp)) {
        status = -1;
        err_msg = "Read error";
    }
    fclose(inp);
    return status;
};
