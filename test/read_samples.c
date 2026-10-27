#include "AVSegyCommon.h"
#include "AVSegyReader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  void *samp = NULL;
  void **hdrs = NULL;
  int64_t *hdr_names = NULL;
  int hdrs_num;
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
  if (av_segy_reader_open(sgy, argv[1]))
    goto error;
  int32_t samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  uint64_t trc_num = 0, counter = 0;
  int bytes_per_samp = av_segy_reader_get_bytes_per_sample(sgy);
  if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver) {
    trc_num = av_segy_reader_get_number_of_traces(sgy);
    samp = calloc(samp_num * trc_num, bytes_per_samp);
    if (!samp) {
      printf("Can't allocate memory for samples");
      return 1;
    }
    void *ptr = samp;
    for (uint64_t i = 0; i < trc_num; ++i) {
      if (av_segy_reader_skip_headers(sgy))
        goto error;
      if (av_segy_reader_read_samples(sgy, ptr, samp_num))
        goto error;
      ptr += samp_num * bytes_per_samp;
    }
  } else {
    uint64_t tmp_trc_num = 1024;
    samp = calloc(samp_num * tmp_trc_num, bytes_per_samp);
    if (!samp) {
      printf("Can't allocate memory for samples");
      return 1;
    }
    if (bh->max_num_add_tr_headers)
      hdrs_num = 1;
    else
      hdrs_num = 2;
    hdrs = malloc(sizeof(void *) * hdrs_num);
    if (!hdrs)
      goto error;
    hdr_names = malloc(sizeof(int64_t) * hdrs_num);
    if (!hdr_names)
      goto error;
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
        goto error;
      }
      hdrs[i] = malloc(val_size);
      if (!hdrs[i])
        goto error;
    }
    void *ptr = samp;
    while (!av_segy_reader_end_of_data(sgy)) {
      ptr = samp + counter * samp_num;
      av_segy_reader_read_headers_single(sgy, hdrs, hdr_names, hdrs_num, 0);
      int16_t *i16_arr = hdrs[0];
      uint32_t var_samp_num = i16_arr[0];
      if (bh->max_num_add_tr_headers) {
        uint32_t *u32_arr = hdrs[1];
        uint32_t ext_samp_num = u32_arr[0];
        var_samp_num = ext_samp_num ? ext_samp_num : var_samp_num;
      }
      av_segy_reader_read_samples(sgy, ptr, var_samp_num);
      if (++counter == tmp_trc_num) {
        tmp_trc_num *= 2;
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
            printf("Already checked. Impossible error\n");
            return -1;
          }
          hdrs[i] = realloc(hdrs[i], tmp_trc_num * val_size);
          if (!hdrs[i])
            goto error;
        }
        samp = realloc(samp, samp_num * tmp_trc_num * bytes_per_samp);
        if (samp)
          goto error;
      }
    }
    trc_num = counter;
    samp = realloc(samp, samp_num * counter * bytes_per_samp);
    if (samp)
      goto error;
    if (hdrs) {
      for (int i = 0; i < hdrs_num; ++i) {
        free(hdrs[i]);
      }
      free(hdrs);
    }
    free(hdr_names);
  }
  av_segy_reader_samples_to_native(sgy, samp, samp_num * trc_num);
  float *arr2d = samp;
  for (uint64_t i = 0; i < trc_num; ++i) {
    if (arr2d[samp_num * (i + 1) - 2 - i] != -0.3681200444698333740234375) {
      printf("Error: %lu, %u\n", i, samp_num);
      goto error;
    }
  }
  free(samp);
  av_segy_reader_delete(&sgy);
  return 0;
error:
  printf("%s\n", err->message);
  if (hdrs) {
    for (int i = 0; i < hdrs_num; ++i) {
      free(hdrs[i]);
    }
    free(hdrs);
  }
  free(hdr_names);
  free(samp);
  av_segy_reader_delete(&sgy);
  return 1;
}
