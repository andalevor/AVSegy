/**
 * \file avsegy.h
 * \brief python module fgor SEGY i|o
 * \author andalevor
 * \date 2026\09\21
 */

#ifndef PY_BINARY_HEADER_H
#define PY_BINARY_HEADER_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

typedef struct {
  PyTypeObject *PyAVSegyBinaryHeaderType;
  PyTypeObject *PyAVSegyReaderType;
  PyTypeObject *PyAVSegyReaderSliceType;
  PyTypeObject *PyAVSegyReaderIteratorType;
} avsegy_state;

avsegy_state *avsegy_get_state(PyObject *module);

#endif /* AVSEGY_H */
