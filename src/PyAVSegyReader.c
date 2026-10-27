#include "PyAVSegyReader.h"
#include "AVSegyCommon.h"
#include "AVSegyReader.h"
#include "PyAVSegyBinaryHeader.h"
#include "PyAVSegyError.h"
#include "Python.h"
#include "avsegy.h"
#define PY_ARRAY_UNIQUE_SYMBOL AVSEGY_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

static PyObject *make_numpy_array(int format_code, uint64_t trc_num,
                                  int32_t samp_num);
static PyObject *read_fixed_samp(AVSegyReader *sgy, Py_ssize_t start,
                                 Py_ssize_t stop);
static PyObject *read_var_samp(AVSegyReader *sgy, Py_ssize_t start,
                               Py_ssize_t stop);
static PyObject *read_fixed_hdrs(AVSegyReader *sgy, PyObject *hdr_names,
                                 Py_ssize_t start, Py_ssize_t stop);
static PyObject *read_var_hdrs(AVSegyReader *sgy, PyObject *hdr_names,
                               Py_ssize_t start, Py_ssize_t stop);
static PyObject *read_fixed_both(AVSegyReader *sgy, PyObject *hdr_names,
                                 Py_ssize_t start, Py_ssize_t stop);
static PyObject *read_var_both(AVSegyReader *sgy, PyObject *hdr_names,
                               Py_ssize_t start, Py_ssize_t stop);

static void PyAVSegyReaderSlice_dealloc(PyAVSegyReaderSlice *self) {
  PyTypeObject *tp = Py_TYPE(self);
  Py_XDECREF(self->reader);
  Py_XDECREF(self->hdr_names);
  tp->tp_free((PyObject *)self);
  Py_DECREF(tp);
}

static PyObject *PyAVSegyReaderSlice_subscript(PyObject *self, PyObject *key) {
  PyAVSegyReaderSlice *slice = (PyAVSegyReaderSlice *)self;
  AVSegyReader *sgy = slice->reader->sgy;
  if (!sgy) {
    PyErr_SetString(PyExc_ValueError, "SEGY is closed");
    return NULL;
  }
  AVSegyErr const *err = av_segy_reader_get_error(sgy);
  if (av_segy_reader_rewind(sgy)) {
    py_av_segy_check_error(err);
    return NULL;
  }
  if (!PySlice_Check(key)) {
    PyErr_SetString(PyExc_TypeError, "indices must be slices");
    return NULL;
  }

  Py_ssize_t start, stop, step;
  if (PySlice_Unpack((PyObject *)key, &start, &stop, &step) < 0)
    return NULL;
  if (step != 1) {
    PyErr_SetString(PyExc_ValueError, "slice step must be 1");
    return NULL;
  }
  if (start < 0 || stop < -1) {
    PyErr_SetString(PyExc_ValueError, "slice start value must be positive, "
                                      "stop value canmust be positive or -1");
    return NULL;
  }

  PyObject *hdr_names = slice->hdr_names;
  PyObject *result;
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  switch (slice->kind) {
  case AV_SEGY_READ_SAMPLES:
    if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver)
      result = read_fixed_samp(sgy, start, stop);
    else
      result = read_var_samp(sgy, start, stop);
    break;
  case AV_SEGY_READ_HEADERS:
    if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver)
      result = read_fixed_hdrs(sgy, hdr_names, start, stop);
    else
      result = read_var_hdrs(sgy, hdr_names, start, stop);
    break;
  case AV_SEGY_READ_ALL:
    if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver)
      result = read_fixed_both(sgy, hdr_names, start, stop);
    else
      result = read_var_both(sgy, hdr_names, start, stop);
    break;
  default:
    PyErr_SetString(PyExc_RuntimeError, "Internal error unknown reading type");
    return NULL;
  }

  return result;
}

static PyType_Slot PyAVSegyReaderSlice_slots[] = {
    {Py_tp_dealloc, PyAVSegyReaderSlice_dealloc},
    {Py_mp_subscript, PyAVSegyReaderSlice_subscript},
    {0, NULL},
};

PyType_Spec PyAVSegyReaderSlice_Spec = {
    .name = "avsegy.ReaderSlice",
    .basicsize = sizeof(PyAVSegyReaderSlice),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = PyAVSegyReaderSlice_slots,
};

static PyObject *make_PyAVSegyReaderSlice(PyAVSegyReader *reader,
                                          AVSegyReaderReadKind kind,
                                          PyObject *hdr_names) {
  PyObject *module = PyType_GetModule(Py_TYPE(reader));
  avsegy_state *state = avsegy_get_state(module);
  if (!state || !state->PyAVSegyReaderSliceType) {
    PyErr_SetString(PyExc_RuntimeError, "can't get module state");
    return NULL;
  }

  PyTypeObject *type = state->PyAVSegyReaderSliceType;
  PyAVSegyReaderSlice *obj = (PyAVSegyReaderSlice *)type->tp_alloc(type, 0);
  if (!obj) {
    return NULL;
  }
  Py_INCREF(reader);
  obj->reader = reader;
  obj->kind = kind;
  obj->hdr_names = hdr_names;

  return (PyObject *)obj;
}

static void PyAVSegyReaderIterator_dealloc(PyAVSegyReaderSlice *self) {
  PyTypeObject *tp = Py_TYPE(self);
  Py_XDECREF(self->reader);
  Py_XDECREF(self->hdr_names);
  tp->tp_free((PyObject *)self);
  Py_DECREF(tp);
}

static PyObject *PyAVSegyReaderIterator_iter(PyObject *self) {
  Py_INCREF(self);
  return self;
}

static PyObject *PyAVSegyReaderIterator_iternext(PyAVSegyReaderIterator *self) {
  AVSegyReader *sgy = self->reader->sgy;
  if (av_segy_reader_end_of_data(sgy))
    return NULL;

  PyObject *result;
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  PyObject *hdr_names = self->hdr_names;
  Py_ssize_t stop = self->group_size;
  switch (self->kind) {
  case AV_SEGY_READ_SAMPLES:
    if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver)
      result = read_fixed_samp(sgy, 0, stop);
    else
      result = read_var_samp(sgy, 0, stop);
    break;
  case AV_SEGY_READ_HEADERS:
    if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver)
      result = read_fixed_hdrs(sgy, hdr_names, 0, stop);
    else
      result = read_var_hdrs(sgy, hdr_names, 0, stop);
    break;
  case AV_SEGY_READ_ALL:
    if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver)
      result = read_fixed_both(sgy, hdr_names, 0, stop);
    else
      result = read_var_both(sgy, hdr_names, 0, stop);
    break;
  default:
    PyErr_SetString(PyExc_RuntimeError, "Internal error unknown reading type");
    return NULL;
  }

  return result;
}

static PyType_Slot PyAVSegyReaderIterator_slots[] = {
    {Py_tp_dealloc, PyAVSegyReaderIterator_dealloc},
    {Py_tp_iter, PyAVSegyReaderIterator_iter},
    {Py_tp_iternext, PyAVSegyReaderIterator_iternext},
    {0, NULL},
};

PyType_Spec PyAVSegyReaderIterator_Spec = {
    .name = "avsegy.ReaderIterator",
    .basicsize = sizeof(PyAVSegyReaderIterator),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = PyAVSegyReaderIterator_slots,
};

