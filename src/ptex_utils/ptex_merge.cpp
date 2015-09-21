#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "objreader.hpp"

#include "PtexUtils.h"
#include "ptexutils.hpp"
#include "helpers.hpp"

using PtexMergeOptions = ptex_utils::PtexMergeOptions;
namespace fs = boost::filesystem;
namespace sys = boost::system;

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
    if (ptex->meshType() != info.mesh_type){
	std::string err = std::string("Mesh type does not match: ")
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
	std::string err = std::string("Alpha channel does not match: ")
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
        err_msg = std::string("Opening input file: ") + filename +
            ":" + std::string(err_msg.c_str());
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
            err_msg = std::string(files[0]) + ":"+ std::string(err_msg.c_str());
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

static
fs::path strip_prefix(const fs::path & path, const fs::path &prefix)
{
    auto i = std::begin(path);
    auto iend = std::end(path);
    auto p = std::begin(prefix);
    auto pend = std::end(prefix);
    for (; i != iend && p != pend; ++i, ++p)
    {
        if (*i == *p)
            continue;
        else
            break;
    }
    if (p != pend)
        return path;

    fs::path res;
    for (; i != iend; ++i)
        res /= *i;
    return res;
}

static
void write_meta_block(PtexWriter *writer, ptex_utils::PtexMeta *meta)
{
    for (int32_t i = 0; i < meta->size; ++i) {
        Ptex::MetaDataType t = meta->types[i];
        const char* key = meta->keys[i];
        int count = meta->counts[i];
        const void *data = meta->data[i];
        switch (t) {
        case Ptex::mdt_string:
            writer->writeMeta(key, (const char*) data);
            break;
        case Ptex::mdt_int8:
            writer->writeMeta(key, (const int8_t*) data, count);
            break;
        case Ptex::mdt_int16:
            writer->writeMeta(key, (const int16_t*) data, count);
            break;
        case Ptex::mdt_int32:
            writer->writeMeta(key, (const int32_t*) data, count);
            break;
        case Ptex::mdt_float:
            writer->writeMeta(key, (const float*) data, count);
            break;
        case Ptex::mdt_double:
            writer->writeMeta(key, (const double*) data, count);
            break;
        }
    }

}

int ptex_utils::ptex_merge(const PtexMergeOptions & opts,
                           int nfiles, const char** files,
                           const char*output_file, int *offsets,
                           Ptex::String &err_msg){

    InputInfo info;
    info.options = opts;

    if (nfiles < 1){
	err_msg = "At least one file required";
	return -1;
    }

    if (output_file == 0){
	err_msg = "Output file is null";
	return -1;
    }

    fs::path root = fs::absolute(opts.root ? fs::path(opts.root) : fs::current_path());

    fs::path outpath = fs::absolute(output_file, root);

    std::vector<fs::path> filepaths;
    for (int i = 0; i < nfiles; ++i) {
        fs::path p = fs::absolute(files[i], root);
        filepaths.push_back(p);
    }

    for (int i = 0; i < nfiles; i++){
	if (append_input(info, filepaths[i].string().c_str(), err_msg))
	    return -1;
    }

    WriterPtr writer(PtexWriter::open(output_file,
                                      info.options.mesh_type,
                                      info.options.data_type,
                                      info.options.num_channels,
                                      info.options.alpha_channel,
                                      info.num_faces,
                                      err_msg));
    if (!writer) {
        err_msg = "Can't open for writing " + std::string(output_file) + ":" + err_msg;
	return -1;
    }
    for (int i = 0; i < nfiles; ++i)
    {
        if (opts.callback) {
            bool stop = opts.callback(i, opts.callback_data);
            if (stop) {
                err_msg = "Interrupted";
                return -1;
            }
        }
	if(append_ptexture(info.options, writer.get(), info.offsets[i], info.ptexes[i].get()))
           return -1;
    }

    if (offsets) {
        std::copy(begin(info.offsets), end(info.offsets), offsets);
    }

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
    std::transform(std::begin(filepaths), std::end(filepaths),std::back_inserter(names),
                   [&](const fs::path p) {
                       return strip_prefix(p, root).string();
                   });
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
    if (opts.meta)
        write_meta_block(writer.get(), opts.meta);
    if (!writer->close(err_msg)){
        err_msg = "Closing writer " + std::string(output_file) + ":" + err_msg.c_str();
	return -1;
    }
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

    sys::error_code ec;
    std::time_t dtime = fs::last_write_time(file, ec);
    if (ec) {
        err_msg = ec.message().c_str();
        return -1;
    }

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
    fs::path dir(searchdir);
    for (size_t i = 0; i < names.size(); ++i) {

        fs::path full_name = dir / names[i];

        std::time_t md = fs::last_write_time(full_name, ec);
        if (!ec && md > dtime)
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

    if (std::all_of(std::begin(info.ptexes),
                    std::end(info.ptexes),
                    [](const PtxPtr & p) -> bool { return !p; })) {
        return 0;
    }
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
