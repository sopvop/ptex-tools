#include <cmath>
#include <limits>

#include <Python.h>

#include <PtexHalf.h>

#include "ptex_util.hpp"
#include "objreader.hpp"

using namespace ptex_tools;

static PyObject*
as_fs_string(PyObject *obj){
    PyObject* result = 0;
    if(PyBytes_Check(obj)){
        Py_INCREF(obj);
        result = obj;
    } else if(PyUnicode_Check(obj)){
        result = PyUnicode_AsEncodedString(obj, Py_FileSystemDefaultEncoding, NULL);
        if (!result)
            return 0;
        if (!PyBytes_Check(result)){
            PyErr_SetString(PyExc_TypeError, "encoder failed to return bytes");
            Py_DECREF(result);
            return 0;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "expected string or unicode filename");
        return 0;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(result);
    const char *data = PyBytes_AS_STRING(result);
    if (size != (Py_ssize_t) strlen(data)){
        PyErr_SetString(PyExc_TypeError, "embedded NUL character");
        Py_DECREF(result);
        return 0;
    }
    return result;
}

static PyObject*
Py_merge_ptex(PyObject *, PyObject* args){
    PyObject *input_list = 0, *seq = 0, *item = 0, *result = 0;
    PyObject **bytes_objects = 0;
    const char **input_files = 0;
    char *output = 0;
    int *offsets = 0;
    Py_ssize_t input_len;
    Ptex::String err_msg;
    int status;

    if(!PyArg_ParseTuple(args, "Oet:merge_ptex", &input_list,
                         Py_FileSystemDefaultEncoding,
                         &output))
	return 0;
    if (!PySequence_Check(input_list)) {
	PyErr_SetString(PyExc_ValueError, "first argument should be sequence of filepaths");
	goto exit;
    }
    seq = PySequence_Fast(input_list, "first argument should be sequence of filepaths");
    if (seq == 0)
	goto exit;
    input_len = PySequence_Length(seq);
    if (input_len < 2) {
	PyErr_SetString(PyExc_ValueError, "at least 2 input files required");
	goto exit;
    }

    input_files = (const char**) malloc(sizeof(char*)*input_len);
    bytes_objects = (PyObject **) malloc(sizeof(PyObject*) * input_len);
    memset(bytes_objects, 0, sizeof(PyObject*) * input_len);

    offsets = (int*) malloc(sizeof(int)*input_len);
    for (int i = 0; i < input_len; ++i){
	//borrowing, do not decref
	item = as_fs_string(PySequence_Fast_GET_ITEM(seq, i));
	if(!item){
	    PyErr_Format(PyExc_ValueError, "Input list element %i is not a string", i);
	    goto exit;
	}
	//internal ptr
        bytes_objects[i] = item;
	input_files[i] = PyBytes_AsString(item);
    }
    Py_BEGIN_ALLOW_THREADS
    status = ptex_merge((int) input_len, input_files, output, offsets, err_msg);
    Py_END_ALLOW_THREADS
    if (status){
	PyErr_SetString(PyExc_RuntimeError, err_msg.c_str());
	goto exit;
    }
    //now build result;
    result = PyList_New(input_len);
    for (int i = 0; i < input_len; ++i){
	item = PySequence_Fast_GET_ITEM(seq, i);
	item = Py_BuildValue("Oi", item, offsets[i]); //increfs item
	PyList_SetItem(result, i, item); //steals item
    }
  exit:
    if (bytes_objects) {
        for(int i = 0; i < input_len; ++i){
            Py_XDECREF(bytes_objects[i]);
        }
    }
    PyMem_Free(output);
    free(bytes_objects);
    free(input_files);
    free(offsets);
    Py_XDECREF(seq);
    return result;
}

static PyObject*
Py_reverse_ptex(PyObject *, PyObject* args){
    char *input = 0;
    char *output = 0;
    Ptex::String err_msg;
    int status;
    if(!PyArg_ParseTuple(args, "etet:reverse_ptex",
                         Py_FileSystemDefaultEncoding, &input,
                         Py_FileSystemDefaultEncoding, &output))
	return 0;
    Py_BEGIN_ALLOW_THREADS
    status = ptex_reverse(input, output, err_msg);
    Py_END_ALLOW_THREADS
    PyMem_Free(input);
    PyMem_Free(output);
    if (status){
	PyErr_SetString(PyExc_RuntimeError, err_msg.c_str());
        return 0;
    }
    Py_RETURN_NONE;
}

template <typename Conv, typename Vec, typename T>
static
int read_sequence(PyObject *seq, Conv conv, Vec & vec, T) {
    Py_ssize_t len = PySequence_Length(seq);
    vec.reserve(len);
    for (Py_ssize_t i = 0; i < len; ++i) {
        PyObject* item = PySequence_Fast_GET_ITEM(seq, i);
        T v = conv(item);
        if (PyErr_Occurred())
            return -1;
        vec.push_back(v);
    }
    return 0;
}

template <typename T>
T clamp(const T& n, const T& lower, const T& upper) {
  return std::max(lower, std::min(n, upper));
}
static PyObject*
Py_make_constant(PyObject *, PyObject* args, PyObject *kws) {
    char *output = 0;

    char *cformat = 0;

    int alphachan = -1;

    PyObject *data = 0, *nverts = 0, *verts = 0, *pos = 0;

    PyObject *sdata = 0, *snverts = 0, *sverts = 0, *spos = 0;

    static const char *keywords[] = { "filename", "format", "data", "nverts", "verts", "pos",
                                      "alphachannel", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kws, "etetOOOO|i:make_constant",
                                    (char **) keywords,
                                    Py_FileSystemDefaultEncoding, &output,
                                    Py_FileSystemDefaultEncoding, &cformat,
                                    &data, &nverts, &verts, &pos, &alphachan))
        return 0;

    obj_mesh mesh;

    Ptex::DataType dt;
    Ptex::String err_msg;
    std::vector<float> vdata;
    std::vector<uint8_t> vdata8;
    std::vector<uint16_t> vdata16;
    int nchans = 0;
    void *ptx_data;
    std::string format(cformat);

    bool err = 0;

    sdata = PySequence_Fast(data, "third argument should be sequence of floats");
    if (sdata == 0)
	goto exit;

    snverts = PySequence_Fast(nverts, "nverts should be sequence of ints");
    if (sdata == 0)
	goto exit;

    sverts = PySequence_Fast(verts, "verts should be sequence of ints");
    if (sdata == 0)
	goto exit;

    spos = PySequence_Fast(pos, "pos should be sequence of floats");
    if (sdata == 0)
	goto exit;

    err = read_sequence(snverts,  PyInt_AsLong, mesh.nverts, (long) 1);
    if (err) {
	PyErr_SetString(PyExc_TypeError, "nverts should be ints");
        goto exit;
    }
    err = read_sequence(sverts,  PyInt_AsLong, mesh.verts, (long) 1);
    if (err) {
	PyErr_SetString(PyExc_TypeError, "verts should be ints");
        goto exit;
    }

    err = read_sequence(spos, PyFloat_AsDouble, mesh.pos, 1.0);
    if (err) {
	PyErr_SetString(PyExc_TypeError, "pos should be floats");
        goto exit;
    }

    err = read_sequence(sdata, PyFloat_AsDouble, vdata, 1.0);
    if (err) {
	PyErr_SetString(PyExc_TypeError, "data should be floats");
        goto exit;
    }

    if (format == "uint8") {
        dt = Ptex::dt_uint8;
        for (float v : vdata) {
            vdata8.push_back(clamp((int) std::lround(v*255), 0,
                                (int) std::numeric_limits<uint8_t>::max()));
        }
        ptx_data = vdata8.data();
    }
    else if (format == "uint16") {
        dt = Ptex::dt_uint16;
        for (float v : vdata) {
            vdata16.push_back(clamp((int) std::lround(v*std::numeric_limits<uint16_t>::max()),
                                    0,
                                    (int) std::numeric_limits<uint16_t>::max()));
        }
        ptx_data = vdata16.data();

    }
    else if (format == "float16" || format == "half") {
        dt = Ptex::dt_half;
        for (float v : vdata) {
            PtexHalf h(v);
            vdata16.push_back(h.bits);
        }
        ptx_data = vdata16.data();
    }
    else if (format == "float" || format == "float32") {
        dt = Ptex::dt_float;
        ptx_data = vdata.data();
    }
    else {
        err = -1;
        PyErr_SetString(PyExc_ValueError,
                        "format should be uint8, uint16, half (float16) or float (float32)");
        goto exit;
    }

    if (check_consistency(mesh, err_msg)) {
        err = -1;
        PyErr_Format(PyExc_ValueError,
                     "Mesh has inconsistent data: %s", err_msg.c_str());
        goto exit;
    }

    nchans = vdata.size();

    if (alphachan < -1 || alphachan > nchans) {
        err = -1;
        PyErr_SetString(PyExc_ValueError, "invalid alpha channel specified");
        goto exit;
    }

    Py_BEGIN_ALLOW_THREADS;
    err = ptex_tools::make_constant(output, dt, nchans, alphachan,
                                    ptx_data,
                                    mesh.nverts.size(), mesh.nverts.data(), mesh.verts.data(),
                                    mesh.pos.data(), err_msg);
    Py_END_ALLOW_THREADS;
    if (err) {
        PyErr_SetString(PyExc_RuntimeError, err_msg.c_str());
    }
  exit:
    PyMem_Free(output);
    PyMem_Free(cformat);
    Py_XDECREF(sdata);
    Py_XDECREF(snverts);
    Py_XDECREF(sverts);
    Py_XDECREF(spos);
    if (err)
        return 0;
    Py_RETURN_NONE;
}

static PyMethodDef ptexutils_methods [] = {
    { "merge_ptex", Py_merge_ptex, METH_VARARGS, "merge ptex files"},
    { "reverse_ptex", Py_reverse_ptex, METH_VARARGS, "reverse faces in ptex file"},
    { "make_constant", (PyCFunction) Py_make_constant, METH_VARARGS | METH_KEYWORDS,
      "create constant ptex file"},
    { NULL, NULL, 0, NULL }
};

extern "C" {

PyMODINIT_FUNC
initcptexutils(void){
   PyObject *m;
   m = Py_InitModule3("cptexutils", ptexutils_methods, "");
   if (m == NULL)
       return;
}
}