static PyObject *make_PyAVSegyReaderIterator(PyAVSegyReader *reader,
                                             AVSegyReaderReadKind kind,
                                             PyObject *hdr_names,
                                             Py_ssize_t group_size) {
  if (!group_size) {
    PyErr_SetString(PyExc_ValueError, "Group size can't be zero");
    return NULL;
  }

  PyObject *module = PyType_GetModule(Py_TYPE(reader));
  avsegy_state *state = avsegy_get_state(module);
  if (!state || !state->PyAVSegyReaderIteratorType) {
    PyErr_SetString(PyExc_RuntimeError, "can't get module state");
    return NULL;
  }

  PyTypeObject *type = state->PyAVSegyReaderIteratorType;
  PyAVSegyReaderIterator *obj =
      (PyAVSegyReaderIterator *)type->tp_alloc(type, 0);
  if (!obj) {
    return NULL;
  }
  av_segy_reader_rewind(reader->sgy);
  Py_INCREF(reader);
  obj->reader = reader;
  obj->kind = kind;
  obj->hdr_names = hdr_names;
  obj->group_size = group_size;

  return (PyObject *)obj;
}

static void PyAVSegyReader_dealloc(PyAVSegyReader *self) {
  av_segy_reader_delete(&self->sgy);
  PyTypeObject *type = Py_TYPE(self);
  type->tp_free((PyObject *)self);
  Py_DECREF(type);
}

static PyObject *PyAVSegyReader_new(PyTypeObject *type, PyObject *args,
                                    PyObject *kwds) {
  (void)args;
  (void)kwds;
  PyAVSegyReader *self = (PyAVSegyReader *)type->tp_alloc(type, 0);
  if (!self)
    return NULL;
  self->sgy = av_segy_reader_new();
  if (!self->sgy) {
    PyErr_NoMemory();
    return NULL;
  }
  return (PyObject *)self;
}

static int PyAVSegyReader_init(PyAVSegyReader *self, PyObject *args,
                               PyObject *kwds) {
  char const *file_name = NULL;
  PyObject *bin_hdr_obj = NULL;

  static char *kwlist[] = {"file_name", "bin_hdr", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O", kwlist, &file_name,
                                   &bin_hdr_obj)) {
    return -1;
  }

  AVSegyErr const *err = av_segy_reader_get_error(self->sgy);
  if (bin_hdr_obj) {
    PyObject *module = PyType_GetModule(Py_TYPE(self));
    avsegy_state *state = avsegy_get_state(module);
    if (!PyObject_TypeCheck(bin_hdr_obj, state->PyAVSegyBinaryHeaderType)) {
      PyErr_SetString(PyExc_TypeError, "Wrong type for binary header");
      return -1;
    }
    AVSegyBinHdr bh = ((PyAVSegyBinaryHeader *)bin_hdr_obj)->bh;
    av_segy_reader_open_override(self->sgy, file_name, &bh);
  } else {
    av_segy_reader_open(self->sgy, file_name);
  }
  py_av_segy_check_error(err);
  return 0;
}
static PyObject *PyAVSegyReader_text_header(PyAVSegyReader *self,
                                            PyObject *Py_UNUSED(ignored)) {
  char *buf = malloc(AV_SEGY_TEXT_HEADER_SIZE + 1);
  if (!buf) {
    PyErr_NoMemory();
    return NULL;
  }
  av_segy_reader_read_text_header(self->sgy, buf);
  buf[AV_SEGY_TEXT_HEADER_SIZE] = '\0';
  PyObject *py_str = PyUnicode_FromString(buf);
  free(buf);
  return py_str;
}

static PyObject *PyAVSegyReader_close(PyAVSegyReader *self,
                                      PyObject *Py_UNUSED(ignored)) {
  av_segy_reader_delete(&self->sgy);
  Py_RETURN_NONE;
}

static PyObject *PyAVSegyReader_enter(PyObject *self,
                                      PyObject *Py_UNUSED(ignored)) {
  Py_INCREF(self);
  return self;
}

static PyObject *PyAVSegyReader_exit(PyAVSegyReader *self, PyObject *args) {
  (void)args;
  av_segy_reader_delete(&self->sgy);
  Py_RETURN_FALSE;
}

static PyObject *PyAVSegyReader_binary_header(PyAVSegyReader *self,
                                              PyObject *Py_UNUSED(ignored)) {
  PyObject *module;
  avsegy_state *state;

  module = PyType_GetModule(Py_TYPE(self));

  if (!module)
    return NULL;

  state = avsegy_get_state(module);

  Py_DECREF(module);
  PyAVSegyBinaryHeader *bh_obj =
      (PyAVSegyBinaryHeader *)state->PyAVSegyBinaryHeaderType->tp_alloc(
          state->PyAVSegyBinaryHeaderType, 0);
  if (!bh_obj)
    return NULL;
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(self->sgy);
  bh_obj->bh = *bh;
  return (PyObject *)bh_obj;
}

static PyObject *PyAVSegyReader_read_samples(PyAVSegyReader *self,
                                             PyObject *args) {
  (void)args;
  return make_PyAVSegyReaderSlice(self, AV_SEGY_READ_SAMPLES, NULL);
}

static PyObject *prepare_hdr_names(PyAVSegyReader *pysgy, PyObject *arg) {
  PyObject *result;
  if (PyArray_CheckExact(arg)) {
    int typenum = PyArray_TYPE((PyArrayObject *)arg);
    if (typenum == NPY_INT64) {
      Py_INCREF(arg);
      result = arg;
    } else {
      PyErr_SetString(PyExc_TypeError, "Array type shoud be INT64");
      return NULL;
    }
  } else if (!PyList_Check(arg)) {
    PyErr_SetString(PyExc_TypeError, "List expected");
    return NULL;
  } else {
    Py_ssize_t list_size = PyList_GET_SIZE(arg);
    npy_intp dims[1] = {(npy_intp)list_size};
    PyArrayObject *arr = (PyArrayObject *)PyArray_SimpleNew(1, dims, NPY_INT64);
    if (!arr)
      return NULL;

    npy_int64 *data = (npy_int64 *)PyArray_DATA(arr);

    for (Py_ssize_t i = 0; i < list_size; ++i) {
      PyObject *item = PyList_GET_ITEM(arg, i);
      if (!PyLong_Check(item)) {
        Py_DECREF(arr);
        PyErr_Format(PyExc_TypeError, "Element with index:%zd is not int", i);
        return NULL;
      }

      int64_t value;
      if (PyLong_AsInt64(item, &value) == -1 && PyErr_Occurred()) {
        Py_DECREF(arr);
        return NULL;
      }

      data[i] = (npy_int64)value;
    }
    result = (PyObject *)arr;
  }
  AVSegyBinHdr const *bin_hdr = av_segy_reader_get_binary_header(pysgy->sgy);
  uint64_t max_offset =
      (bin_hdr->max_num_add_tr_headers + 1) * AV_SEGY_TRACE_HEADER_SIZE;
  uint64_t const *trc_hdr_offset =
      av_segy_reader_get_trc_hdr_map_offset(pysgy->sgy);
  int64_t const *trc_hdr_format =
      av_segy_reader_get_trc_hdr_map_format(pysgy->sgy);
  size_t trc_hdr_map_size = av_segy_reader_get_trc_hdr_map_size(pysgy->sgy);
  npy_int64 *hdr_names = PyArray_DATA((PyArrayObject *)result);
  Py_ssize_t hdr_names_size = PyArray_SIZE((PyArrayObject *)result);
  for (Py_ssize_t i = 0; i < hdr_names_size; ++i) {
    int64_t hn = hdr_names[i];
    if (hn < 0 || (size_t)hn >= trc_hdr_map_size) {
      PyErr_Format(PyExc_ValueError,
                   "Header name: %lld is out of range of trace header map", hn);
      return NULL;
    }
    int64_t offset = trc_hdr_offset[hn];
    int64_t format = trc_hdr_format[hn];
    int val_size;
    switch (format) {
    case AV_SEGY_I8:
    case AV_SEGY_U8:
      val_size = 1;
      break;
    case AV_SEGY_I16:
    case AV_SEGY_U16:
      val_size = 2;
      break;
    case AV_SEGY_I32:
    case AV_SEGY_U32:
    case AV_SEGY_F32:
      val_size = 4;
      break;
    case AV_SEGY_I64:
    case AV_SEGY_U64:
    case AV_SEGY_F64:
      val_size = 8;
      break;
    default:
      PyErr_Format(PyExc_ValueError,
                   "Unknown trace header format: %lld of header name: %lld",
                   format, hn);
      return NULL;
    }
    uint64_t end_of_value = offset + val_size;
    if (end_of_value > max_offset) {
      PyErr_Format(PyExc_ValueError,
                   "Trace header value reading (%lld) will exceed size of "
                   "header (%llu)",
                   hn, max_offset);
      return NULL;
    }
  }

  return result;
}

