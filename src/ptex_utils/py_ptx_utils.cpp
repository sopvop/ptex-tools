#include <Python.h>
#include "ptex_util.hpp"

static PyObject*
Py_merge_ptex(PyObject *, PyObject* args){
    PyObject *input_list = 0, *seq, *item, *result = 0;
    const char **input_files = 0;
    const char *output = 0;
    int *offsets;
    Ptex::String err_msg;

    if(!PyArg_ParseTuple(args, "Os:merge_ptex", &input_list, &output))
	return 0;
    if (!PySequence_Check(input_list)) {
	PyErr_SetString(PyExc_ValueError, "first argument should be sequence of filepaths");
	return 0;
    }
    seq = PySequence_Fast(input_list, "first argument should be sequence of filepaths");
    if (seq == 0)
	return 0;
    Py_ssize_t input_len = PySequence_Length(seq);
    if (input_len < 2) {
	PyErr_SetString(PyExc_ValueError, "at least 2 input files required");
	Py_DECREF(seq);
	return 0;
    }

    input_files = (const char**) malloc(sizeof(char*)*input_len);
    offsets = (int*) malloc(sizeof(int)*input_len);

    for (int i = 0; i < input_len; ++i){
	//borrowing, do not decref
	item = PySequence_Fast_GET_ITEM(seq, i);
	if(!PyString_Check(item)){
	    PyErr_Format(PyExc_ValueError, "Input list element %i is not a string", i);
	    goto exit;
	}
	//internal ptr
	input_files[i] = PyString_AsString(item);
    }
    if (ptex_merge((int) input_len, input_files, output, offsets, err_msg)){
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
    free(input_files);
    free(offsets);
    Py_DECREF(seq);
    return result;
}

static PyObject*
Py_reverse_ptex(PyObject *, PyObject* args){
    const char *input = 0;
    const char *output = 0;
    Ptex::String err_msg;
    if(!PyArg_ParseTuple(args, "ss:reverse_ptex", &input, &output))
	return 0;
    if (ptex_reverse(input, output, err_msg)){
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
