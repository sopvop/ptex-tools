#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#include "objreader.hpp"

#include "PtexUtils.h"
#include "ptexutils.hpp"
#include "helpers.hpp"

using PtexMergeOptions = ptex_utils::PtexMergeOptions;

struct InputInfo {
    PtexMergeOptions options;

    int num_faces = 0;

    bool merge_mesh = true;
    obj_mesh mesh;

    std::vector<PtxPtr> ptexes;
    std::vector<int32_t> offsets;
    std::vector<int32_t> mesh_offsets;
    void add(int32_t offset, int32_t mesh_offset, PtxPtr & p) {
        offsets.push_back(offset);
        mesh_offsets.push_back(mesh_offset);
        ptexes.emplace_back(std::move(p));
    }
};

static
int append_mesh(obj_mesh &mesh, PtexTexture *tex) {
    MetaPtr meta(tex->getMetaData());

    const int32_t *nverts;
    const int32_t *verts;
    const float *pos;
    int face_count, verts_count, pos_count;
    meta->getValue("PtexFaceVertCounts", nverts, face_count);
    if (nverts == 0)
        return 0;
    meta->getValue("PtexFaceVertIndices", verts, verts_count);
    if (verts == 0)
        return 0;
    meta->getValue("PtexVertPositions", pos, pos_count);
    if (pos == 0)
        return 0;

    mesh.nverts.insert(end(mesh.nverts), nverts, nverts+face_count);
    int offset = mesh.pos.size()/3;

    mesh.verts.reserve(mesh.verts.size() + verts_count);

    std::transform(verts, verts+verts_count, std::back_inserter(mesh.verts),
                   [&](int32_t v)->int32_t { return v+offset; });
    mesh.pos.insert(end(mesh.pos), pos, pos+pos_count);

    return face_count;
}

static
int check_ptx(const PtexMergeOptions &info,
              PtexTexture *ptex,
              Ptex::String &err_msg) {
    /*
    if (ptex->dataType() != info.data_type){
	std::string err = std::string("Data type does not match with first file: ")
	    + ptex->path()
	    + " expected: " + Ptex::DataTypeName(info.data_type)
	    + " got:" + Ptex::DataTypeName(ptex->dataType());
	err_msg = err.c_str();
	return -1;
    }
    */
    if (ptex->meshType() != info.mesh_type){
	std::string err = std::string("Mesh type does not match with first file: ")
	    + ptex->path();
	err_msg = err.c_str();
	return -1;
    }
    if (ptex->numChannels() < info.num_channels){
	std::string err = std::string("Not enough channels in file: ")
	    + ptex->path();
	err_msg = err.c_str();
	return -1;
    }

    if (ptex->alphaChannel() != info.alpha_channel && info.alpha_channel != -1){
	std::string err = std::string("Alpha channel does not match wifh first file: ")
	    + ptex->path();
	err_msg = err.c_str();
	return -1;
    }
    return 0;
}

static
int append_input(InputInfo &info, const char* filename, Ptex::String &err_msg) {
    PtxPtr ptex(PtexTexture::open(filename, err_msg, 0));
    if (!ptex) {
	return -1;
    }
    if (check_ptx(info.options, ptex.get(), err_msg))
        return -1;
    int mesh_offset = 0;
    if (info.merge_mesh) {
        mesh_offset = info.mesh.nverts.size();
        int nfaces = append_mesh(info.mesh, ptex.get());
        info.options.merge_mesh = nfaces > 0;
    }

    int nf = ptex->numFaces();
    info.add(info.num_faces, mesh_offset, ptex);
    info.num_faces += nf;

    return 0;
}

