#include <Python.h>
#include "ptex_util.hpp"

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

static PyMethodDef ptexutils_methods [] = {
    { "merge_ptex", Py_merge_ptex, METH_VARARGS, "merge ptex files"},
    { "reverse_ptex", Py_reverse_ptex, METH_VARARGS, "reverse faces in ptex file"},
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