static PyObject *PyAVSegyReader_remap_trace_headers(PyAVSegyReader *self,
                                                    PyObject *remap_list) {
  AVSegyReader *sgy = self->sgy;
  AVSegyErr const *err = av_segy_reader_get_error(sgy);
  AVSegyBinHdr const *bin_hdr = av_segy_reader_get_binary_header(sgy);

  if (!PyList_Check(remap_list)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a list of tuples");
    return NULL;
  }

  Py_ssize_t remap_size = PyList_Size(remap_list);
  npy_intp dims[1];
  dims[0] = remap_size;
  PyObject *array = PyArray_SimpleNew(1, dims, NPY_INT64);
  if (!array)
    return NULL;
  uint64_t *offset_arr = malloc(sizeof(uint64_t) * remap_size);
  int64_t *format_arr = malloc(sizeof(int64_t) * remap_size);
  if (!offset_arr || !format_arr) {
    PyErr_NoMemory();
    goto error;
  }

  for (Py_ssize_t i = 0; i < remap_size; i++) {
    PyObject *item = PyList_GetItem(remap_list, i);
    if (!item)
      goto error;

    if (!PyTuple_Check(item) || PyTuple_Size(item) != 2) {
      PyErr_SetString(PyExc_TypeError,
                      "Each element must be a tuple of length 2");
      goto error;
    }

    PyObject *fmt_obj = PyTuple_GetItem(item, 0);
    PyObject *offset_obj = PyTuple_GetItem(item, 1);
    if (!fmt_obj || !offset_obj)
      goto error;

    if (!PyLong_Check(fmt_obj) || !PyLong_Check(offset_obj)) {
      PyErr_SetString(PyExc_TypeError, "Tuple elements must be integers");
      goto error;
    }

    int64_t fmt;
    uint64_t offset;
    if (!PyLong_AsInt64(fmt_obj, &fmt) || !PyLong_AsUInt64(offset_obj, &offset))
      goto error;

    if (fmt < 0 || fmt > AV_SEGY_F64) {
      PyErr_Format(PyExc_ValueError, "Format value %lld is out of range", fmt);
      goto error;
    }

    uint64_t max_offset =
        (bin_hdr->max_num_add_tr_headers + 1) * AV_SEGY_TRACE_HEADER_SIZE;
    int val_size;
    switch (fmt) {
    case AV_SEGY_I8:
    case AV_SEGY_U8:
      val_size = 1;
      break;
    case AV_SEGY_I16:
    case AV_SEGY_U16:
      val_size = 2;
      break;
    case AV_SEGY_I32:
    case AV_SEGY_U32:
      val_size = 4;
      break;
    case AV_SEGY_I64:
    case AV_SEGY_U64:
      val_size = 8;
      break;
    default:
      PyErr_SetString(PyExc_ValueError, "Wrong header format value");
      goto error;
    }

    if (offset + val_size > max_offset) {
      PyErr_Format(PyExc_ValueError,
                   "Offset %lld is bigger than trace header length", offset);
      goto error;
    }

    offset_arr[i] = offset;
    format_arr[i] = fmt;
  }

  int64_t *hdr_names = PyArray_DATA((PyArrayObject *)array);
  if (av_segy_reader_trc_hdr_remap(sgy, offset_arr, format_arr, hdr_names,
                                   remap_size)) {
    py_av_segy_check_error(err);
    goto error;
  }

  free(offset_arr);
  free(format_arr);

  return array;
error:
  Py_DECREF(array);
  if (offset_arr)
    free(offset_arr);
  if (format_arr)
    free(format_arr);
  return NULL;
}

static PyObject *PyAVSegyReader_read_headers(PyAVSegyReader *self,
                                             PyObject *arg) {
  return make_PyAVSegyReaderSlice(self, AV_SEGY_READ_HEADERS,
                                  prepare_hdr_names(self, arg));
}

static PyObject *PyAVSegyReader_read_all(PyAVSegyReader *self, PyObject *arg) {
  return make_PyAVSegyReaderSlice(self, AV_SEGY_READ_ALL,
                                  prepare_hdr_names(self, arg));
}

static PyObject *PyAVSegyReader_read_samples_by(PyAVSegyReader *self,
                                                PyObject *args,
                                                PyObject *kwargs) {
  Py_ssize_t group_size = 0;

  static char *kwlist[] = {"group_size", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "n", kwlist, &group_size))
    return NULL;
  return make_PyAVSegyReaderIterator(self, AV_SEGY_READ_SAMPLES, NULL,
                                     group_size);
}

static PyObject *PyAVSegyReader_read_headers_by(PyAVSegyReader *self,
                                                PyObject *args,
                                                PyObject *kwargs) {
  Py_ssize_t group_size = 0;
  PyObject *hdr_names = NULL;

  static char *kwlist[] = {"group_size", "hdr_names", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "nO", kwlist, &group_size,
                                   &hdr_names))
    return NULL;
  return make_PyAVSegyReaderIterator(self, AV_SEGY_READ_HEADERS,
                                     prepare_hdr_names(self, hdr_names),
                                     group_size);
}

static PyObject *
PyAVSegyReader_read_headers_and_samples_by(PyAVSegyReader *self, PyObject *args,
                                           PyObject *kwargs) {
  Py_ssize_t group_size = 0;
  PyObject *hdr_names = NULL;

  static char *kwlist[] = {"group_size", "hdr_names", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "nO", kwlist, &group_size,
                                   &hdr_names))
    return NULL;
  return make_PyAVSegyReaderIterator(
      self, AV_SEGY_READ_ALL, prepare_hdr_names(self, hdr_names), group_size);
}

static PyMethodDef PyAVSegyReader_methods[] = {
    {"text_header", (PyCFunction)PyAVSegyReader_text_header, METH_NOARGS,
     "Returns text header."},
    {"binary_header", (PyCFunction)PyAVSegyReader_binary_header, METH_NOARGS,
     "Returns binary header."},
    {"read_samples", (PyCFunction)PyAVSegyReader_read_samples, METH_NOARGS,
     "Reads trace samples."},
    {"read_headers", (PyCFunction)PyAVSegyReader_read_headers, METH_O,
     "Reads trace headers."},
    {"read_headers_and_samples", (PyCFunction)PyAVSegyReader_read_all, METH_O,
     "Reads trace headers and samples."},
    {"read_samples_by", _PyCFunction_CAST(PyAVSegyReader_read_samples_by),
     METH_VARARGS | METH_KEYWORDS, "Reads trace samples by group_size."},
    {"read_headers_by", _PyCFunction_CAST(PyAVSegyReader_read_headers_by),
     METH_VARARGS | METH_KEYWORDS, "Reads trace headers by group_size."},
    {"read_headers_and_samples",
     _PyCFunction_CAST(PyAVSegyReader_read_headers_and_samples_by),
     METH_VARARGS | METH_KEYWORDS,
     "Reads trace headers and samples by group_size."},
    {"remap_trace_headers", (PyCFunction)PyAVSegyReader_remap_trace_headers,
     METH_O, "Adds new trace headers mappings."},
    {"close", (PyCFunction)PyAVSegyReader_close, METH_NOARGS,
     "Close SEGY file."},
    {"__enter__", (PyCFunction)PyAVSegyReader_enter, METH_NOARGS, NULL},
    {"__exit__", (PyCFunction)PyAVSegyReader_exit, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}};

