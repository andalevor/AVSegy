#include "AVSegyCommon.h"
#include "AVSegyReader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  void **hdrs = NULL;
  int main_hdr_num = 90;
  int64_t *hdr_names = NULL;
  int8_t *i8_arr;
  uint8_t *u8_arr;
  int16_t *i16_arr;
  uint16_t *u16_arr;
  int32_t *i32_arr;
  uint32_t *u32_arr;
  int64_t *i64_arr;
  uint64_t *u64_arr;
  float *f32_arr;
  double *f64_arr;
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
  uint32_t samp_num = av_segy_reader_get_max_number_of_samples(sgy);
  AVSegyBinHdr const *bh = av_segy_reader_get_binary_header(sgy);
  uint64_t trc_num = 0, counter = 0;
  int64_t const *trc_hdr_format = av_segy_reader_get_trc_hdr_map_format(sgy);
  if (bh->fixed_tr_length || !bh->SEGY_rev_major_ver) {
    hdr_names = malloc(sizeof(int64_t) * main_hdr_num);
    if (!hdr_names)
      goto error;
    hdrs = malloc(sizeof(void *) * main_hdr_num);
    if (!hdrs)
      goto error;
    trc_num = av_segy_reader_get_number_of_traces(sgy);
    for (int i = 0; i < main_hdr_num; ++i) {
      hdr_names[i] = i;
      int fmt = trc_hdr_format[i];
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
      case AV_SEGY_F32:
        val_size = 4;
        break;
      case AV_SEGY_I64:
      case AV_SEGY_U64:
      case AV_SEGY_F64:
        val_size = 8;
        break;
      default:
        goto error;
      }
      hdrs[i] = malloc(trc_num * val_size);
      if (!hdrs[i])
        goto error;
    }
    for (uint64_t j = 0; j < trc_num; ++j) {
      if (av_segy_reader_read_headers_single(sgy, hdrs, hdr_names, main_hdr_num,
                                             j))
        goto error;
      if (av_segy_reader_skip_samples(sgy, samp_num))
        goto error;
    }
  } else {
    int64_t *hdr_names = NULL;
    int hdrs_num;
    if (bh->max_num_add_tr_headers)
      hdrs_num = main_hdr_num + 1;
    else
      hdrs_num = main_hdr_num + 2;
    hdr_names = malloc(sizeof(int64_t) * hdrs_num);
    if (!hdr_names)
      goto error;
    hdrs = malloc(sizeof(void *) * hdrs_num);
    if (!hdrs)
      goto error;
    uint64_t tmp_trc_num = 1024;
    for (int i = 0; i < main_hdr_num; ++i) {
      hdr_names[i] = i;
      int fmt = trc_hdr_format[i];
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
      case AV_SEGY_F32:
        val_size = 4;
        break;
      case AV_SEGY_I64:
      case AV_SEGY_U64:
      case AV_SEGY_F64:
        val_size = 8;
        break;
      default:
        goto error;
      }
      hdrs[i] = malloc(tmp_trc_num * val_size);
      if (!hdrs[i])
        goto error;
    }
    while (!av_segy_reader_end_of_data(sgy)) {
      hdr_names[main_hdr_num] = AV_SEGY_SAMP_NUM;
      hdrs[main_hdr_num] = malloc(tmp_trc_num * 2);
      if (!hdrs[main_hdr_num])
        goto error;
      if (bh->max_num_add_tr_headers) {
        hdr_names[main_hdr_num + 1] = AV_SEGY_EXT_SAMP_NUM;
        hdrs[main_hdr_num + 1] = malloc(tmp_trc_num * 4);
        if (!hdrs[main_hdr_num + 1])
          goto error;
      }
      if (av_segy_reader_read_headers_single(sgy, hdrs, hdr_names, hdrs_num,
                                             counter++))
        goto error;
      i16_arr = hdrs[main_hdr_num];
      samp_num = i16_arr[counter - 1];
      if (bh->max_num_add_tr_headers) {
        u32_arr = hdrs[main_hdr_num + 1];
        uint32_t ext_samp_num = u32_arr[counter - 1];
        samp_num = ext_samp_num ? ext_samp_num : samp_num;
      }
      if (av_segy_reader_skip_samples(sgy, samp_num))
        goto error;
      if (counter == tmp_trc_num) {
        tmp_trc_num *= 2;
        for (int i = 0; i < main_hdr_num; ++i) {
          int fmt = trc_hdr_format[i];
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
          case AV_SEGY_F32:
            val_size = 4;
            break;
          case AV_SEGY_I64:
          case AV_SEGY_U64:
          case AV_SEGY_F64:
            val_size = 8;
            break;
          default:
            goto error;
          }
          hdrs[i] = realloc(hdrs[i], tmp_trc_num * val_size);
          if (!hdrs[i])
            goto error;
        }
      }
    }
    free(hdrs[main_hdr_num]);
    if (bh->max_num_add_tr_headers)
      free(hdrs[main_hdr_num + 1]);
    for (int i = 0; i < main_hdr_num; ++i) {
      int fmt = trc_hdr_format[i];
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
      case AV_SEGY_F32:
        val_size = 4;
        break;
      case AV_SEGY_I64:
      case AV_SEGY_U64:
      case AV_SEGY_F64:
        val_size = 8;
        break;
      default:
        goto error;
      }
      hdrs[i] = realloc(hdrs[i], counter * val_size);
      if (!hdrs[i])
        goto error;
    }
  }
  for (int i = 0; i < main_hdr_num; ++i) {
    int fmt = trc_hdr_format[i];
    switch (fmt) {
    case AV_SEGY_I8:
      i8_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%d ", i8_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_U8:
      u8_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%u ", u8_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_I16:
      i16_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%d ", i16_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_U16:
      u16_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%u ", u16_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_I32:
      i32_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%d ", i32_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_U32:
      u32_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%u ", u32_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_I64:
      i64_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%ld ", i64_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_U64:
      u64_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%lu ", u64_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_F32:
      f32_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%f ", f32_arr[j]);
      }
      printf("\n");
      break;
    case AV_SEGY_F64:
      f64_arr = hdrs[i];
      for (uint64_t j = 0; j < trc_num; ++j) {
        printf("%f ", f64_arr[j]);
      }
      printf("\n");
      break;
    }
  }
  for (int i = 0; i < main_hdr_num; ++i) {
    free(hdrs[i]);
  }
  free(hdrs);
  free(hdr_names);
  av_segy_reader_delete(&sgy);
  return 0;
error:
  printf("%s\n", err->message);
  if (hdrs) {
    for (int i = 0; i < main_hdr_num; ++i) {
      free(hdrs[i]);
    }
    free(hdrs);
  }
  free(hdr_names);
  av_segy_reader_delete(&sgy);
  return 1;
}