static
int append_ptexture(const PtexMergeOptions &info,
                    PtexWriter *writer,
                    const int offset,
                    PtexTexture *ptex) {

    const int num_faces = ptex->numFaces();
    const int nchannels = ptex->numChannels();
    const Ptex::DataType data_type = ptex->dataType();

    const int data_pixel_size = Ptex::DataSize(data_type) * nchannels;
    const int stripped_pixel_size = Ptex::DataSize(data_type) * info.num_channels;
    const int out_pixel_size = Ptex::DataSize(info.data_type)*info.num_channels;

    std::vector<char> data;
    data.resize(data_pixel_size*1024);

    const bool strip_chans = nchannels > info.num_channels;

    const bool do_convert = info.data_type != ptex->dataType();

    std::vector<float> fdata;
    std::vector<char> odata;

    if (do_convert) {
        fdata.resize(info.num_channels*1024);
        odata.resize(out_pixel_size*1024);
    }

    for (int i = 0; i < num_faces; ++i) {
	Ptex::FaceInfo outf = ptex->getFaceInfo(i);
	outf.adjfaces[0] = outf.adjfaces[0] == -1 ? -1 : outf.adjfaces[0] + offset;
	outf.adjfaces[1] = outf.adjfaces[1] == -1 ? -1 : outf.adjfaces[1] + offset;
	outf.adjfaces[2] = outf.adjfaces[2] == -1 ? -1 : outf.adjfaces[2] + offset;
	outf.adjfaces[3] = outf.adjfaces[3] == -1 ? -1 : outf.adjfaces[3] + offset;

	size_t req_size = outf.res.size()*data_pixel_size;
	if (req_size > data.size()){
	    data.resize(req_size);
            if (do_convert) {
                fdata.resize(outf.res.size()*info.num_channels);
                odata.resize(outf.res.size()*out_pixel_size);
            }
	}
        // This is slow and bad
        ptex->getData(i, data.data(), 0);
        if (strip_chans && !outf.isConstant()) {
            char *dst = data.data()+stripped_pixel_size;
            char *src = data.data()+data_pixel_size;
            char *end = data.data()+stripped_pixel_size*outf.res.size();
            while (dst != end) {
                std::memmove(dst, src, stripped_pixel_size);
                dst += stripped_pixel_size;
                src += data_pixel_size;
            }
        }
        if (do_convert) {
            Ptex::ConvertToFloat(fdata.data(), data.data(), data_type,
                                 info.num_channels*outf.res.size());
            Ptex::ConvertFromFloat(odata.data(), fdata.data(),
                                   info.data_type, info.num_channels*outf.res.size());
            if (outf.isConstant())
                writer->writeConstantFace(offset+i, outf, odata.data());
            else
                writer->writeFace(offset+i, outf, odata.data(), 0);
        }
        else {
            if (outf.isConstant())
                writer->writeConstantFace(offset+i, outf, data.data());
            else
                writer->writeFace(offset+i, outf, data.data());
        }
    }
    return 0;
}


int ptex_utils::ptex_merge(int nfiles, const char** files,
                           const char*output_file, int *offsets,
                           Ptex::String &err_msg)
{
    PtexMergeOptions options;
    if (nfiles > 0) {
        PtxPtr first(PtexTexture::open(files[0], err_msg, 0));
        if (!first) {
            return -1;
        }
        options.data_type = first->dataType();
        options.mesh_type = first->meshType();
        options.num_channels = first->numChannels();
        options.alpha_channel = first->alphaChannel();

        options.u_border_mode = first->uBorderMode();
        options.v_border_mode = first->vBorderMode();
    }
    return ptex_utils::ptex_merge(options, nfiles, files, output_file,
                                  offsets, err_msg);

}

int ptex_utils::ptex_merge(const PtexMergeOptions & opts,
                           int nfiles, const char** files,
                           const char*output_file, int *offsets,
                           Ptex::String &err_msg){

    InputInfo info;
    info.options = opts;

    if (nfiles < 2){
	err_msg = "At least 2 files needed";
	return -1;
    }
    if (output_file == 0){
	err_msg = "Output file is null";
	return -1;
    }
    for (int i = 0; i < nfiles; i++){
	if (append_input(info, files[i], err_msg))
	    return -1;
    }
    PtexWriter *writer = PtexWriter::open(output_file,
					  info.options.mesh_type,
					  info.options.data_type,
					  info.options.num_channels,
					  info.options.alpha_channel,
					  info.num_faces,
					  err_msg);
    if (!writer)
	return -1;
    for (int i = 0; i < nfiles; ++i)
    {
	if(append_ptexture(info.options, writer, info.offsets[i], info.ptexes[i].get()))
           return -1;
    }
    std::copy(begin(info.offsets), end(info.offsets), offsets);

    if (info.merge_mesh) {
        writer->writeMeta("PtexFaceVertCounts",
                          info.mesh.nverts.data(),
                          info.mesh.nverts.size());
        writer->writeMeta("PtexFaceVertIndices",
                          info.mesh.verts.data(),
                          info.mesh.verts.size());
        writer->writeMeta("PtexVertPositions",
                          info.mesh.pos.data(),
                          info.mesh.pos.size());
    }

    std::vector<std::string> names;
    std::transform(files, files+nfiles, std::back_inserter(names),
                   [](const char* f) { return strbasename(f); });
    size_t joined_size = std::accumulate(begin(names), end(names), 0,
                                         [](size_t acc, const std::string &s) {
                                             return acc+s.size();
                                         });
    joined_size += names.size()-1;
    std::string joined;
    joined.reserve(joined_size);
    joined.append(names[0]);
    for (auto it = begin(names)+1; it != end(names); ++it) {
        joined.push_back(':');
        joined.append(*it);
    }
    writer->writeMeta("PtexMergedFiles", joined.c_str());
    writer->writeMeta("PtexMergedOffsets", info.offsets.data(), info.offsets.size());
    if (info.merge_mesh) {
        writer->writeMeta("PtexMergedMeshOffsets",
                          info.mesh_offsets.data(),
                          info.mesh_offsets.size());
    }

    if (!writer->close(err_msg)){
	writer->release();
	return -1;
    }
    writer->release();
    return 0;
}