static PyType_Slot PyAVSegyReader_slots[] = {
    {Py_tp_new, PyAVSegyReader_new},
    {Py_tp_init, PyAVSegyReader_init},
    {Py_tp_dealloc, PyAVSegyReader_dealloc},
    {Py_tp_methods, PyAVSegyReader_methods},
    {0, NULL}};

PyType_Spec PyAVSegyReader_Spec = {
    .name = "avsegy.Reader",
    .basicsize = sizeof(PyAVSegyReader),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = PyAVSegyReader_slots,
};

static PyObject *make_numpy_array(int format_code, uint64_t trc_num,
                                  int32_t samp_num) {
  npy_intp dims[2];
  dims[0] = (npy_intp)trc_num;
  dims[1] = (npy_intp)samp_num;
  Py_ssize_t fmt;
  switch (format_code) {
  case 1:
  case 5:
    fmt = NPY_FLOAT32;
    break;
  case 2:
  case 7:
    fmt = NPY_INT32;
    break;
  case 3:
    fmt = NPY_INT16;
    break;
  case 6:
    fmt = NPY_FLOAT64;
    break;
  case 8:
    fmt = NPY_INT8;
    break;
  case 9:
    fmt = NPY_INT64;
    break;
  case 10:
  case 15:
    fmt = NPY_UINT32;
    break;
  case 11:
    fmt = NPY_UINT16;
    break;
  case 12:
    fmt = NPY_UINT64;
    break;
  case 16:
    fmt = NPY_UINT8;
    break;
  default:
    PyErr_SetString(PyExc_ValueError, "Wrong sample format code");
    return NULL;
  }
  PyObject *array = PyArray_SimpleNew(2, dims, fmt);
  if (!array) {
    PyErr_NoMemory();
    return NULL;
  }
  return array;
}

static PyObject *read_fixed_samp(AVSegyReader *sgy, Py_ssize_t start,
                                 Py_ssize_t stop) {
  AVSegyErr const *err = av_segy_reader_get_error(sgy);
  uint64_t trc_num_in_file = av_segy_reader_get_number_of_traces(sgy);
  if ((uint64_t)start >= trc_num_in_file) {
    PyErr_SetString(PyExc_ValueError,
                    "Slice start value is more than number of traces in file");
    return NULL;
  }
  Py_ssize_t trc_num_req = stop == -1 ? -1 : stop - start;
  uint64_t trc_num_to_read = trc_num_req;
  if (trc_num_req == -1 || (uint64_t)trc_num_req > trc_num_in_file - start) {
    trc_num_to_read = trc_num_in_file - start;
  }
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  int32_t max_samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  int bytes_per_samp = av_segy_reader_get_bytes_per_sample(sgy);
  PyObject *array =
      make_numpy_array(bh->format_code, trc_num_to_read, max_samp_num);
  if (!array)
    return NULL;
  /* Skip traces */
  for (Py_ssize_t i = 0; i < start; ++i) {
    if (av_segy_reader_skip_headers(sgy))
      goto error;
    if (av_segy_reader_skip_samples(sgy, max_samp_num))
      goto error;
  }
  void *samp = PyArray_DATA((PyArrayObject *)array);
  for (uint64_t i = 0; i < trc_num_to_read; ++i) {
    if (av_segy_reader_skip_headers(sgy))
      goto error;
    if (av_segy_reader_read_samples(sgy, samp, max_samp_num))
      goto error;
    samp += max_samp_num * bytes_per_samp;
  }
  samp = PyArray_DATA((PyArrayObject *)array);
  av_segy_reader_samples_to_native(sgy, samp, trc_num_to_read * max_samp_num);
  return array;
error:
  py_av_segy_check_error(err);
  Py_DECREF(array);
  return NULL;
}

static PyObject *read_var_samp(AVSegyReader *sgy, Py_ssize_t start,
                               Py_ssize_t stop) {
  void **hdrs = NULL;
  int64_t *hdr_names = NULL;
  int hdrs_num;
  AVSegyErr const *err = av_segy_reader_get_error(sgy);
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  int32_t max_samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  Py_ssize_t trc_num_to_read = stop == -1 ? 1024 : stop - start;
  PyObject *array =
      make_numpy_array(bh->format_code, trc_num_to_read, max_samp_num);
  if (!array)
    return NULL;
  void *samp = PyArray_DATA((PyArrayObject *)array);
  /* Prepare simple data structure to read samples number */
  if (bh->max_num_add_tr_headers)
    hdrs_num = 1;
  else
    hdrs_num = 2;
  hdrs = calloc(hdrs_num, sizeof(void *));
  if (!hdrs) {
    PyErr_NoMemory();
    goto error;
  }
  hdr_names = malloc(sizeof(int64_t) * hdrs_num);
  if (!hdr_names) {
    PyErr_NoMemory();
    goto error;
  }
  hdr_names[0] = AV_SEGY_SAMP_NUM;
  if (bh->max_num_add_tr_headers)
    hdr_names[1] = AV_SEGY_EXT_SAMP_NUM;
  for (int i = 0; i < hdrs_num; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    int val_size;
    switch (fmt) {
    case AV_SEGY_I8:
    case AV_SEGY_U8:
      val_size = 1;
      break;
    case AV_SEGY_I16:
    case AV_SEGY_U16:
      val_size = 2;
      break;
    case AV_SEGY_I32:
    case AV_SEGY_U32:
      val_size = 4;
      break;
    case AV_SEGY_I64:
    case AV_SEGY_U64:
      val_size = 8;
      break;
    default:
      PyErr_SetString(PyExc_ValueError, "Wrong header format value");
      goto error;
    }
    hdrs[i] = malloc(val_size);
    if (!hdrs[i]) {
      Py_DECREF(array);
      PyErr_NoMemory();
      goto error;
    }
  }
  for (Py_ssize_t i = 0; i < start; ++i) {
    if (av_segy_reader_read_headers_single(sgy, hdrs, hdr_names, hdrs_num, 0)) {
      py_av_segy_check_error(err);
      goto error;
    }
    int16_t *i16_arr = hdrs[0];
    uint32_t var_samp_num = i16_arr[0];
    if (bh->max_num_add_tr_headers) {
      uint32_t *u32_arr = hdrs[1];
      uint32_t ext_samp_num = u32_arr[0];
      var_samp_num = ext_samp_num ? ext_samp_num : var_samp_num;
    }
    if (av_segy_reader_skip_samples(sgy, var_samp_num)) {
      py_av_segy_check_error(err);
      goto error;
    }
    if (av_segy_reader_end_of_data(sgy)) {
      for (int i = 0; i < hdrs_num; ++i)
        free(hdrs[i]);
      free(hdrs);
      free(hdr_names);
      return array;
    }
  }
  uint64_t counter = 0;
  void *ptr;
  int bytes_per_samp = av_segy_reader_get_bytes_per_sample(sgy);
  while (!av_segy_reader_end_of_data(sgy)) {
    ptr = samp + counter * max_samp_num * bytes_per_samp;
    if (av_segy_reader_read_headers_single(sgy, hdrs, hdr_names, hdrs_num, 0)) {
      py_av_segy_check_error(err);
      goto error;
    }
    int16_t *i16_arr = hdrs[0];
    uint32_t var_samp_num = i16_arr[0];
    if (bh->max_num_add_tr_headers) {
      uint32_t *u32_arr = hdrs[1];
      uint32_t ext_samp_num = u32_arr[0];
      var_samp_num = ext_samp_num ? ext_samp_num : var_samp_num;
    }
    if (av_segy_reader_read_samples(sgy, ptr, var_samp_num)) {
      py_av_segy_check_error(err);
      goto error;
    }
    if (++counter == (uint64_t)trc_num_to_read) {
      if (stop != -1)
        break;
      trc_num_to_read *= 2;
      npy_intp new_dims[2] = {trc_num_to_read, max_samp_num};
      PyArray_Dims newshape = {new_dims, 2};
      PyObject *dummy =
          PyArray_Resize((PyArrayObject *)array, &newshape, 1, NPY_CORDER);
      if (!dummy) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
        goto error;
      }
      samp = PyArray_DATA((PyArrayObject *)array);
      Py_DECREF(dummy);
    }
  }
  if (counter != (uint64_t)trc_num_to_read) {
    npy_intp new_dims[2] = {counter, max_samp_num};
    PyArray_Dims newshape = {new_dims, 2};
    PyObject *dummy =
        PyArray_Resize((PyArrayObject *)array, &newshape, 1, NPY_CORDER);
    if (!dummy) {
      PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
      goto error;
    }
    Py_DECREF(dummy);
  }
  for (int i = 0; i < hdrs_num; ++i) {
    free(hdrs[i]);
  }
  free(hdrs);
  free(hdr_names);
  samp = PyArray_DATA((PyArrayObject *)array);
  av_segy_reader_samples_to_native(sgy, samp,
                                   PyArray_SIZE((PyArrayObject *)array));
  return array;
