#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <string.h>
#include <limits>

#include <PtexHalf.h>

#include "ptexutils.hpp"

#include "helpers.hpp"
#include "objreader.hpp"

using namespace ptex_utils;

int do_ptex_merge(int argc, const char** argv) {
    std::string prog = strbasename(argv[0]);
    if (argc < 5) {
        std::cerr<<"Usage:"<<std::endl
                 << prog
                 <<" merge input.ptx input2.ptx [input3.ptx ..] output.ptx"<<std::endl;
	return -1;
    }
    int nfiles = argc-3;
    std::vector<int> offsets(nfiles);
    Ptex::String err_msg;
    if (ptex_merge(nfiles, argv+2, argv[argc-1], offsets.data(), err_msg)) {
         std::cerr<<err_msg.c_str()<<std::endl;
         return -1;
    }
    for (int i = 0; i < nfiles; ++i){
	std::cout<<offsets[i]<<":"<<argv[i+2]<<std::endl;
    }
    return 0;
}

int do_ptex_remerge(int argc, const char** argv) {
    std::string prog = strbasename(argv[0]);
    if (argc < 4) {
        std::cerr<<"usage: " << prog <<" merge search/dir texture.ptx\n";
	return -1;
    }
    Ptex::String err_msg;
    int status = ptex_remerge(argv[3], argv[2], err_msg);
    if (status) {
        std::cerr<<err_msg.c_str()<<"\n";
        return status;
    }
    return status;

}

int do_ptex_reverse(int argc, const char** argv) {
    std::string prog = strbasename(argv[0]);
    if (argc != 4) {
        std::cerr<<"Usage:"<<std::endl
                 <<prog
                 <<" reverse input.ptx output.ptx"<<std::endl;
        return -1;
    }
    Ptex::String err_msg;
    if (ptex_reverse(argv[2], argv[3], err_msg)) {
        std::cerr<<err_msg.c_str()<<std::endl;
        return -1;
    }
    return 0;
}

void constant_usage(const char* name) {
    std::cerr<<"Usage:\n"
             << strbasename(name)
             <<" constant [opts] mesh.obj output.ptx\n";
}
void constant_help() {
    std::cerr<<"Options: -t DATATYPE\n"
             <<"         --datatype DATATYPE\n"
             <<"           Ptex datatype: uint8, uint18, half or float\n"
             <<"         -n N\n"
             <<"         --channels N\n"
             <<"           Number of channels. [default 1]\n"
             <<"         -d float [float]\n"
             <<"         --data N float [float]\n"
             <<"           Data to fill ptex with [default 0]\n"
             <<"         -a N\n"
             <<"         --alphachannel N\n";
}

struct constant_options
{
    Ptex::DataType datatype = Ptex::dt_uint8;
    std::vector<float> data;
    unsigned int channels = 0;
    int alphachannel = -1;
    const char* objfile;
    const char* ptxfile;
};

