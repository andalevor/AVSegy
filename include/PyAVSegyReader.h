/**
 * \file PyAVSegyReader.h
 * \brief SEGY reader python bindings
 * \author andalevor
 * \date 2026\06\29
 */

#ifndef PY_AVSEGY_READER_H
#define PY_AVSEGY_READER_H

#define PY_SSIZE_T_CLEAN
#include "AVSegyReader.h"
#include <Python.h>

typedef struct {
  PyObject_HEAD;
  AVSegyReader *sgy;
} PyAVSegyReader;

extern PyType_Spec PyAVSegyReader_Spec;

typedef enum {
  AV_SEGY_READ_SAMPLES,
  AV_SEGY_READ_HEADERS,
  AV_SEGY_READ_ALL
} AVSegyReaderReadKind;

typedef struct {
  PyObject_HEAD;
  PyAVSegyReader *reader;
  AVSegyReaderReadKind kind;
  PyObject *hdr_names;
} PyAVSegyReaderSlice;

extern PyType_Spec PyAVSegyReaderSlice_Spec;

typedef struct {
  PyObject_HEAD;
  PyAVSegyReader *reader;
  AVSegyReaderReadKind kind;
  PyObject *hdr_names;
  Py_ssize_t group_size;
} PyAVSegyReaderIterator;

extern PyType_Spec PyAVSegyReaderIterator_Spec;

#endif /* PY_AVSEGY_READER_H */
