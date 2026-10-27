#include "PyAVSegyBinaryHeader.h"
#include "AVSegyCommon.h"
#include "PyAVSegySizeMac.h"
#include <stddef.h>

static PyMemberDef PyAVSegyBinaryHeader_members[] = {
    {"job_id", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, job_id), 0,
     "Job ID"},
    {"line_num", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, line_num), 0,
     "Number of line"},
    {"reel_num", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, reel_num), 0,
     "Number of reel"},
    {"tr_per_ens", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, tr_per_ens), 0,
     "Namber of data traces per ensemble"},
    {"aux_per_ens", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, aux_per_ens),
     0, "Number of auxiliary traces per ensemble"},
    {"samp_int", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, samp_int), 0,
     "Sample interval"},
    {"samp_int_orig", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, samp_int_orig),
     0, "Sample interval of original field recording"},
    {"samp_per_tr", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, samp_per_tr),
     0, "Number of samples per data trace"},
    {"samp_per_tr_orig", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, samp_per_tr_orig),
     0, "Number of samples per data trace of original field recording"},
    {"format_code", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, format_code),
     0, "Data sample format code"},
    {"ens_fold", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, ens_fold), 0,
     "Ensemble fold"},
    {"sort_code", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, sort_code), 0,
     "Trace sorting code"},
    {"vert_sum_code", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, vert_sum_code),
     0, "Vertical sum code"},
    {"sw_freq_at_start", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, sw_freq_at_start),
     0, "Sweep frequency at start"},
    {"sw_freq_at_end", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, sw_freq_at_end),
     0, "Sweep frequency at end"},
    {"sw_length", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, sw_length), 0,
     "Sweep length"},
    {"sw_type_code", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, sw_type_code),
     0, "Sweep type code"},
    {"sw_ch_tr_num", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, sw_ch_tr_num),
     0, "Trace number of sweep channel"},
    {"taper_at_start", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, taper_at_start),
     0, "Sweep trace taper length in ms at start"},
    {"taper_at_end", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, taper_at_end),
     0, "Sweep trace taper length in ms at end"},
    {"taper_type", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, taper_type), 0,
     "Taper type"},
    {"corr_traces", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, corr_traces),
     0, "Correlated data traces"},
    {"bin_gain_recov", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, bin_gain_recov),
     0, "Binary gain recovered"},
    {"amp_recov_meth", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, amp_recov_meth),
     0, "Amplitude recovery method"},
    {"measure_system", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, measure_system),
     0, "Measurement system"},
    {"impulse_sig_pol", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, impulse_sig_pol),
     0, "Impulse signal polarity"},
    {"vib_pol_code", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, vib_pol_code),
     0, "Vibrator polarity code"},
    {"ext_tr_per_ens", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, ext_tr_per_ens),
     0, "Extended number of data traces per ensemble"},
    {"ext_aux_per_ens", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, ext_aux_per_ens),
     0, "Extended number of auxiliary traces per ensemble"},
    {"ext_samp_per_tr", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, ext_samp_per_tr),
     0, "Extended number of samples per data trace"},
    {"ext_samp_int", Py_T_DOUBLE,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, ext_samp_int),
     0, "Extended sample interval"},
    {"ext_samp_int_orig", Py_T_DOUBLE,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, ext_samp_int_orig),
     0, "Extended sample interval of original field recording"},
    {"ext_samp_per_tr_orig", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, ext_samp_per_tr_orig),
     0, "Extended number of samples per data trace in original recording"},
    {"ext_ens_fold", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, ext_ens_fold),
     0, "Extended ensemble fold"},
    {"endianness", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) + offsetof(AVSegyBinHdr, endianness), 0,
     "Endianness"},
    {"SEGY_rev_major_ver", Py_T_UINT8,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, SEGY_rev_major_ver),
     0, "Major SEG-Y Format Revision Number"},
    {"SEGY_rev_minor_ver", Py_T_UINT8,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, SEGY_rev_minor_ver),
     0, "Minor SEG-Y Format Revision Number"},
    {"fixed_tr_length", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, fixed_tr_length),
     0, "Fixed length trace flag"},
    {"ext_text_headers_num", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, ext_text_headers_num),
     0, "Number of Extended Textual File Header records"},
    {"max_num_add_tr_headers", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, max_num_add_tr_headers),
     0, "Maximum number of additional trace headers"},
    {"time_basis_code", Py_T_INT16,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, time_basis_code),
     0, "Time basis code"},
    {"num_of_tr_in_file", Py_T_UINT64,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, num_of_tr_in_file),
     0, "Number of traces in this file"},
    {"byte_off_of_first_tr", Py_T_UINT64,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, byte_off_of_first_tr),
     0, "Byte offset of first trace relative to start of file"},
    {"num_of_trailer_stanza", Py_T_INT32,
     offsetof(PyAVSegyBinaryHeader, bh) +
         offsetof(AVSegyBinHdr, num_of_trailer_stanza),
     0, "Number data trailer stanza records following the last trace"},
    {NULL, 0, 0, 0, NULL},
};