error:
  Py_DECREF(array);
  if (hdrs) {
    for (int i = 0; i < hdrs_num; ++i) {
      free(hdrs[i]);
    }
    free(hdrs);
  }
  free(hdr_names);
  return NULL;
}

static PyObject *read_fixed_hdrs(AVSegyReader *sgy, PyObject *hdr_names_obj,
                                 Py_ssize_t start, Py_ssize_t stop) {
  PyArrayObject *arr = (PyArrayObject *)hdr_names_obj;
  int64_t *hdr_names = PyArray_DATA(arr);
  size_t hdr_names_size = PyArray_SIZE(arr);
  void **hdrs = malloc(sizeof(void *) * hdr_names_size);
  if (!hdrs) {
    PyErr_NoMemory();
    return NULL;
  }
  PyObject *list = PyList_New(hdr_names_size);
  if (!list)
    goto error;
  uint64_t trc_num_in_file = av_segy_reader_get_number_of_traces(sgy);
  if ((uint64_t)start >= trc_num_in_file) {
    PyErr_SetString(PyExc_ValueError,
                    "Slice start value is more than number of traces in file");
    goto error;
  }
  Py_ssize_t trc_num_req = stop == -1 ? -1 : stop - start;
  uint64_t trc_num_to_read = trc_num_req;
  if (trc_num_req == -1 || (uint64_t)trc_num_req > trc_num_in_file - start) {
    trc_num_to_read = trc_num_in_file - start;
  }
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  for (size_t i = 0; i < hdr_names_size; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    npy_intp dims[1];
    dims[0] = trc_num_to_read;
    PyObject *array;
    int data_type;
    switch (fmt) {
    case AV_SEGY_I8:
      data_type = NPY_INT8;
      break;
    case AV_SEGY_U8:
      data_type = NPY_UINT8;
      break;
    case AV_SEGY_I16:
      data_type = NPY_INT16;
      break;
    case AV_SEGY_U16:
      data_type = NPY_UINT16;
      break;
    case AV_SEGY_I32:
      data_type = NPY_INT32;
      break;
    case AV_SEGY_U32:
      data_type = NPY_UINT32;
      break;
    case AV_SEGY_F32:
      data_type = NPY_FLOAT32;
      break;
    case AV_SEGY_I64:
      data_type = NPY_INT64;
      break;
    case AV_SEGY_U64:
      data_type = NPY_UINT64;
      break;
    case AV_SEGY_F64:
      data_type = NPY_FLOAT64;
      break;
    default:
      PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                   fmt);
      goto error;
    }
    array = PyArray_SimpleNew(1, dims, data_type);
    if (!array)
      goto error;
    PyList_SetItem(list, i, array);
    hdrs[i] = PyArray_DATA((PyArrayObject *)array);
  }
  /* Skip traces */
  int32_t max_samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  for (Py_ssize_t i = 0; i < start; ++i) {
    if (av_segy_reader_skip_headers(sgy))
      goto error;
    if (av_segy_reader_skip_samples(sgy, max_samp_num))
      goto error;
  }
  AVSegyErrCode (*read_func)(AVSegyReader *restrict, void **restrict,
                             int64_t *restrict, size_t, size_t);
  /* if requested headers less than 10%, use single reading */
  if ((hdr_names_size * 100) / AV_SEGY_STD_TRC_HDR_MAP_SIZE < 10)
    read_func = av_segy_reader_read_headers_single;
  else
    read_func = av_segy_reader_read_headers_group;
  /* Read headers */
  for (uint64_t j = 0; j < trc_num_to_read; ++j) {
    if (read_func(sgy, hdrs, hdr_names, hdr_names_size, j))
      goto error;
    if (av_segy_reader_skip_samples(sgy, max_samp_num))
      goto error;
  }
  free(hdrs);
  return list;
error:
  if (list)
    Py_DECREF(list);
  if (hdrs)
    free(hdrs);
  return NULL;
}

