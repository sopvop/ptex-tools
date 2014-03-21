#include <ptex_merge.hpp>

#include <stdlib.h>
#include <iostream>
#include <iomanip>

int main(int argc, const char** argv){
    if (argc < 4) {
        std::cerr<<"Please specify at least 2 input files and output filename"<<std::endl;
	return -1;
    }
    int nfiles = argc-2;
    int *offsets = (int*) malloc(sizeof(int)*nfiles);
    Ptex::String err_msg;
    if (ptex_merge(nfiles, argv+1, argv[argc-1], offsets, err_msg)) {
         std::cerr<<err_msg.c_str()<<std::endl;
         free(offsets);
         return -1;
    }
    for (int i = 0; i < nfiles; ++i){
	std::cout<<argv[i+1]<<":"<<offsets[i]<<std::endl;
    }
    free(offsets);
}
