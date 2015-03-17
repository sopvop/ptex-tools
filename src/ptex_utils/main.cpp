#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <string.h>

#include "ptex_util.hpp"

#ifdef _WINDOWS
static char *basename(char *s)
{
    size_t i;
    if (!s || !*s) return ".";
    i = strlen(s)-1;
    for (; i&&s[i]=='/'; i--) s[i] = 0;
    for (; i&&s[i-1]!='/'; i--);
    return s+i;
}
#else
#include <libgen.h>
#endif

using namespace ptex_tools;

int do_ptex_merge(int argc, const char** argv) {
    if (argc < 5) {
        char *prog = strdup(argv[0]);
        std::cerr<<"Usage:"<<std::endl
                 <<basename(prog)
                 <<" merge input.ptx input2.ptx [input3.ptx ..] output.ptx"<<std::endl;
        free(prog);
	return -1;
    }
    int nfiles = argc-3;
    int *offsets = (int*) malloc(sizeof(int)*nfiles);
    Ptex::String err_msg;
    if (ptex_merge(nfiles, argv+2, argv[argc-1], offsets, err_msg)) {
         std::cerr<<err_msg.c_str()<<std::endl;
         free(offsets);
         return -1;
    }
    for (int i = 0; i < nfiles; ++i){
	std::cout<<offsets[i]<<":"<<argv[i+1]<<std::endl;
    }
    free(offsets);
    return 0;
}

int do_ptex_reverse(int argc, const char** argv) {
    if (argc != 4) {
        char *prog = strdup(argv[0]);
        std::cerr<<"Usage:"<<std::endl
                 <<basename(prog)
                 <<" reverse input.ptx output.ptx"<<std::endl;
        free(prog);
        return -1;
    }
    Ptex::String err_msg;
    if (ptex_reverse(argv[2], argv[3], err_msg)) {
        std::cerr<<err_msg.c_str()<<std::endl;
        return -1;
    }
    return 0;
}

int main(int argc, const char** argv){
    if (argc < 2) {
        std::cerr<<"Please specify tool: merge, reverse"<<std::endl;
        return -1;
    }
    std::string tool = argv[1];
    if (tool == "merge")
        return do_ptex_merge(argc, argv);
    else if (tool == "reverse")
        return do_ptex_reverse(argc, argv);
    else {
        std::cerr<<"Unknown tool: "<<tool<<std::endl;;
        return -1;
    }
}
