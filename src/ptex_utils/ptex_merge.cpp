#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

#include "objreader.hpp"

#include "ptexutils.hpp"
#include "helpers.hpp"

struct InputInfo {
    Ptex::DataType data_type;
    Ptex::MeshType mesh_type;
    int alpha_channel;
    int num_channels;
    int num_faces;
    bool merge_mesh;
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
int check_ptx(const InputInfo &info,
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
    if (ptex->numChannels() != info.num_channels){
	std::string err = std::string("Number of channels does not match wifh first file: ")
	    + ptex->path();
	err_msg = err.c_str();
	return -1;
    }

    if (ptex->alphaChannel() != info.alpha_channel){
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
    if (check_ptx(info, ptex.get(), err_msg))
        return -1;
    int mesh_offset = 0;
    if (info.merge_mesh) {
        mesh_offset = info.mesh.nverts.size();
        int nfaces = append_mesh(info.mesh, ptex.get());
        info.merge_mesh = nfaces > 0;
    }

    int nf = ptex->numFaces();
    info.add(info.num_faces, mesh_offset, ptex);
    info.num_faces += nf;

    return 0;
}

static
int append_ptexture(const InputInfo &info, PtexWriter *writer, int offset, PtexTexture *ptex) {

    int num_faces = ptex->numFaces();
    int nchannels = ptex->numChannels();
    Ptex::DataType data_type = ptex->dataType();
    int data_block_size = Ptex::DataSize(data_type) * nchannels;
    int out_block_size = Ptex::DataSize(info.data_type)*nchannels;

    std::vector<char> data;
    data.resize(data_block_size*1024);

    bool do_convert = info.data_type != ptex->dataType();
    std::vector<float> fdata;
    std::vector<char> odata;

    if (do_convert) {
        fdata.resize(nchannels*1024);
        odata.resize(out_block_size*1024);
    }
    for (int i = 0; i < num_faces; ++i) {
	Ptex::FaceInfo face_info = ptex->getFaceInfo(i);
	//copy dat shit
	Ptex::FaceInfo outf = face_info;
	outf.adjfaces[0] = outf.adjfaces[0] == -1 ? -1 : outf.adjfaces[0] + offset;
	outf.adjfaces[1] = outf.adjfaces[1] == -1 ? -1 : outf.adjfaces[1] + offset;
	outf.adjfaces[2] = outf.adjfaces[2] == -1 ? -1 : outf.adjfaces[2] + offset;
	outf.adjfaces[3] = outf.adjfaces[3] == -1 ? -1 : outf.adjfaces[3] + offset;

	size_t req_size = outf.res.size()*data_block_size;
	if (req_size > data.size()){
	    data.resize(req_size);
            if (do_convert) {
                fdata.resize(outf.res.size()*nchannels);
                odata.resize(outf.res.size()*out_block_size);
            }
	}
        // This is slow and bad
        if (do_convert) {
            ptex->getData(i, data.data(), 0);
            Ptex::ConvertToFloat(fdata.data(), data.data(), data_type,
                                 nchannels*outf.res.size());
            Ptex::ConvertFromFloat(odata.data(), fdata.data(),
                                   info.data_type, nchannels*outf.res.size());
            if (face_info.isConstant())
                writer->writeConstantFace(offset+i, outf, odata.data());
            else
                writer->writeFace(offset+i, outf, odata.data());
        }
        else {
            ptex->getData(i, data.data(), 0);
            if (face_info.isConstant())
                writer->writeConstantFace(offset+i, outf, data.data());
            else
                writer->writeFace(offset+i, outf, data.data());
        }
    }
    return 0;
}


int ptex_utils::ptex_merge(int nfiles, const char** files,
                           const char*output_file, int *offsets,
                           Ptex::String &err_msg){

    InputInfo info;
    if (nfiles < 2){
	err_msg = "At least 2 files needed";
	return -1;
    }
    if (output_file == 0){
	err_msg = "Output file is null";
	return -1;
    }
    PtxPtr first(PtexTexture::open(files[0], err_msg, 0));
    if (!first) {
	return -1;
    }
    info.data_type = first->dataType();
    info.mesh_type = first->meshType();
    info.num_channels = first->numChannels();
    info.alpha_channel = first->alphaChannel();
    info.num_faces = first->numFaces();
    int nfaces = append_mesh(info.mesh, first.get());
    info.merge_mesh = nfaces > 0;
    info.add(0, 0, first);

    for (int i = 1; i < nfiles; i++){
	if (append_input(info, files[i], err_msg))
	    return -1;
    }
    PtexWriter *writer = PtexWriter::open(output_file,
					  info.mesh_type,
					  info.data_type,
					  info.num_channels,
					  info.alpha_channel,
					  info.num_faces,
					  err_msg);
    if (!writer)
	return -1;
    for (int i = 0; i < nfiles; ++i)
    {
	if(append_ptexture(info, writer, info.offsets[i], info.ptexes[i].get()))
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

    info.data_type     = ptx->dataType();
    info.mesh_type     = ptx->meshType();
    info.num_channels  = ptx->numChannels();
    info.alpha_channel = ptx->alphaChannel();
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
            if (check_ptx(info, ptex.get(), err_msg))
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
                                      info.mesh_type,
                                      info.data_type,
                                      info.num_channels,
                                      info.alpha_channel,
                                      info.num_faces,
                                      err_msg));
    if (!writer)
	return -1;

    for (size_t i = 0; i < names.size(); ++i) {
        if (info.ptexes[i]) {
            if (append_ptexture(info, writer.get(), info.offsets[i], info.ptexes[i].get())) {
                return -1;
            }
        }
    }
    if(!writer->close(err_msg)) {
       return -1;
    }
    return 0;
}
