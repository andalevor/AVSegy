#define PY_SSIZE_T_CLEAN
#include "PyAVSegyError.h"
#include "AVSegyCommon.h"
#include <Python.h>

void py_av_segy_check_error(AVSegyErr const *err) {
  switch (err->code) {
  case AV_SEGY_ERR_OK:
    break;
  case AV_SEGY_ERR_NO_MEM:
    PyErr_NoMemory();
    break;
  case AV_SEGY_ERR_FILE_OPEN:
  case AV_SEGY_ERR_FILE_SEEK:
  case AV_SEGY_ERR_FILE_READ:
  case AV_SEGY_ERR_FILE_TELL:
    PyErr_SetString(PyExc_OSError, err->message);
    break;
  case AV_SEGY_ERR_UNSUPPORTED_FORMAT:
  case AV_SEGY_ERR_UNKNOWN_ENDIANNESS:
  case AV_SEGY_ERR_BROKEN_FILE:
    PyErr_SetString(PyExc_RuntimeError, err->message);
    break;
  }
}