static PyObject *PyAVSegyBinaryHeader_new(PyTypeObject *type, PyObject *args,
                                          PyObject *kwds) {
  PyAVSegyBinaryHeader *self;
  (void)args;
  (void)kwds;
  self = (PyAVSegyBinaryHeader *)type->tp_alloc(type, 0);
  if (!self)
    return NULL;
  memset(&self->bh, 0, sizeof(AVSegyBinHdr));
  return (PyObject *)self;
}

static PyObject *PyAVSegyBinaryHeader_repr(PyAVSegyBinaryHeader *self) {
  AVSegyBinHdr const *bh = &self->bh;
  int len1 = snprintf(NULL, 0, "%.6f", bh->ext_samp_int);
  char *buf1 = malloc(len1 + 1);
  snprintf(buf1, len1, "%.6f", bh->ext_samp_int);
  int len2 = snprintf(NULL, 0, "%.6f", bh->ext_samp_int);
  char *buf2 = malloc(len2 + 1);
  snprintf(buf2, len2, "%.6f", bh->ext_samp_int);
  PyObject *s = PyUnicode_FromFormat(
      "Binary header:\nJob id: %d\nLine number: "
      "%d\nReel number: %d\nData traces per ensemble: "
      "%d\nAuxiliary traces per ensemble: %d\nSample interval: %d\nSample "
      "interval of original record: %d\nNumber of samples per trace: "
      "%d\nNumber of samples per original trace: %d\nFormat code: %d\nEnsemble "
      "fold: %d\nSort code: %d\nVertical sum code: %d\nSweep start frequency: "
      "%d\nSweep end frequency: %d\nSweep length: %d\nSweep type code: "
      "%d\nTrace number of sweep channel: %d\nTaper at start: %d\nTaper at "
      "end: %d\nTaper type: %d\nCorrelated traces: %d\nBinary gain recovery: "
      "%d\nAmplitude recovery method: %d\nMeasurement system: %d\nImpulse "
      "signal polarity: %d\nVibratory polarity code: %d\nTraces per ensemble "
      "(ext): %d\nAuxiliary traces per ensemble (ext): %d\nSamples per trace "
      "(ext): %d\nSamples interval (ext): %s\nSamples interval original record "
      "(ext): %s\nSamples per original trace (ext): %d\nEnsemble fold (ext): "
      "%d\nEndianness: %x\nSEGY revision major version: %d\nSEGY revision "
      "minor version: %d\nFixed trace length flag: %d\nExtended text headers "
      "number: %d\nMaximum number of additional trace headers: %d\nTime basis "
      "code: %d\nNumber of traces in file: %llu\nByte offset of first trace: "
      "%llu\nNumber of trailer stanza: %d",
      bh->job_id, bh->line_num, bh->reel_num, bh->tr_per_ens, bh->aux_per_ens,
      bh->samp_int, bh->samp_int_orig, bh->samp_per_tr, bh->samp_per_tr_orig,
      bh->format_code, bh->ens_fold, bh->sort_code, bh->vert_sum_code,
      bh->sw_freq_at_start, bh->sw_freq_at_end, bh->sw_length, bh->sw_type_code,
      bh->sw_ch_tr_num, bh->taper_at_start, bh->taper_at_end, bh->taper_type,
      bh->corr_traces, bh->bin_gain_recov, bh->amp_recov_meth,
      bh->measure_system, bh->impulse_sig_pol, bh->vib_pol_code,
      bh->ext_tr_per_ens, bh->ext_aux_per_ens, bh->ext_samp_per_tr, buf1, buf2,
      bh->ext_samp_per_tr_orig, bh->ext_ens_fold, bh->endianness,
      bh->SEGY_rev_major_ver, bh->SEGY_rev_minor_ver, bh->fixed_tr_length,
      bh->ext_text_headers_num, bh->max_num_add_tr_headers, bh->time_basis_code,
      bh->num_of_tr_in_file, bh->byte_off_of_first_tr,
      bh->num_of_trailer_stanza);
  free(buf1);
  free(buf2);
  return s;
}

static PyType_Slot PyAVSegyBinaryHeader_slots[] = {
    {Py_tp_new, PyAVSegyBinaryHeader_new},
    {Py_tp_repr, PyAVSegyBinaryHeader_repr},
    {Py_tp_members, PyAVSegyBinaryHeader_members},
    {0, NULL}};

PyType_Spec PyAVSegyBinaryHeader_Spec = {
    .name = "avsegy.BinaryHeader",
    .basicsize = sizeof(PyAVSegyBinaryHeader),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = PyAVSegyBinaryHeader_slots,
};
