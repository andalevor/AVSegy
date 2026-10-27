/**
 * \file PyAVSegyBinaryHeader.h
 * \brief SEGY binary header python bindings
 * \author andalevor
 * \date 2026\06\29
 */

#ifndef PY_AVSEGY_BINARY_HEADER_H
#define PY_AVSEGY_BINARY_HEADER_H

#define PY_SSIZE_T_CLEAN
#include "AVSegyCommon.h"
#include <Python.h>

typedef struct {
  PyObject_HEAD;
  AVSegyBinHdr bh;
} PyAVSegyBinaryHeader;

extern PyType_Spec PyAVSegyBinaryHeader_Spec;

#endif /* PY_AVSEGY_BINARY_HEADER_H */
