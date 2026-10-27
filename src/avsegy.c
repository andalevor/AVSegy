#include "avsegy.h"
#include "PyAVSegyBinaryHeader.h"
#include "PyAVSegyReader.h"
#define PY_ARRAY_UNIQUE_SYMBOL AVSEGY_ARRAY_API
#include <numpy/arrayobject.h>

avsegy_state *avsegy_get_state(PyObject *module) {
  return (avsegy_state *)PyModule_GetState(module);
}

static int avsegy_traverse(PyObject *module, visitproc visit, void *arg) {
  avsegy_state *state = avsegy_get_state(module);

  Py_VISIT(state->PyAVSegyBinaryHeaderType);
  Py_VISIT(state->PyAVSegyReaderType);

  return 0;
}

static int avsegy_clear(PyObject *module) {
  avsegy_state *state = avsegy_get_state(module);

  Py_CLEAR(state->PyAVSegyBinaryHeaderType);
  Py_CLEAR(state->PyAVSegyReaderType);

  return 0;
}

static void avsegy_free(void *module) { avsegy_clear((PyObject *)module); }

static int avsegy_exec(PyObject *module) {
  avsegy_state *state = avsegy_get_state(module);

  if (_import_array() < 0) {
    PyErr_Print();
    PyErr_SetString(PyExc_ImportError,
                    "could not import numpy.core.multiarray");
    return -1;
  }

  PyObject *type =
      PyType_FromModuleAndSpec(module, &PyAVSegyBinaryHeader_Spec, NULL);
  if (!type)
    return -1;
  state->PyAVSegyBinaryHeaderType = (PyTypeObject *)type;
  if (PyModule_AddObjectRef(module, "BinaryHeader", type) < 0) {

    return -1;
  }

  type = PyType_FromModuleAndSpec(module, &PyAVSegyReader_Spec, NULL);
  if (!type)
    return -1;
  state->PyAVSegyReaderType = (PyTypeObject *)type;
  if (PyModule_AddObjectRef(module, "Reader", type) < 0) {

    return -1;
  }

  type = PyType_FromModuleAndSpec(module, &PyAVSegyReaderSlice_Spec, NULL);
  if (!type)
    return -1;
  state->PyAVSegyReaderSliceType = (PyTypeObject *)type;

  type = PyType_FromModuleAndSpec(module, &PyAVSegyReaderIterator_Spec, NULL);
  if (!type)
    return -1;
  state->PyAVSegyReaderIteratorType = (PyTypeObject *)type;

  PyObject *fmt_mod = NULL;
  PyObject *th_mod = PyModule_New("avsegy.trace_header_names");
  if (!th_mod)
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRC_SEQ_LINE", AV_SEGY_TRC_SEQ_LINE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRC_SEQ_SEGY", AV_SEGY_TRC_SEQ_SEGY))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "FFID", AV_SEGY_FFID))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "CHAN", AV_SEGY_CHAN))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "ESP", AV_SEGY_ESP))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "ENS_NO", AV_SEGY_ENS_NO))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SEQ_NO", AV_SEGY_SEQ_NO))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRACE_ID", AV_SEGY_TRACE_ID))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "VERT_SUM", AV_SEGY_VERT_SUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "HOR_SUM", AV_SEGY_HOR_SUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "DATA_USE", AV_SEGY_DATA_USE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "OFFSET", AV_SEGY_OFFSET))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "R_ELEV", AV_SEGY_R_ELEV))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "S_ELEV", AV_SEGY_S_ELEV))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "S_DEPTH", AV_SEGY_S_DEPTH))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "R_DATUM", AV_SEGY_R_DATUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "S_DATUM", AV_SEGY_S_DATUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "S_WATER", AV_SEGY_S_WATER))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "R_WATER", AV_SEGY_R_WATER))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "ELEV_SCALAR", AV_SEGY_ELEV_SCALAR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "COORD_SCALAR", AV_SEGY_COORD_SCALAR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_X", AV_SEGY_SOU_X))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_Y", AV_SEGY_SOU_Y))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "REC_X", AV_SEGY_REC_X))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "REC_Y", AV_SEGY_REC_Y))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "COORD_UNITS", AV_SEGY_COORD_UNITS))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "WEATH_VEL", AV_SEGY_WEATH_VEL))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SUBWEATH_VEL", AV_SEGY_SUBWEATH_VEL))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "S_UPHOLE", AV_SEGY_S_UPHOLE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "R_UPHOLE", AV_SEGY_R_UPHOLE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "S_STAT", AV_SEGY_S_STAT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "R_STAT", AV_SEGY_R_STAT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TOT_STAT", AV_SEGY_TOT_STAT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LAG_A", AV_SEGY_LAG_A))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LAG_B", AV_SEGY_LAG_B))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "DELAY_TIME", AV_SEGY_DELAY_TIME))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "MUTE_START", AV_SEGY_MUTE_START))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "MUTE_END", AV_SEGY_MUTE_END))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SAMP_NUM", AV_SEGY_SAMP_NUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SAMP_INT", AV_SEGY_SAMP_INT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "GAIN_TYPE", AV_SEGY_GAIN_TYPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "GAIN_CONST", AV_SEGY_GAIN_CONST))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "INIT_GAIN", AV_SEGY_INIT_GAIN))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "CORRELATED", AV_SEGY_CORRELATED))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SW_START", AV_SEGY_SW_START))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SW_END", AV_SEGY_SW_END))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SW_LENGTH", AV_SEGY_SW_LENGTH))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SW_TYPE", AV_SEGY_SW_TYPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SW_TAPER_START", AV_SEGY_SW_TAPER_START))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SW_TAPER_END", AV_SEGY_SW_TAPER_END))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TAPER_TYPE", AV_SEGY_TAPER_TYPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "ALIAS_FILT_FREQ",
                              AV_SEGY_ALIAS_FILT_FREQ))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "ALIAS_FILT_SLOPE",
                              AV_SEGY_ALIAS_FILT_SLOPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "NOTCH_FILT_FREQ",
                              AV_SEGY_NOTCH_FILT_FREQ))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "NOTCH_FILT_SLOPE",
                              AV_SEGY_NOTCH_FILT_SLOPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LOW_CUT_FREQ", AV_SEGY_LOW_CUT_FREQ))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "HIGH_CUT_FREQ", AV_SEGY_HIGH_CUT_FREQ))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LOW_CUT_SLOPE", AV_SEGY_LOW_CUT_SLOPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "HIGH_CUT_SLOPE", AV_SEGY_HIGH_CUT_SLOPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "YEAR", AV_SEGY_YEAR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "DAY", AV_SEGY_DAY))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "HOUR", AV_SEGY_HOUR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "MINUTE", AV_SEGY_MINUTE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SECOND", AV_SEGY_SECOND))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TIME_BASIS_CODE",
                              AV_SEGY_TIME_BASIS_CODE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRACE_WEIGHT", AV_SEGY_TRACE_WEIGHT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "GROUP_NUM_ROLL", AV_SEGY_GROUP_NUM_ROLL))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "GROUP_NUM_FIRST",
                              AV_SEGY_GROUP_NUM_FIRST))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "GROUP_NUM_LAST", AV_SEGY_GROUP_NUM_LAST))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "GAP_SIZE", AV_SEGY_GAP_SIZE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "OVER_TRAVEL", AV_SEGY_OVER_TRAVEL))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "CDP_X", AV_SEGY_CDP_X))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "CDP_Y", AV_SEGY_CDP_Y))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "INLINE", AV_SEGY_INLINE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "XLINE", AV_SEGY_XLINE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SP_NUM", AV_SEGY_SP_NUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SP_NUM_SCALAR", AV_SEGY_SP_NUM_SCALAR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TR_VAL_UNIT", AV_SEGY_TR_VAL_UNIT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRANS_CONST_MANT",
                              AV_SEGY_TRANS_CONST_MANT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRANS_CONST_EXP",
                              AV_SEGY_TRANS_CONST_EXP))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TRANS_UNITS", AV_SEGY_TRANS_UNITS))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "DEVICE_ID", AV_SEGY_DEVICE_ID))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "TIME_SCALAR", AV_SEGY_TIME_SCALAR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOURCE_TYPE", AV_SEGY_SOURCE_TYPE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_V_DIR", AV_SEGY_SOU_V_DIR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_X_DIR", AV_SEGY_SOU_X_DIR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_I_DIR", AV_SEGY_SOU_I_DIR))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_MEAS_MANT", AV_SEGY_SOU_MEAS_MANT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_MEAS_EXP", AV_SEGY_SOU_MEAS_EXP))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "SOU_MEAS_UNIT", AV_SEGY_SOU_MEAS_UNIT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_TRC_SEQ_LINE",
                              AV_SEGY_EXT_TRC_SEQ_LINE))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_TRC_SEQ_SEGY",
                              AV_SEGY_EXT_TRC_SEQ_SEGY))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_FFID", AV_SEGY_EXT_FFID))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_ENS_NO", AV_SEGY_EXT_ENS_NO))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_R_ELEV", AV_SEGY_EXT_R_ELEV))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "R_DEPTH", AV_SEGY_R_DEPTH))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_S_ELEV", AV_SEGY_EXT_S_ELEV))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_S_DEPTH", AV_SEGY_EXT_S_DEPTH))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_R_DATUM", AV_SEGY_EXT_R_DATUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_S_DATUM", AV_SEGY_EXT_S_DATUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_S_WATER", AV_SEGY_EXT_S_WATER))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_R_WATER", AV_SEGY_EXT_R_WATER))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_SOU_X", AV_SEGY_EXT_SOU_X))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_SOU_Y", AV_SEGY_EXT_SOU_Y))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_REC_X", AV_SEGY_EXT_REC_X))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_REC_Y", AV_SEGY_EXT_REC_Y))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_OFFSET", AV_SEGY_EXT_OFFSET))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_SAMP_NUM", AV_SEGY_EXT_SAMP_NUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "NANOSECOND", AV_SEGY_NANOSECOND))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_SAMP_INT", AV_SEGY_EXT_SAMP_INT))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "CABLE_NUM", AV_SEGY_CABLE_NUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "ADD_TRC_HDR_NUM",
                              AV_SEGY_ADD_TRC_HDR_NUM))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LAST_TRC_FLAG", AV_SEGY_LAST_TRC_FLAG))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_CDP_X", AV_SEGY_EXT_CDP_X))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "EXT_CDP_Y", AV_SEGY_EXT_CDP_Y))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "FIRST", 0))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LAST1", 90))
    goto error;
  if (PyModule_AddIntConstant(th_mod, "LAST2", AV_SEGY_STD_TRC_HDR_MAP_SIZE))
    goto error;
  if (PyModule_AddObjectRef(module, "trace_header_names", th_mod))
    goto error;

  fmt_mod = PyModule_New("avsegy.trace_header_formats");
  if (!fmt_mod)
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "i8", AV_SEGY_I8))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "i16", AV_SEGY_I16))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "i32", AV_SEGY_I32))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "i64", AV_SEGY_I64))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "u8", AV_SEGY_U8))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "u16", AV_SEGY_U16))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "u32", AV_SEGY_U32))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "u64", AV_SEGY_U64))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "f32", AV_SEGY_F32))
    goto error;
  if (PyModule_AddIntConstant(fmt_mod, "f64", AV_SEGY_F64))
    goto error;
  if (PyModule_AddObjectRef(module, "trace_header_formats", fmt_mod))
    goto error;

  Py_DECREF(th_mod);
  Py_DECREF(fmt_mod);
  return 0;
error:
  Py_DECREF(th_mod);
  if (fmt_mod)
    Py_DECREF(fmt_mod);
  return -1;
}

static PyModuleDef_Slot avsegy_slots[] = {{Py_mod_exec, (void *)avsegy_exec},
                                          {0, NULL}};

static PyModuleDef avsegy_module = {PyModuleDef_HEAD_INIT,
                                    .m_name = "avsegy",
                                    .m_doc =
                                        "C extension to read/write SEGY files",
                                    .m_size = sizeof(avsegy_state),
                                    .m_slots = avsegy_slots,
                                    .m_traverse = avsegy_traverse,
                                    .m_clear = avsegy_clear,
                                    .m_free = avsegy_free};
PyMODINIT_FUNC PyInit_avsegy(void) { return PyModuleDef_Init(&avsegy_module); }
