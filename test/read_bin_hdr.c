#include "AVSegyCommon.h"
#include "AVSegyReader.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s file_name.sgy\n", argv[0]);
    return 1;
  }
  AVSegyReader *sgy = av_segy_reader_new();
  if (!sgy) {
    printf("Can not allocate memory for SEGY reader struct\n");
    return 1;
  }
  AVSegyErr const *err = av_segy_reader_get_error(sgy);
  if (av_segy_reader_open(sgy, argv[1])) {
    printf("%s\n", err->message);
    return 1;
  }
  AVSegyBinHdr const *bin_hdr = av_segy_reader_get_binary_header(sgy);
  printf("Job identification number: %d\n", bin_hdr->job_id);
  printf("Line number: %d\n", bin_hdr->line_num);
  printf("Reel number: %d\n", bin_hdr->reel_num);
  printf("Number of data traces per ensemble: %d\n", bin_hdr->tr_per_ens);
  printf("Number of auxiliary traces per ensemble: %d\n", bin_hdr->aux_per_ens);
  printf("Sample interval: %d\n", bin_hdr->samp_int);
  printf("Sample interval of original field record: %d\n",
         bin_hdr->samp_int_orig);
  printf("Number of samples per data trace: %d\n", bin_hdr->samp_per_tr);
  printf("Number of samples per data trace of original field recording: %d\n",
         bin_hdr->samp_per_tr_orig);
  printf("Data sample format code: %d\n", bin_hdr->format_code);
  printf("Ensemble fold: %d\n", bin_hdr->ens_fold);
  printf("Trace sorting code: %d\n", bin_hdr->sort_code);
  printf("Vertical sum code: %d\n", bin_hdr->vert_sum_code);
  printf("Sweep frequency at start: %d\n", bin_hdr->sw_freq_at_start);
  printf("Sweep frequency at end: %d\n", bin_hdr->sw_freq_at_end);
  printf("Sweep length: %d\n", bin_hdr->sw_length);
  printf("Sweep type code: %d\n", bin_hdr->sw_type_code);
  printf("Trace number of sweep channel: %d\n", bin_hdr->sw_ch_tr_num);
  printf("Sweep trace taper length at start: %d\n", bin_hdr->taper_at_start);
  printf("Sweep trace taper length at end: %d\n", bin_hdr->taper_at_end);
  printf("Taper type: %d\n", bin_hdr->taper_type);
  printf("Correlated data traces: %d\n", bin_hdr->corr_traces);
  printf("Binary gain recovered: %d\n", bin_hdr->bin_gain_recov);
  printf("Amplitude recovery method: %d\n", bin_hdr->amp_recov_meth);
  printf("Measurement system: %d\n", bin_hdr->measure_system);
  printf("Impulse signal polarity: %d\n", bin_hdr->impulse_sig_pol);
  printf("Viratory polarity code: %d\n", bin_hdr->vib_pol_code);
  printf("Extended number of data traces per ensemble: %d\n",
         bin_hdr->ext_tr_per_ens);
  printf("Extended number of auxiliary traces per ensemble: %d\n",
         bin_hdr->ext_aux_per_ens);
  printf("Extended number of samples per data trace: %d\n",
         bin_hdr->ext_samp_per_tr);
  printf("Extended sample interval: %f\n", bin_hdr->ext_samp_int);
  printf("Extended sample interval of original field recording: %f\n",
         bin_hdr->ext_samp_int_orig);
  printf(
      "Extended number of samples per data trace in original recording: %d\n",
      bin_hdr->ext_samp_per_tr_orig);
  printf("Extended ensemble fold: %d\n", bin_hdr->ext_ens_fold);
  printf("Endianness constant: %x\n", bin_hdr->endianness);
  printf("SEG-Y major version: %d\n", bin_hdr->SEGY_rev_major_ver);
  printf("SEG-Y minor version: %d\n", bin_hdr->SEGY_rev_minor_ver);
  printf("Fixed lenght trace flag: %d\n", bin_hdr->fixed_tr_length);
  printf("Number of extended textual headers: %d\n",
         bin_hdr->ext_text_headers_num);
  printf("Maximum number of additional trace headers: %d\n",
         bin_hdr->max_num_add_tr_headers);
  printf("Time basis code: %d\n", bin_hdr->time_basis_code);
  printf("Number of traces in this file: %lu\n", bin_hdr->num_of_tr_in_file);
  printf("Byte offset of first trace: %lu\n", bin_hdr->byte_off_of_first_tr);
  printf("Number of trailer stanzas: %d\n", bin_hdr->num_of_trailer_stanza);
  av_segy_reader_delete(&sgy);
}