static PyObject *read_var_hdrs(AVSegyReader *sgy, PyObject *hdr_names_obj,
                               Py_ssize_t start, Py_ssize_t stop) {
  PyArrayObject *arr = (PyArrayObject *)hdr_names_obj;
  int64_t *hdr_names = PyArray_DATA(arr);
  size_t hdr_names_size = PyArray_SIZE(arr);
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  size_t hdrs_num =
      bh->max_num_add_tr_headers ? hdr_names_size + 2 : hdr_names_size + 1;
  size_t hdrs_skip_size = hdrs_num - hdr_names_size;
  void **hdrs_skip = NULL;
  int64_t *hdrs_skip_names = NULL;
  int64_t *tmp = malloc(sizeof(int64_t) * hdrs_num);
  memcpy(tmp, hdr_names, sizeof(int64_t) * hdr_names_size);
  hdr_names = tmp;
  hdr_names[hdr_names_size] = AV_SEGY_SAMP_NUM;
  if (bh->max_num_add_tr_headers)
    hdr_names[hdr_names_size + 1] = AV_SEGY_EXT_SAMP_NUM;
  void **hdrs = calloc(hdrs_num, sizeof(void *));
  if (!hdrs) {
    PyErr_NoMemory();
    return NULL;
  }
  PyObject *list = PyList_New(hdr_names_size);
  if (!list)
    goto error;
  uint64_t trc_num_to_read = stop == -1 ? 1024 : stop - start;
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  for (size_t i = 0; i < hdr_names_size; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    npy_intp dims[1];
    dims[0] = trc_num_to_read;
    PyObject *array;
    int data_type;
    switch (fmt) {
    case AV_SEGY_I8:
      data_type = NPY_INT8;
      break;
    case AV_SEGY_U8:
      data_type = NPY_UINT8;
      break;
    case AV_SEGY_I16:
      data_type = NPY_INT16;
      break;
    case AV_SEGY_U16:
      data_type = NPY_UINT16;
      break;
    case AV_SEGY_I32:
      data_type = NPY_INT32;
      break;
    case AV_SEGY_U32:
      data_type = NPY_UINT32;
      break;
    case AV_SEGY_F32:
      data_type = NPY_FLOAT32;
      break;
    case AV_SEGY_I64:
      data_type = NPY_INT64;
      break;
    case AV_SEGY_U64:
      data_type = NPY_UINT64;
      break;
    case AV_SEGY_F64:
      data_type = NPY_FLOAT64;
      break;
    default:
      PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                   fmt);
      goto error;
    }
    array = PyArray_SimpleNew(1, dims, data_type);
    if (!array)
      goto error;
    PyList_SetItem(list, i, array);
    hdrs[i] = PyArray_DATA((PyArrayObject *)array);
  }
  for (size_t i = hdr_names_size; i < hdrs_num; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    int val_size;
    switch (fmt) {
    case AV_SEGY_I8:
    case AV_SEGY_U8:
      val_size = sizeof(int8_t);
      break;
    case AV_SEGY_I16:
    case AV_SEGY_U16:
      val_size = sizeof(int16_t);
      break;
    case AV_SEGY_I32:
    case AV_SEGY_U32:
    case AV_SEGY_F32:
      val_size = sizeof(int32_t);
      break;
    case AV_SEGY_I64:
    case AV_SEGY_U64:
    case AV_SEGY_F64:
      val_size = sizeof(int64_t);
      break;
    default:
      PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                   fmt);
      goto error;
    }
    hdrs[i] = malloc(val_size * trc_num_to_read);
    if (!hdrs[i]) {
      PyErr_NoMemory();
      goto error;
    }
  }
  /* Skip traces */
  hdrs_skip_size = hdrs_num - hdr_names_size;
  hdrs_skip = calloc(hdrs_skip_size, sizeof(void *));
  if (!hdrs_skip)
    goto error;
  hdrs_skip_names = malloc(sizeof(int64_t) * hdrs_skip_size);
  if (!hdrs_skip_names)
    goto error;
  hdrs_skip_names[0] = AV_SEGY_SAMP_NUM;
  hdrs_skip[0] = malloc(sizeof(int16_t));
  if (!hdrs_skip[0])
    goto error;
  if (bh->max_num_add_tr_headers) {
    hdrs_skip_names[1] = AV_SEGY_EXT_SAMP_NUM;
    hdrs_skip[1] = malloc(sizeof(int32_t));
    if (!hdrs_skip[1])
      goto error;
  }
  int16_t *i16_arr;
  int32_t *i32_arr;
  for (Py_ssize_t i = 0; i < start; ++i) {
    if (av_segy_reader_read_headers_single(sgy, hdrs_skip, hdrs_skip_names,
                                           hdrs_skip_size, 0))
      goto error;
    i16_arr = hdrs_skip[0];
    int32_t samp_num = i16_arr[0];
    if (bh->max_num_add_tr_headers) {
      i32_arr = hdrs_skip[1];
      int32_t ext_samp_num = i32_arr[0];
      samp_num = ext_samp_num ? ext_samp_num : samp_num;
    }
    if (av_segy_reader_skip_samples(sgy, samp_num))
      goto error;
  }
  /* Read headers */
  size_t counter = 0;
  AVSegyErrCode (*read_func)(AVSegyReader *restrict, void **restrict,
                             int64_t *restrict, size_t, size_t);
  /* if requested headers less than 10%, use single reading */
  if ((hdr_names_size * 100) / AV_SEGY_STD_TRC_HDR_MAP_SIZE < 10)
    read_func = av_segy_reader_read_headers_single;
  else
    read_func = av_segy_reader_read_headers_group;
  while (!av_segy_reader_end_of_data(sgy)) {
    if (read_func(sgy, hdrs, hdr_names, hdr_names_size, counter))
      goto error;
    i16_arr = hdrs[hdr_names_size];
    int32_t samp_num = i16_arr[counter];
    if (bh->max_num_add_tr_headers) {
      i32_arr = hdrs_skip[hdr_names_size + 1];
      int32_t ext_samp_num = i32_arr[counter];
      samp_num = ext_samp_num ? ext_samp_num : samp_num;
    }
    if (av_segy_reader_skip_samples(sgy, samp_num))
      goto error;
    if (++counter == (uint64_t)trc_num_to_read) {
      if (stop != -1)
        break;
      trc_num_to_read *= 2;
      for (size_t i = 0; i < hdr_names_size; ++i) {
        PyObject *arr = PyList_GetItem(list, i);
        if (!arr)
          goto error;
        npy_intp new_dims[1] = {trc_num_to_read};
        PyArray_Dims newshape = {new_dims, 1};
        PyObject *dummy =
            PyArray_Resize((PyArrayObject *)arr, &newshape, 1, NPY_CORDER);
        if (!dummy) {
          PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
          goto error;
        }
        Py_DECREF(dummy);
        hdrs[i] = PyArray_DATA((PyArrayObject *)arr);
      }
      for (size_t i = hdr_names_size; i < hdrs_num; ++i) {
        int64_t hn = hdr_names[i];
        int fmt = trc_hdr_format[hn];
        int val_size;
        switch (fmt) {
        case AV_SEGY_I8:
        case AV_SEGY_U8:
          val_size = sizeof(int8_t);
          break;
        case AV_SEGY_I16:
        case AV_SEGY_U16:
          val_size = sizeof(int16_t);
          break;
        case AV_SEGY_I32:
        case AV_SEGY_U32:
        case AV_SEGY_F32:
          val_size = sizeof(int32_t);
          break;
        case AV_SEGY_I64:
        case AV_SEGY_U64:
        case AV_SEGY_F64:
          val_size = sizeof(int64_t);
          break;
        default:
          PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                       fmt);
          goto error;
        }
        if (!realloc(hdrs[i], val_size * trc_num_to_read)) {
          PyErr_NoMemory();
          goto error;
        }
      }
    }
  }
  if (counter != (uint64_t)trc_num_to_read) {
    for (size_t i = 0; i < hdr_names_size; ++i) {
      PyObject *arr = PyList_GetItem(list, i);
      if (!arr)
        goto error;
      npy_intp new_dims[1] = {counter};
      PyArray_Dims newshape = {new_dims, 1};
      PyObject *dummy =
          PyArray_Resize((PyArrayObject *)arr, &newshape, 1, NPY_CORDER);
      if (!dummy) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
        goto error;
      }
      Py_DECREF(dummy);
    }
  }
  for (size_t i = 0; i < hdrs_skip_size; ++i)
    free(hdrs[i]);
  free(hdrs_skip);
  free(hdrs_skip_names);
  for (size_t i = hdr_names_size; i < hdrs_num; ++i)
    free(hdrs[i]);
  free(hdrs);
  return list;
error:
  if (list)
    Py_DECREF(list);
  if (hdrs_skip) {
    for (size_t i = 0; i < hdrs_skip_size; ++i)
      free(hdrs[i]);
    free(hdrs_skip);
  }
  if (hdrs_skip_names)
    free(hdrs_skip_names);
  if (hdrs) {
    for (size_t i = hdr_names_size; i < hdrs_num; ++i)
      free(hdrs[i]);
    free(hdrs);
  }
  return NULL;
}