static
void split_names(const char* str, std::vector<std::string> &names) {
    const char* end;
    end = std::strchr(str, ':');
    while (str[0] && end) {
        names.emplace_back(str, end-str);
        str = end+1;
        end = std::strchr(str, ':');
    }
    if (str[0])
        names.push_back(str);
}

inline double mtime(const struct stat & st) {
    return st.st_mtim.tv_sec + st.st_mtim.tv_nsec/1E9;
};

static
int parse_remerge(InputInfo &info,
                  const char *file,
                  const char *searchdir,
                  std::vector<std::string> &names,
                  Ptex::String &err_msg)
{
    PtxPtr ptx(PtexTexture::open(file, err_msg, 0));
    if (!ptx) {
	return -1;
    }

    struct stat stat_info;

    if(stat( file, &stat_info ) != 0 ) {
        err_msg = "Can't stat file";
        return -1;
    }
    double dtime = mtime(stat_info);

    info.options.data_type     = ptx->dataType();
    info.options.mesh_type     = ptx->meshType();
    info.options.num_channels  = ptx->numChannels();
    info.options.alpha_channel = ptx->alphaChannel();
    info.num_faces     = ptx->numFaces();

    MetaPtr meta(ptx->getMetaData());

    const char* filenames;
    meta->getValue("PtexMergedFiles", filenames);
    if (!filenames) {
        err_msg = "PtexMergedFiles meta not set, probably not a merged file";
        return -1;
    }

    split_names(filenames, names);

    const int32_t *offsets;
    int noffsets;
    meta->getValue("PtexMergedOffsets", offsets, noffsets);
    if (!offsets) {
        err_msg = "PtexMergedOffsets meta not set, probably not a merged file";
        return -1;
    }
    if ((size_t) noffsets != names.size()) {
        err_msg = "Number of offsets and file names in meta does not match";
        return -1;
    }
    std::string dir(searchdir);
    for (size_t i = 0; i < names.size(); ++i) {
        std::string full_name = dir + "/" + names[i];
        if(stat(full_name.c_str(), &stat_info ) == 0
           && mtime(stat_info) > dtime)
        {
            PtxPtr ptex(PtexTexture::open(full_name.c_str(), err_msg, 0));
            if (!ptex) {
                return -1;
            }
            if (check_ptx(info.options, ptex.get(), err_msg))
                return 2;
            if ( (i != names.size() -1 && offsets[i+1]-offsets[i] < ptex->numFaces())
                 ||  (info.num_faces - offsets[i]) < ptex->numFaces())
            {
                err_msg = Ptex::String("Ptex number of faces is more than in merged: ")
                    + ptex->path();
                return 2;
            }
            info.add(offsets[i], 0, ptex);
        } else {
            PtxPtr p;
            info.add(0,0,p);
        }
    }
    return 0;
}

int ptex_utils::ptex_remerge(const char *file,
                             const char *searchdir,
                             Ptex::String &err_msg)
{
    InputInfo info;
    std::vector<std::string> names;

    int status = parse_remerge(info, file, searchdir, names, err_msg);
    if (status)
        return status;

    WriterPtr writer(PtexWriter::edit(file, false,
                                      info.options.mesh_type,
                                      info.options.data_type,
                                      info.options.num_channels,
                                      info.options.alpha_channel,
                                      info.num_faces,
                                      err_msg));
    if (!writer)
	return -1;

    for (size_t i = 0; i < names.size(); ++i) {
        if (info.ptexes[i]) {
            if (append_ptexture(info.options, writer.get(), info.offsets[i],
                                info.ptexes[i].get()))
            {
                return -1;
            }
        }
    }
    if(!writer->close(err_msg)) {
       return -1;
    }
    return 0;
}