int parse_constant_options(constant_options &o, int argc, const char** argv)
{
    if (argc < 4) {
        constant_usage(argv[0]);
        constant_help();
        return -1;
    }

    const char** opts = argv+2;
    const char** endopt = argv + argc;
    auto get_opt = [&]() ->const char* {
        if (opts == endopt)
            return 0;
        return opts[0];
    };
    auto next_opt = [&]()-> const char* {
        if (opts == endopt)
            return 0;
        return *(++opts);
    };
    auto int_opt = [&](int *o)->bool {
        char* end = 0;
        int v = strtol(opts[0], &end, 10);
        if (end[0] != '\0')
            return false;
        *o = v;
        return true;
    };
    auto double_opt = [&](double *o)->bool {
        char* end = 0;
        int v = strtod(opts[0], &end);
        if (end[0] != '\0')
            return false;
        *o = v;
        return true;
    };

    while(opts != endopt && opts[0][0] == '-') {
        std::string opt = opts[0];
        if (opt == "-t" || opt == "--datatype") {
            if (!next_opt()) {
                constant_usage(argv[0]);
                return -1;
            }
            std::string dt(get_opt());
            if (dt == "uint8")
                o.datatype = Ptex::dt_uint8;
            else if (dt == "uint16")
                o.datatype = Ptex::dt_uint16;
            else if (dt == "half" || dt == "float16")
                o.datatype = Ptex::dt_half;
            else if (dt == "float" || dt == "float32")
                o.datatype = Ptex::dt_float;
            else {
                std::cerr<<"Invalid datatype specified\n";
                return -1;
            }
        }
        else if(opt == "-n" || opt == "--channels") {
            int n = 0;
            if (!next_opt() || !int_opt(&n) || n <= 0) {
                std::cerr<<"Invalid number of channels\n";
                return -1;
            }
            o.channels = n;
        }
        else if (opt == "-a" || opt == "--alphachannel") {
            if (!next_opt() || !int_opt(&o.alphachannel) || o.alphachannel < -1) {
                std::cerr<<"Invalid alpha channel\n";
                return -1;
            }
        }
        else if(opt == "-d" || opt == "--data") {
            double v;
            bool status = false;
            while (next_opt() && (status = double_opt(&v))) {
                o.data.push_back(v);
            }
            if (!status)
                opts--;
            if (o.data.size() == 0) {
                std::cerr<<"No data specified\n";
                return -1;
            }

        }
        ++opts;
    }

    if (opts != endopt-2) {
        constant_usage(argv[0]);
        return -1;
    }
    o.objfile = *opts++;
    o.ptxfile = *opts++;

    if (o.channels == 0) {
        o.channels = o.data.size();
    }

    if (o.alphachannel != -1 && o.alphachannel >= (int) o.channels) {
        std::cerr<<"Alpha channel out of range\n";
        return -1;
    }

    if (o.data.size() == 0)
        o.data.push_back(0.0f);

    if (o.data.size() < o.channels) {
        float d = o.data.back();

        while (o.data.size() < o.channels) {
            o.data.push_back(d);
        }
    }
    return 0;
}

template <typename T>
T clamp(const T& n, const T& lower, const T& upper) {
  return std::max(lower, std::min(n, upper));
}

int do_ptex_constant(int argc, const char** argv) {
    constant_options opts;
    if (parse_constant_options(opts, argc, argv))
        return -1;

    std::vector<uint8_t> u8d;
    std::vector<uint16_t> u16d;

    void *data;
    if (opts.datatype == Ptex::dt_uint8) {
        for (float v : opts.data) {
            u8d.push_back(clamp((int) std::lround(v*255), 0,
                                (int) std::numeric_limits<uint8_t>::max()));
        }
        data = u8d.data();
    }
    else if (opts.datatype == Ptex::dt_uint16) {
        for (float v : opts.data) {
            u16d.push_back(clamp((int) std::lround(v*std::numeric_limits<uint16_t>::max()),
                                 0,
                                 (int) std::numeric_limits<uint16_t>::max()));
        }
        data = u16d.data();
    }
    else if (opts.datatype == Ptex::dt_half) {
        for (float v : opts.data) {
            PtexHalf h(v);
            u16d.push_back(h.bits);
        }
        data = u16d.data();
    }
    else {
        data = opts.data.data();
    }


    Ptex::String err_msg;

    obj_mesh mesh;
    if (parse_obj(opts.objfile, mesh, err_msg)) {
        std::cerr<<"Error reading "<< opts.objfile <<": "
                 <<err_msg.c_str()<<"\n";
        return -1;
    }

    if (make_constant(opts.ptxfile,
                      opts.datatype,
                      opts.channels,
                      opts.alphachannel, data,
                      mesh.nverts.size(), mesh.nverts.data(), mesh.verts.data(),
                      mesh.pos.data(), err_msg)) {
        std::cerr<<"Error creating "<<opts.ptxfile<<":"
                 <<err_msg.c_str()<<"\n";
        return -1;
    }
    return 0;
}

void usage(const char* name) {
    std::cerr<<"usage: " << strbasename(name) << " <command> [<args>]\n\n"
             <<"Commands are:\n"
             <<"   merge     Merge several textures into one\n"
             <<"   remerge   Update merged textures\n"
             <<"   reverse   Reverse winding order in ptex\n"
             <<"   constant  Create constant filled texture from obj file\n";
};

int main(int argc, const char** argv){
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    std::string tool = argv[1];
    if (tool == "merge")
        return do_ptex_merge(argc, argv);
    else if (tool == "remerge")
        return do_ptex_remerge(argc, argv);
    else if (tool == "reverse")
        return do_ptex_reverse(argc, argv);
    else if (tool == "constant") {
        return do_ptex_constant(argc, argv);
    }
    else {
        std::cerr<<"Unknown tool: "<<tool<<"\n";
        usage(argv[0]);
        return -1;
    }
}