static PyObject *read_fixed_both(AVSegyReader *sgy, PyObject *hdr_names_obj,
                                 Py_ssize_t start, Py_ssize_t stop) {
  PyArrayObject *arr = (PyArrayObject *)hdr_names_obj;
  int64_t *hdr_names = PyArray_DATA(arr);
  size_t hdr_names_size = PyArray_SIZE(arr);
  void **hdrs = malloc(sizeof(void *) * hdr_names_size);
  if (!hdrs) {
    PyErr_NoMemory();
    return NULL;
  }
  PyObject *list = NULL;
  PyObject *tuple = PyTuple_New(2);
  if (!tuple)
    goto error;
  list = PyList_New(hdr_names_size);
  if (!list)
    goto error;
  uint64_t trc_num_in_file = av_segy_reader_get_number_of_traces(sgy);
  if ((uint64_t)start >= trc_num_in_file) {
    PyErr_SetString(PyExc_ValueError,
                    "Slice start value is more than number of traces in file");
    goto error;
  }
  Py_ssize_t trc_num_req = stop == -1 ? -1 : stop - start;
  uint64_t trc_num_to_read = trc_num_req;
  if (trc_num_req == -1 || (uint64_t)trc_num_req > trc_num_in_file - start) {
    trc_num_to_read = trc_num_in_file - start;
  }
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  for (size_t i = 0; i < hdr_names_size; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    npy_intp dims[1];
    dims[0] = trc_num_to_read;
    PyObject *array;
    int data_type;
    switch (fmt) {
    case AV_SEGY_I8:
      data_type = NPY_INT8;
      break;
    case AV_SEGY_U8:
      data_type = NPY_UINT8;
      break;
    case AV_SEGY_I16:
      data_type = NPY_INT16;
      break;
    case AV_SEGY_U16:
      data_type = NPY_UINT16;
      break;
    case AV_SEGY_I32:
      data_type = NPY_INT32;
      break;
    case AV_SEGY_U32:
      data_type = NPY_UINT32;
      break;
    case AV_SEGY_F32:
      data_type = NPY_FLOAT32;
      break;
    case AV_SEGY_I64:
      data_type = NPY_INT64;
      break;
    case AV_SEGY_U64:
      data_type = NPY_UINT64;
      break;
    case AV_SEGY_F64:
      data_type = NPY_FLOAT64;
      break;
    default:
      PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                   fmt);
      goto error;
    }
    array = PyArray_SimpleNew(1, dims, data_type);
    if (!array)
      goto error;
    PyList_SetItem(list, i, array);
    hdrs[i] = PyArray_DATA((PyArrayObject *)array);
  }
  PyTuple_SetItem(tuple, 0, list);
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  int32_t max_samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  int bytes_per_samp = av_segy_reader_get_bytes_per_sample(sgy);
  PyObject *array =
      make_numpy_array(bh->format_code, trc_num_to_read, max_samp_num);
  if (!array)
    return NULL;
  PyTuple_SetItem(tuple, 1, array);
  /* Skip traces */
  for (Py_ssize_t i = 0; i < start; ++i) {
    if (av_segy_reader_skip_headers(sgy))
      goto error;
    if (av_segy_reader_skip_samples(sgy, max_samp_num))
      goto error;
  }
  AVSegyErrCode (*read_func)(AVSegyReader *restrict, void **restrict,
                             int64_t *restrict, size_t, size_t);
  /* if requested headers less than 10%, use single reading */
  if ((hdr_names_size * 100) / AV_SEGY_STD_TRC_HDR_MAP_SIZE < 10)
    read_func = av_segy_reader_read_headers_single;
  else
    read_func = av_segy_reader_read_headers_group;
  /* Read traces */
  void *samp = PyArray_DATA((PyArrayObject *)array);
  for (uint64_t j = 0; j < trc_num_to_read; ++j) {
    if (read_func(sgy, hdrs, hdr_names, hdr_names_size, j))
      goto error;
    if (av_segy_reader_read_samples(sgy, samp, max_samp_num))
      goto error;
    samp += max_samp_num * bytes_per_samp;
  }
  samp = PyArray_DATA((PyArrayObject *)array);
  av_segy_reader_samples_to_native(sgy, samp, trc_num_to_read * max_samp_num);
  free(hdrs);
  return tuple;
error:
  if (tuple)
    Py_DECREF(tuple);
  if (list)
    Py_DECREF(list);
  free(hdrs);
  return NULL;
}

static PyObject *read_var_both(AVSegyReader *sgy, PyObject *hdr_names_obj,
                               Py_ssize_t start, Py_ssize_t stop) {
  PyArrayObject *arr = (PyArrayObject *)hdr_names_obj;
  int64_t *hdr_names = PyArray_DATA(arr);
  size_t hdr_names_size = PyArray_SIZE(arr);
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  size_t hdrs_num =
      bh->max_num_add_tr_headers ? hdr_names_size + 2 : hdr_names_size + 1;
  size_t hdrs_skip_size = hdrs_num - hdr_names_size;
  void **hdrs_skip = NULL;
  int64_t *hdrs_skip_names = NULL;
  int64_t *tmp = malloc(sizeof(int64_t) * hdrs_num);
  memcpy(tmp, hdr_names, sizeof(int64_t) * hdr_names_size);
  hdr_names = tmp;
  hdr_names[hdr_names_size] = AV_SEGY_SAMP_NUM;
  if (bh->max_num_add_tr_headers)
    hdr_names[hdr_names_size + 1] = AV_SEGY_EXT_SAMP_NUM;
  void **hdrs = calloc(hdrs_num, sizeof(void *));
  if (!hdrs) {
    PyErr_NoMemory();
    return NULL;
  }
  PyObject *list = NULL;
  PyObject *tuple = PyTuple_New(2);
  if (!tuple)
    goto error;
  list = PyList_New(hdr_names_size);
  if (!list)
    goto error;
  uint64_t trc_num_to_read = stop == -1 ? 1024 : stop - start;
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  for (size_t i = 0; i < hdr_names_size; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    npy_intp dims[1];
    dims[0] = trc_num_to_read;
    PyObject *array;
    int data_type;
    switch (fmt) {
    case AV_SEGY_I8:
      data_type = NPY_INT8;
      break;
    case AV_SEGY_U8:
      data_type = NPY_UINT8;
      break;
    case AV_SEGY_I16:
      data_type = NPY_INT16;
      break;
    case AV_SEGY_U16:
      data_type = NPY_UINT16;
      break;
    case AV_SEGY_I32:
      data_type = NPY_INT32;
      break;
    case AV_SEGY_U32:
      data_type = NPY_UINT32;
      break;
    case AV_SEGY_F32:
      data_type = NPY_FLOAT32;
      break;
    case AV_SEGY_I64:
      data_type = NPY_INT64;
      break;
    case AV_SEGY_U64:
      data_type = NPY_UINT64;
      break;
    case AV_SEGY_F64:
      data_type = NPY_FLOAT64;
      break;
    default:
      PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                   fmt);
      goto error;
    }
    array = PyArray_SimpleNew(1, dims, data_type);
    if (!array)
      goto error;
    PyList_SetItem(list, i, array);
    hdrs[i] = PyArray_DATA((PyArrayObject *)array);
  }
  for (size_t i = hdr_names_size; i < hdrs_num; ++i) {
    int64_t hn = hdr_names[i];
    int fmt = trc_hdr_format[hn];
    int val_size;
    switch (fmt) {
    case AV_SEGY_I8:
    case AV_SEGY_U8:
      val_size = sizeof(int8_t);
      break;
    case AV_SEGY_I16:
    case AV_SEGY_U16:
      val_size = sizeof(int16_t);
      break;
    case AV_SEGY_I32:
    case AV_SEGY_U32:
    case AV_SEGY_F32:
      val_size = sizeof(int32_t);
      break;
    case AV_SEGY_I64:
    case AV_SEGY_U64:
    case AV_SEGY_F64:
      val_size = sizeof(int64_t);
      break;
    default:
      PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                   fmt);
      goto error;
    }
    hdrs[i] = malloc(val_size * trc_num_to_read);
    if (!hdrs[i]) {
      PyErr_NoMemory();
      goto error;
    }
  }
  PyTuple_SetItem(tuple, 0, list);
  int32_t max_samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  PyObject *array =
      make_numpy_array(bh->format_code, trc_num_to_read, max_samp_num);
  if (!array)
    return NULL;
  PyTuple_SetItem(tuple, 1, array);
  /* Skip traces */
  hdrs_skip_size = hdrs_num - hdr_names_size;
  hdrs_skip = calloc(hdrs_skip_size, sizeof(void *));
  if (!hdrs_skip)
    goto error;
  hdrs_skip_names = malloc(sizeof(int64_t) * hdrs_skip_size);
  if (!hdrs_skip_names)
    goto error;
  hdrs_skip_names[0] = AV_SEGY_SAMP_NUM;
  hdrs_skip[0] = malloc(sizeof(int16_t));
  if (!hdrs_skip[0])
    goto error;
  if (bh->max_num_add_tr_headers) {
    hdrs_skip_names[1] = AV_SEGY_EXT_SAMP_NUM;
    hdrs_skip[1] = malloc(sizeof(int32_t));
    if (!hdrs_skip[1])
      goto error;
  }
  int16_t *i16_arr;
  int32_t *i32_arr;
  for (Py_ssize_t i = 0; i < start; ++i) {
    if (av_segy_reader_read_headers_single(sgy, hdrs_skip, hdrs_skip_names,
                                           hdrs_skip_size, 0))
      goto error;
    i16_arr = hdrs_skip[0];
    int32_t samp_num = i16_arr[0];
    if (bh->max_num_add_tr_headers) {
      i32_arr = hdrs_skip[1];
      int32_t ext_samp_num = i32_arr[0];
      samp_num = ext_samp_num ? ext_samp_num : samp_num;
    }
    if (av_segy_reader_skip_samples(sgy, samp_num))
      goto error;
  }
  /* Read traces */
  int bytes_per_samp = av_segy_reader_get_bytes_per_sample(sgy);
  void *samp = PyArray_DATA((PyArrayObject *)array);
  size_t counter = 0;
  AVSegyErrCode (*read_func)(AVSegyReader *restrict, void **restrict,
                             int64_t *restrict, size_t, size_t);
  /* if requested headers less than 10%, use single reading */
  if ((hdr_names_size * 100) / AV_SEGY_STD_TRC_HDR_MAP_SIZE < 10)
    read_func = av_segy_reader_read_headers_single;
  else
    read_func = av_segy_reader_read_headers_group;
  while (!av_segy_reader_end_of_data(sgy)) {
    if (read_func(sgy, hdrs, hdr_names, hdr_names_size, counter))
      goto error;
    i16_arr = hdrs[hdr_names_size];
    int32_t samp_num = i16_arr[counter];
    if (bh->max_num_add_tr_headers) {
      i32_arr = hdrs_skip[hdr_names_size + 1];
      int32_t ext_samp_num = i32_arr[counter];
      samp_num = ext_samp_num ? ext_samp_num : samp_num;
    }
    if (av_segy_reader_read_samples(sgy, samp, max_samp_num))
      goto error;
    samp += max_samp_num * bytes_per_samp;
    if (++counter == (uint64_t)trc_num_to_read) {
      if (stop != -1)
        break;
      trc_num_to_read *= 2;
      for (size_t i = 0; i < hdr_names_size; ++i) {
        PyObject *arr = PyList_GetItem(list, i);
        if (!arr)
          goto error;
        npy_intp new_dims[1] = {trc_num_to_read};
        PyArray_Dims newshape = {new_dims, 1};
        PyObject *dummy =
            PyArray_Resize((PyArrayObject *)arr, &newshape, 1, NPY_CORDER);
        if (!dummy) {
          PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
          goto error;
        }
        Py_DECREF(dummy);
        hdrs[i] = PyArray_DATA((PyArrayObject *)arr);
      }
      for (size_t i = hdr_names_size; i < hdrs_num; ++i) {
        int64_t hn = hdr_names[i];
        int fmt = trc_hdr_format[hn];
        int val_size;
        switch (fmt) {
        case AV_SEGY_I8:
        case AV_SEGY_U8:
          val_size = sizeof(int8_t);
          break;
        case AV_SEGY_I16:
        case AV_SEGY_U16:
          val_size = sizeof(int16_t);
          break;
        case AV_SEGY_I32:
        case AV_SEGY_U32:
        case AV_SEGY_F32:
          val_size = sizeof(int32_t);
          break;
        case AV_SEGY_I64:
        case AV_SEGY_U64:
        case AV_SEGY_F64:
          val_size = sizeof(int64_t);
          break;
        default:
          PyErr_Format(PyExc_ValueError, "Wrong header number %d format: %d", i,
                       fmt);
          goto error;
        }
        if (!realloc(hdrs[i], val_size * trc_num_to_read)) {
          PyErr_NoMemory();
          goto error;
        }
      }
      npy_intp new_dims[2] = {trc_num_to_read, max_samp_num};
      PyArray_Dims newshape = {new_dims, 2};
      PyObject *dummy =
          PyArray_Resize((PyArrayObject *)array, &newshape, 1, NPY_CORDER);
      if (!dummy) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
        goto error;
      }
      samp = PyArray_DATA((PyArrayObject *)array);
      Py_DECREF(dummy);
    }
  }
  if (counter != (uint64_t)trc_num_to_read) {
    for (size_t i = 0; i < hdr_names_size; ++i) {
      PyObject *arr = PyList_GetItem(list, i);
      if (!arr)
        goto error;
      npy_intp new_dims[1] = {counter};
      PyArray_Dims newshape = {new_dims, 1};
      PyObject *dummy =
          PyArray_Resize((PyArrayObject *)arr, &newshape, 1, NPY_CORDER);
      if (!dummy) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
        goto error;
      }
      Py_DECREF(dummy);
    }
    npy_intp new_dims[2] = {counter, max_samp_num};
    PyArray_Dims newshape = {new_dims, 2};
    PyObject *dummy =
        PyArray_Resize((PyArrayObject *)array, &newshape, 1, NPY_CORDER);
    if (!dummy) {
      PyErr_SetString(PyExc_RuntimeError, "Failed to resize array");
      goto error;
    }
    Py_DECREF(dummy);
  }
  samp = PyArray_DATA((PyArrayObject *)array);
  av_segy_reader_samples_to_native(sgy, samp,
                                   PyArray_SIZE((PyArrayObject *)array));
  for (size_t i = 0; i < hdrs_skip_size; ++i)
    free(hdrs[i]);
  free(hdrs_skip);
  free(hdrs_skip_names);
  for (size_t i = hdr_names_size; i < hdrs_num; ++i)
    free(hdrs[i]);
  free(hdrs);
  return tuple;
error:
  if (tuple)
    Py_DECREF(tuple);
  if (list)
    Py_DECREF(list);
  if (hdrs_skip) {
    for (size_t i = 0; i < hdrs_skip_size; ++i)
      free(hdrs[i]);
    free(hdrs_skip);
  }
  if (hdrs_skip_names)
    free(hdrs_skip_names);
  if (hdrs) {
    for (size_t i = hdr_names_size; i < hdrs_num; ++i)
      free(hdrs[i]);
    free(hdrs);
  }
  return NULL;
}
