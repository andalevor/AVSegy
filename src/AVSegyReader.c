#include "AVSegyReader.h"
#include "AVSegyCommon.h"
#include "AVSegyUtils.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AVSegyErrCode reader_open(AVSegyReader *restrict sgy,
                                 char const *restrict file_name,
                                 AVSegyBinHdr const *restrict over);
static AVSegyErrCode fill_bin_hdr(AVSegyReader *sgy);
static AVSegyErrCode assign_raw_readers(AVSegyReader *sgy);
static AVSegyErrCode assign_bytes_per_sample(AVSegyReader *sgy);
static AVSegyErrCode get_first_trc_pos(AVSegyReader *sgy);
static AVSegyErrCode get_data_end_pos(AVSegyReader *sgy);
static AVSegyErrCode read_trc_hdrs_group_fix(AVSegyReader *restrict sgy,
                                             void **restrict hdrs,
                                             int64_t *restrict hdr_names,
                                             size_t hdr_names_len,
                                             size_t trc_num);
static AVSegyErrCode read_trc_hdrs_single_fix(AVSegyReader *restrict sgy,
                                              void **restrict hdrs,
                                              int64_t *restrict hdr_names,
                                              size_t hdr_names_len,
                                              size_t trc_num);
static AVSegyErrCode read_trc_hdrs_group_var(AVSegyReader *restrict sgy,
                                             void **restrict hdrs,
                                             int64_t *restrict hdr_names,
                                             size_t hdr_names_len,
                                             size_t trc_num);
static AVSegyErrCode read_trc_hdrs_single_var(AVSegyReader *restrict sgy,
                                              void **restrict hdrs,
                                              int64_t *restrict hdr_names,
                                              size_t hdr_names_len,
                                              size_t trc_num);
static AVSegyErrCode skip_trc_hdrs_fix(AVSegyReader *sgy);
static AVSegyErrCode skip_trc_hdrs_var(AVSegyReader *sgy);

static int8_t read_i8(char const **buf);
static uint8_t read_u8(char const **buf);
static int16_t read_i16(char const **buf);
static uint16_t read_u16(char const **buf);
static int32_t read_i24(char const **buf);
static uint32_t read_u24(char const **buf);
static int32_t read_i32(char const **buf);
static uint32_t read_u32(char const **buf);
static int64_t read_i64(char const **buf);
static uint64_t read_u64(char const **buf);
static int16_t read_i16_sw(char const **buf);
static uint16_t read_u16_sw(char const **buf);
static int32_t read_i24_sw(char const **buf);
static uint32_t read_u24_sw(char const **buf);
static int32_t read_i32_sw(char const **buf);
static uint32_t read_u32_sw(char const **buf);
static int64_t read_i64_sw(char const **buf);
static uint64_t read_u64_sw(char const **buf);
static double read_ieee_double_native(AVSegyReader *sgy, char const **buf);
static double read_ieee_double(AVSegyReader *sgy, char const **buf);
static float read_ieee_float_native(AVSegyReader *sgy, char const **buf);
static float read_ieee_float(AVSegyReader *sgy, char const **buf);
static void ibm_float_decode(void *buf, size_t samp_num);
static void ieee32_decode(AVSegyReader *sgy, void *buf, size_t samp_num);
static void ieee64_decode(AVSegyReader *sgy, void *buf, size_t samp_num);

struct AVSegyReader {
  AVSegyErr err;
  FILE *file;
  AVSegyBinHdr bh;
  long first_trc_pos;
  long current_pos;
  long data_end_pos;
  int bytes_per_sample;
  int32_t samp_per_tr;
  int8_t (*read_i8)(char const **buf);
  uint8_t (*read_u8)(char const **buf);
  int16_t (*read_i16)(char const **buf);
  uint16_t (*read_u16)(char const **buf);
  int32_t (*read_i24)(char const **buf);
  uint32_t (*read_u24)(char const **buf);
  int32_t (*read_i32)(char const **buf);
  uint32_t (*read_u32)(char const **buf);
  int64_t (*read_i64)(char const **buf);
  uint64_t (*read_u64)(char const **buf);
  double (*read_ieee_double)(AVSegyReader *sgy, char const **buf);
  float (*read_ieee_float)(AVSegyReader *sgy, char const **buf);
  AVSegyErrCode (*read_trc_hdrs_group)(AVSegyReader *sgy, void **hdrs,
                                       int64_t *hdr_names, size_t hdr_names_len,
                                       size_t trc_num);
  AVSegyErrCode (*read_trc_hdrs_single)(AVSegyReader *sgy, void **hdrs,
                                        int64_t *hdr_names,
                                        size_t hdr_names_len, size_t trc_num);
  AVSegyErrCode (*skip_trc_hdrs)(AVSegyReader *sgy);
  char trc_hdr_buf[AV_SEGY_TEXT_HEADER_SIZE];
  uint64_t *trc_hdr_offset;
  int64_t *trc_hdr_format;
  size_t trc_hdr_map_size;
};

AVSegyReader *av_segy_reader_new(void) {
  AVSegyReader *sgy = (AVSegyReader *)malloc(sizeof(AVSegyReader));
  if (!sgy)
    return NULL;
  sgy->trc_hdr_offset = NULL;
  sgy->err.code = AV_SEGY_ERR_OK;
  sgy->err.message = "";
  sgy->file = NULL;
  sgy->trc_hdr_map_size = AV_SEGY_STD_TRC_HDR_MAP_SIZE;
  memset(&sgy->bh, 0, sizeof(AVSegyBinHdr));
  sgy->trc_hdr_format = NULL;
  sgy->trc_hdr_offset = malloc(sizeof(uint64_t) * sgy->trc_hdr_map_size);
  if (!sgy->trc_hdr_offset)
    goto error;
  sgy->trc_hdr_format = malloc(sizeof(int64_t) * sgy->trc_hdr_map_size);
  if (!sgy->trc_hdr_format)
    goto error;
  memcpy(sgy->trc_hdr_offset, av_segy_std_trc_hdr_offset,
         sizeof(uint64_t) * sgy->trc_hdr_map_size);
  memcpy(sgy->trc_hdr_format, av_segy_std_trc_hdr_format,
         sizeof(int64_t) * sgy->trc_hdr_map_size);
  return sgy;
error:
  if (sgy->trc_hdr_format)
    free(sgy->trc_hdr_format);
  free(sgy->trc_hdr_offset);
  return NULL;
}

void av_segy_reader_delete(AVSegyReader **sgy) {
  if (sgy && *sgy) {
    if ((*sgy)->file)
      fclose((*sgy)->file);
    free(*sgy);
  }
}

AVSegyErr const *av_segy_reader_get_error(AVSegyReader const *sgy) {
  return &sgy->err;
}

int32_t av_segy_reader_get_max_number_of_samples(AVSegyReader const *sgy) {
  return sgy->samp_per_tr;
}

int av_segy_reader_get_bytes_per_sample(AVSegyReader const *sgy) {
  return sgy->bytes_per_sample;
}

bool av_segy_reader_is_fix_trace_length(AVSegyReader const *sgy) {
  return sgy->bh.fixed_tr_length || !sgy->bh.SEGY_rev_major_ver;
}

AVSegyErrCode av_segy_reader_open(AVSegyReader *sgy, char const *file_name) {
  return reader_open(sgy, file_name, NULL);
}

AVSegyErrCode av_segy_reader_open_override(AVSegyReader *sgy,
                                           char const *file_name,
                                           AVSegyBinHdr const *restrict over) {
  return reader_open(sgy, file_name, over);
}

AVSegyErrCode reader_open(AVSegyReader *restrict sgy,
                          char const *restrict file_name,
                          AVSegyBinHdr const *restrict over) {
  if (sgy->file)
    fclose(sgy->file);
  sgy->file = fopen(file_name, "rb");
  if (!sgy->file) {
    sgy->err.code = AV_SEGY_ERR_FILE_OPEN;
    sgy->err.message = "Can not open file for reading";
    goto error;
  }
  if (over)
    sgy->bh = *over;
  else if (fill_bin_hdr(sgy))
    goto error;
  if (assign_bytes_per_sample(sgy))
    goto error;
  if (get_first_trc_pos(sgy))
    goto error;
  if (get_data_end_pos(sgy))
    goto error;
  if (sgy->bh.fixed_tr_length || !sgy->bh.SEGY_rev_major_ver) {
    sgy->read_trc_hdrs_group = read_trc_hdrs_group_fix;
    sgy->read_trc_hdrs_single = read_trc_hdrs_single_fix;
    sgy->skip_trc_hdrs = skip_trc_hdrs_fix;
  } else {
    sgy->read_trc_hdrs_group = read_trc_hdrs_group_var;
    sgy->read_trc_hdrs_single = read_trc_hdrs_single_var;
    sgy->skip_trc_hdrs = skip_trc_hdrs_var;
  }
  if (fseek(sgy->file, sgy->first_trc_pos, SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't seek to first trace position";
    goto error;
  }
  sgy->current_pos = sgy->first_trc_pos;
  return sgy->err.code;
error:
  return sgy->err.code;
}

AVSegyErrCode av_segy_reader_read_raw_text_header(AVSegyReader *sgy,
                                                  char *buf) {
  if (fseek(sgy->file, 0, SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't seek to the beginning of file";
    goto error;
  }
  size_t rd = fread(buf, 1, AV_SEGY_TEXT_HEADER_SIZE, sgy->file);
  if (rd != AV_SEGY_TEXT_HEADER_SIZE) {
    sgy->err.code = AV_SEGY_ERR_FILE_READ;
    sgy->err.message = "Can't read text header";
    goto error;
  }
  if (fseek(sgy->file, sgy->current_pos, SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't seek back to original position";
    goto error;
  }
error:
  return sgy->err.code;
}

AVSegyErrCode av_segy_reader_read_text_header(AVSegyReader *sgy, char *buf) {
  if (av_segy_reader_read_raw_text_header(sgy, buf))
    goto error;
  if ((uint8_t)buf[0] == 0xC3) {
    av_segy_ebcdic_to_ascii(buf, AV_SEGY_TEXT_HEADER_SIZE);
  }
error:
  return sgy->err.code;
}

AVSegyBinHdr const *av_segy_reader_get_binary_header(AVSegyReader const *sgy) {
  return &sgy->bh;
}

uint64_t av_segy_reader_get_number_of_traces(AVSegyReader const *sgy) {
  return (sgy->data_end_pos - sgy->current_pos) /
         ((sgy->bh.max_num_add_tr_headers + 1) * AV_SEGY_TRACE_HEADER_SIZE +
          sgy->samp_per_tr * sgy->bytes_per_sample);
}

AVSegyErrCode read_trc_hdrs_group_fix(AVSegyReader *sgy, void **hdrs,
                                      int64_t *hdr_names, size_t hdr_names_len,
                                      size_t trc_num) {
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
  for (uint32_t j = 0; j < (uint32_t)sgy->bh.max_num_add_tr_headers + 1; ++j) {
    size_t read =
        fread(sgy->trc_hdr_buf, 1, AV_SEGY_TRACE_HEADER_SIZE, sgy->file);
    if (read != AV_SEGY_TRACE_HEADER_SIZE) {
      sgy->err.code = AV_SEGY_ERR_FILE_READ;
      sgy->err.message = "Can't read trace header";
      goto error;
    }
    sgy->current_pos += read;
    for (size_t i = 0; i < hdr_names_len; ++i) {
      int64_t hdr_name = hdr_names[i];
      int format = sgy->trc_hdr_format[hdr_name];
      uint64_t offset = sgy->trc_hdr_offset[hdr_name];
      if (offset < AV_SEGY_TRACE_HEADER_SIZE * j ||
          offset >= AV_SEGY_TRACE_HEADER_SIZE * (j + 1))
        continue;
      char const *ptr =
          sgy->trc_hdr_buf + offset - AV_SEGY_TRACE_HEADER_SIZE * j;
      switch (format) {
      case AV_SEGY_I8:
        i8_arr = hdrs[i];
        i8_arr[trc_num] = sgy->read_i8(&ptr);
        break;
      case AV_SEGY_U8:
        u8_arr = hdrs[i];
        u8_arr[trc_num] = sgy->read_u8(&ptr);
        break;
      case AV_SEGY_I16:
        i16_arr = hdrs[i];
        i16_arr[trc_num] = sgy->read_i16(&ptr);
        break;
      case AV_SEGY_U16:
        u16_arr = hdrs[i];
        u16_arr[trc_num] = sgy->read_u16(&ptr);
        break;
      case AV_SEGY_I32:
        i32_arr = hdrs[i];
        i32_arr[trc_num] = sgy->read_i32(&ptr);
        break;
      case AV_SEGY_U32:
        u32_arr = hdrs[i];
        u32_arr[trc_num] = sgy->read_u32(&ptr);
        break;
      case AV_SEGY_I64:
        i64_arr = hdrs[i];
        i64_arr[trc_num] = sgy->read_i64(&ptr);
        break;
      case AV_SEGY_U64:
        u64_arr = hdrs[i];
        u64_arr[trc_num] = sgy->read_u64(&ptr);
        break;
      case AV_SEGY_F32:
        f32_arr = hdrs[i];
        f32_arr[trc_num] = sgy->read_ieee_float(sgy, &ptr);
        break;
      case AV_SEGY_F64:
        f64_arr = hdrs[i];
        f64_arr[trc_num] = sgy->read_ieee_double(sgy, &ptr);
        break;
      default:
        sgy->err.code = AV_SEGY_ERR_UNSUPPORTED_FORMAT;
        sgy->err.message = "This header type isn't in the list of supported";
        goto error;
      }
    }
  }
error:
  return sgy->err.code;
}
AVSegyErrCode read_trc_hdrs_single_fix(AVSegyReader *sgy, void **hdrs,
                                       int64_t *hdr_names, size_t hdr_names_len,
                                       size_t trc_num) {
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
  size_t read;
  char const *ptr;
  uint64_t start_pos = sgy->current_pos;
  for (size_t i = 0; i < hdr_names_len; ++i) {
    int64_t hdr_name = hdr_names[i];
    int format = sgy->trc_hdr_format[hdr_name];
    int offset = sgy->trc_hdr_offset[hdr_name];
    if (fseek(sgy->file, start_pos + offset, SEEK_SET)) {
      sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
      sgy->err.message = "Can't seek back to next header";
      goto error;
    }
    switch (format) {
    case AV_SEGY_I8:
      i8_arr = hdrs[i];
      read = fread(&i8_arr[trc_num], 1, sizeof(int8_t), sgy->file);
      if (read != sizeof(int8_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header u8";
        goto error;
      }
      ptr = (char const *)&i8_arr[trc_num];
      i8_arr[trc_num] = sgy->read_i8(&ptr);
      break;
    case AV_SEGY_U8:
      u8_arr = hdrs[i];
      read = fread(&u8_arr[trc_num], 1, sizeof(uint8_t), sgy->file);
      if (read != sizeof(uint8_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header u8";
        goto error;
      }
      ptr = (char const *)&u8_arr[trc_num];
      u8_arr[trc_num] = sgy->read_u8(&ptr);
      break;
    case AV_SEGY_I16:
      i16_arr = hdrs[i];
      read = fread(&i16_arr[trc_num], 1, sizeof(int16_t), sgy->file);
      if (read != sizeof(int16_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header i16";
        goto error;
      }
      ptr = (char const *)&i16_arr[trc_num];
      i16_arr[trc_num] = sgy->read_i16(&ptr);
      break;
    case AV_SEGY_U16:
      u16_arr = hdrs[i];
      read = fread(&u16_arr[trc_num], 1, sizeof(uint16_t), sgy->file);
      if (read != sizeof(uint16_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header u16";
        goto error;
      }
      ptr = (char const *)&u16_arr[trc_num];
      u16_arr[trc_num] = sgy->read_u16(&ptr);
      break;
    case AV_SEGY_I32:
      i32_arr = hdrs[i];
      read = fread(&i32_arr[trc_num], 1, sizeof(int32_t), sgy->file);
      if (read != sizeof(int32_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header i32";
        goto error;
      }
      ptr = (char const *)&i32_arr[trc_num];
      i32_arr[trc_num] = sgy->read_i32(&ptr);
      break;
    case AV_SEGY_U32:
      u32_arr = hdrs[i];
      read = fread(&u32_arr[trc_num], 1, sizeof(uint32_t), sgy->file);
      if (read != sizeof(uint32_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header u32";
        goto error;
      }
      ptr = (char const *)&u32_arr[trc_num];
      u32_arr[trc_num] = sgy->read_u32(&ptr);
      break;
    case AV_SEGY_I64:
      i64_arr = hdrs[i];
      read = fread(&i64_arr[trc_num], 1, sizeof(int64_t), sgy->file);
      if (read != sizeof(int64_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header i64";
        goto error;
      }
      ptr = (char const *)&i64_arr[trc_num];
      i64_arr[trc_num] = sgy->read_i64(&ptr);
      break;
    case AV_SEGY_U64:
      u64_arr = hdrs[i];
      read = fread(&u64_arr[trc_num], 1, sizeof(uint64_t), sgy->file);
      if (read != sizeof(uint64_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header u64";
        goto error;
      }
      ptr = (char const *)&u64_arr[trc_num];
      u64_arr[trc_num] = sgy->read_u64(&ptr);
      break;
    case AV_SEGY_F32:
      f32_arr = hdrs[i];
      read = fread(&f32_arr[trc_num], 1, sizeof(uint32_t), sgy->file);
      if (read != sizeof(uint32_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header f32";
        goto error;
      }
      ptr = (char const *)&f32_arr[trc_num];
      f32_arr[trc_num] = sgy->read_ieee_float(sgy, &ptr);
      break;
    case AV_SEGY_F64:
      f64_arr = hdrs[i];
      read = fread(&f64_arr[trc_num], 1, sizeof(uint64_t), sgy->file);
      if (read != sizeof(uint64_t)) {
        sgy->err.code = AV_SEGY_ERR_FILE_READ;
        sgy->err.message = "Can't read trace header f64";
        goto error;
      }
      ptr = (char const *)&f64_arr[trc_num];
      f64_arr[trc_num] = sgy->read_ieee_double(sgy, &ptr);
      break;
    default:
      sgy->err.code = AV_SEGY_ERR_UNSUPPORTED_FORMAT;
      sgy->err.message = "This header type isn't in the list of supported";
      goto error;
    }
  }
  if (fseek(sgy->file,
            start_pos + AV_SEGY_TRACE_HEADER_SIZE *
                            (sgy->bh.max_num_add_tr_headers + 1),
            SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't seek back to end of trace header";
    goto error;
  }
  sgy->current_pos +=
      AV_SEGY_TRACE_HEADER_SIZE * (sgy->bh.max_num_add_tr_headers + 1);
error:
  return sgy->err.code;
}
AVSegyErrCode read_trc_hdrs_group_var(AVSegyReader *sgy, void **hdrs,
                                      int64_t *hdr_names, size_t hdr_names_len,
                                      size_t trc_num) {
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
  uint64_t trc_hdrs_num = sgy->bh.max_num_add_tr_headers + 1;
  for (uint32_t j = 0; j < trc_hdrs_num; ++j) {
    size_t read =
        fread(sgy->trc_hdr_buf, 1, AV_SEGY_TRACE_HEADER_SIZE, sgy->file);
    if (read != AV_SEGY_TRACE_HEADER_SIZE) {
      sgy->err.code = AV_SEGY_ERR_FILE_READ;
      sgy->err.message = "Can't read trace header";
      goto error;
    }
    if (j == 1) {
      char const *ptr = sgy->trc_hdr_buf + 156;
      trc_hdrs_num = sgy->read_u16(&ptr);
    }
    sgy->current_pos += read;
    for (size_t i = 0; i < hdr_names_len; ++i) {
      int64_t hdr_name = hdr_names[i];
      int format = sgy->trc_hdr_format[hdr_name];
      uint64_t offset = sgy->trc_hdr_offset[hdr_name];
      if (offset < AV_SEGY_TRACE_HEADER_SIZE * j ||
          offset >= AV_SEGY_TRACE_HEADER_SIZE * (j + 1))
        continue;
      char const *ptr =
          sgy->trc_hdr_buf + offset - AV_SEGY_TRACE_HEADER_SIZE * j;
      switch (format) {
      case AV_SEGY_I8:
        i8_arr = hdrs[i];
        i8_arr[trc_num] = sgy->read_i8(&ptr);
        break;
      case AV_SEGY_U8:
        u8_arr = hdrs[i];
        u8_arr[trc_num] = sgy->read_u8(&ptr);
        break;
      case AV_SEGY_I16:
        i16_arr = hdrs[i];
        i16_arr[trc_num] = sgy->read_i16(&ptr);
        break;
      case AV_SEGY_U16:
        u16_arr = hdrs[i];
        u16_arr[trc_num] = sgy->read_u16(&ptr);
        break;
      case AV_SEGY_I32:
        i32_arr = hdrs[i];
        i32_arr[trc_num] = sgy->read_i32(&ptr);
        break;
      case AV_SEGY_U32:
        u32_arr = hdrs[i];
        u32_arr[trc_num] = sgy->read_u32(&ptr);
        break;
      case AV_SEGY_I64:
        i64_arr = hdrs[i];
        i64_arr[trc_num] = sgy->read_i64(&ptr);
        break;
      case AV_SEGY_U64:
        u64_arr = hdrs[i];
        u64_arr[trc_num] = sgy->read_u64(&ptr);
        break;
      case AV_SEGY_F32:
        f32_arr = hdrs[i];
        f32_arr[trc_num] = sgy->read_ieee_float(sgy, &ptr);
        break;
      case AV_SEGY_F64:
        f64_arr = hdrs[i];
        f64_arr[trc_num] = sgy->read_ieee_double(sgy, &ptr);
        break;
      default:
        sgy->err.code = AV_SEGY_ERR_UNSUPPORTED_FORMAT;
        sgy->err.message = "This header type isn't in the list of supported";
        goto error;
      }
    }
  }
error:
  return sgy->err.code;
}
AVSegyErrCode read_trc_hdrs_single_var(AVSegyReader *sgy, void **hdrs,
                                       int64_t *hdr_names, size_t hdr_names_len,
                                       size_t trc_num) {
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
  size_t read;
  int val_size;
  uint64_t start_pos = sgy->current_pos;
  uint16_t trc_hdrs_num = 1;
  char const *ptr;
  if (sgy->bh.max_num_add_tr_headers) {
    if (fseek(sgy->file, 396, SEEK_CUR)) {
      sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
      sgy->err.message = "Can't seek back to next header";
      goto error;
    }
    char buf[2];
    read = fread(buf, 1, sizeof(uint16_t), sgy->file);
    if (read != sizeof(uint16_t)) {
      sgy->err.code = AV_SEGY_ERR_FILE_READ;
      sgy->err.message = "Can't read trace header u16";
      goto error;
    }
    char const *ptr = buf;
    trc_hdrs_num += sgy->read_u16(&ptr) + 1;
  }
  for (size_t i = 0; i < hdr_names_len; ++i) {
    int64_t hdr_name = hdr_names[i];
    int format = sgy->trc_hdr_format[hdr_name];
    int offset = sgy->trc_hdr_offset[hdr_name];
    if (fseek(sgy->file, start_pos + offset, SEEK_SET)) {
      sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
      sgy->err.message = "Can't seek back to next header";
      goto error;
    }
    switch (format) {
    case AV_SEGY_I8:
      i8_arr = hdrs[i];
      val_size = 1;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        i8_arr[trc_num] = 0;
      } else {
        read = fread(&i8_arr[trc_num], 1, sizeof(int8_t), sgy->file);
        if (read != sizeof(int8_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header u8";
          goto error;
        }
        ptr = (char const *)&i8_arr[trc_num];
        i8_arr[trc_num] = sgy->read_i8(&ptr);
      }
      break;
    case AV_SEGY_U8:
      u8_arr = hdrs[i];
      val_size = 1;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        u8_arr[trc_num] = 0;
      } else {
        read = fread(&u8_arr[trc_num], 1, sizeof(uint8_t), sgy->file);
        if (read != sizeof(uint8_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header u8";
          goto error;
        }
        ptr = (char const *)&u8_arr[trc_num];
        u8_arr[trc_num] = sgy->read_u8(&ptr);
      }
      break;
    case AV_SEGY_I16:
      i16_arr = hdrs[i];
      val_size = 2;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        i16_arr[trc_num] = 0;
      } else {
        read = fread(&i16_arr[trc_num], 1, sizeof(int16_t), sgy->file);
        if (read != sizeof(int16_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header i16";
          goto error;
        }
        ptr = (char const *)&i16_arr[trc_num];
        i16_arr[trc_num] = sgy->read_i16(&ptr);
      }
      break;
    case AV_SEGY_U16:
      u16_arr = hdrs[i];
      val_size = 2;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        u16_arr[trc_num] = 0;
      } else {
        read = fread(&u16_arr[trc_num], 1, sizeof(uint16_t), sgy->file);
        if (read != sizeof(uint16_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header u16";
          goto error;
        }
        ptr = (char const *)&u16_arr[trc_num];
        u16_arr[trc_num] = sgy->read_u16(&ptr);
      }
      break;
    case AV_SEGY_I32:
      i32_arr = hdrs[i];
      val_size = 4;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        i32_arr[trc_num] = 0;
      } else {
        read = fread(&i32_arr[trc_num], 1, sizeof(int32_t), sgy->file);
        if (read != sizeof(int32_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header i32";
          goto error;
        }
        ptr = (char const *)&i32_arr[trc_num];
        i32_arr[trc_num] = sgy->read_i32(&ptr);
      }
      break;
    case AV_SEGY_U32:
      u32_arr = hdrs[i];
      val_size = 4;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        u32_arr[trc_num] = 0;
      } else {
        read = fread(&u32_arr[trc_num], 1, sizeof(uint32_t), sgy->file);
        if (read != sizeof(uint32_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header u32";
          goto error;
        }
        ptr = (char const *)&u32_arr[trc_num];
        u32_arr[trc_num] = sgy->read_u32(&ptr);
      }
      break;
    case AV_SEGY_I64:
      i64_arr = hdrs[i];
      val_size = 8;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        i64_arr[trc_num] = 0;
      } else {
        read = fread(&i64_arr[trc_num], 1, sizeof(int64_t), sgy->file);
        if (read != sizeof(int64_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header i64";
          goto error;
        }
        ptr = (char const *)&i64_arr[trc_num];
        i64_arr[trc_num] = sgy->read_i64(&ptr);
      }
      break;
    case AV_SEGY_U64:
      u64_arr = hdrs[i];
      val_size = 8;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        u64_arr[trc_num] = 0;
      } else {
        read = fread(&u64_arr[trc_num], 1, sizeof(uint64_t), sgy->file);
        if (read != sizeof(uint64_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header u64";
          goto error;
        }
        ptr = (char const *)&u64_arr[trc_num];
        u64_arr[trc_num] = sgy->read_u64(&ptr);
      }
      break;
    case AV_SEGY_F32:
      f32_arr = hdrs[i];
      val_size = 4;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        f32_arr[trc_num] = 0;
      } else {
        read = fread(&f32_arr[trc_num], 1, sizeof(uint32_t), sgy->file);
        if (read != sizeof(uint32_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header f32";
          goto error;
        }
        ptr = (char const *)&f32_arr[trc_num];
        f32_arr[trc_num] = sgy->read_ieee_float(sgy, &ptr);
      }
      break;
    case AV_SEGY_F64:
      f64_arr = hdrs[i];
      val_size = 8;
      if (offset + val_size > trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE) {
        f64_arr[trc_num] = 0;
      } else {
        read = fread(&f64_arr[trc_num], 1, sizeof(uint64_t), sgy->file);
        if (read != sizeof(uint64_t)) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header f64";
          goto error;
        }
        ptr = (char const *)&f64_arr[trc_num];
        f64_arr[trc_num] = sgy->read_ieee_double(sgy, &ptr);
      }
      break;
    default:
      sgy->err.code = AV_SEGY_ERR_UNSUPPORTED_FORMAT;
      sgy->err.message = "This header type isn't in the list of supported";
      goto error;
    }
  }
  if (fseek(sgy->file, start_pos + AV_SEGY_TRACE_HEADER_SIZE * trc_hdrs_num,
            SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't seek back to end of trace header";
    goto error;
  }
  sgy->current_pos += AV_SEGY_TRACE_HEADER_SIZE * trc_hdrs_num;
error:
  return sgy->err.code;
}

AVSegyErrCode av_segy_reader_read_headers_single(AVSegyReader *sgy, void **hdrs,
                                                 int64_t *hdr_names,
                                                 size_t hdr_names_len,
                                                 size_t trc_num) {
  return sgy->read_trc_hdrs_single(sgy, hdrs, hdr_names, hdr_names_len,
                                   trc_num);
}

AVSegyErrCode av_segy_reader_read_headers_group(AVSegyReader *sgy, void **hdrs,
                                                int64_t *hdr_names,
                                                size_t hdr_names_len,
                                                size_t trc_num) {
  return sgy->read_trc_hdrs_group(sgy, hdrs, hdr_names, hdr_names_len, trc_num);
}

AVSegyErrCode skip_trc_hdrs_fix(AVSegyReader *sgy) {
  int32_t hdrs_num = sgy->bh.max_num_add_tr_headers + 1;
  if (fseek(sgy->file, hdrs_num * AV_SEGY_TRACE_HEADER_SIZE, SEEK_CUR)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't skip trace headers";
    goto error;
  }
  sgy->current_pos += hdrs_num * AV_SEGY_TRACE_HEADER_SIZE;
error:
  return sgy->err.code;
}

AVSegyErrCode skip_trc_hdrs_var(AVSegyReader *sgy) {
  uint64_t start_pos = sgy->current_pos;
  uint16_t trc_hdrs_num = 1;
  if (sgy->bh.max_num_add_tr_headers) {
    if (fseek(sgy->file, 396, SEEK_CUR)) {
      sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
      sgy->err.message = "Can't seek back to next header";
      goto error;
    }
    char buf[2];
    size_t read = fread(buf, 1, sizeof(uint16_t), sgy->file);
    if (read != sizeof(uint16_t)) {
      sgy->err.code = AV_SEGY_ERR_FILE_READ;
      sgy->err.message = "Can't read trace header u16";
      goto error;
    }
    char const *ptr = buf;
    trc_hdrs_num += sgy->read_u16(&ptr);
  }
  if (fseek(sgy->file, start_pos + AV_SEGY_TRACE_HEADER_SIZE * trc_hdrs_num,
            SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't skip trace headers";
    goto error;
  }
  sgy->current_pos += trc_hdrs_num * AV_SEGY_TRACE_HEADER_SIZE;
error:
  return sgy->err.code;
}

AVSegyErrCode av_segy_reader_skip_headers(AVSegyReader *sgy) {
  return sgy->skip_trc_hdrs(sgy);
}

AVSegyErrCode av_segy_reader_read_samples(AVSegyReader *sgy, void *samp,
                                          uint32_t samp_num) {
  size_t rd = fread(samp, 1, samp_num * sgy->bytes_per_sample, sgy->file);
  if (rd != samp_num * sgy->bytes_per_sample) {
    sgy->err.code = AV_SEGY_ERR_FILE_READ;
    sgy->err.message = "Error while samples reading";
    goto error;
  }
  sgy->current_pos += rd;
error:
  return sgy->err.code;
}

bool av_segy_reader_end_of_data(AVSegyReader *sgy) {
  return sgy->current_pos == sgy->data_end_pos;
}

AVSegyErrCode av_segy_reader_skip_samples(AVSegyReader *sgy, int32_t samp_num) {
  if (fseek(sgy->file, samp_num * sgy->bytes_per_sample, SEEK_CUR)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't skip trace samples";
    goto error;
  }
  sgy->current_pos += samp_num * sgy->bytes_per_sample;
error:
  return sgy->err.code;
}

AVSegyErrCode av_segy_reader_samples_to_native(AVSegyReader *sgy, void *samp,
                                               size_t samp_num) {
  switch (sgy->bh.format_code) {
  case 1:
    ibm_float_decode(samp, samp_num);
    break;
  case 2:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap32s_arr(samp, samp_num);
    break;
  case 3:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap16s_arr(samp, samp_num);
    break;
  case 5:
    if (!(FLT_RADIX == 2 && DBL_MANT_DIG == 53))
      ieee32_decode(sgy, samp, samp_num);
    else if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap32_arr(samp, samp_num);
    break;
  case 6:
    if (!(FLT_RADIX == 2 && DBL_MANT_DIG == 53))
      ieee64_decode(sgy, samp, samp_num);
    else if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap64_arr(samp, samp_num);
    break;
  case 7:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap24s_arr(samp, samp_num);
    break;
  case 8:
    /* do nothing */
    break;
  case 9:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap64s_arr(samp, samp_num);
    break;
  case 10:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap32_arr(samp, samp_num);
    break;
  case 11:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap16_arr(samp, samp_num);
    break;
  case 12:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap64_arr(samp, samp_num);
    break;
  case 15:
    if (!sgy->bh.endianness || sgy->bh.endianness == 0x040302010)
      av_segy_bswap24_arr(samp, samp_num);
    break;
  case 16:
    /* do nothing */
    break;
  default:
    sgy->err.code = AV_SEGY_ERR_UNSUPPORTED_FORMAT;
    sgy->err.message = "unknown format code in binary header";
  }
  return sgy->err.code;
}

AVSegyErrCode av_segy_reader_rewind(AVSegyReader *sgy) {
  sgy->current_pos = sgy->first_trc_pos;
  if (fseek(sgy->file, sgy->first_trc_pos, SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't set file position";
  }
  return sgy->err.code;
}

uint64_t const *av_segy_reader_get_trc_hdr_map_offset(AVSegyReader *sgy) {
  return sgy->trc_hdr_offset;
}
int64_t const *av_segy_reader_get_trc_hdr_map_format(AVSegyReader *sgy) {
  return sgy->trc_hdr_format;
}
size_t av_segy_reader_get_trc_hdr_map_size(AVSegyReader *sgy) {
  return sgy->trc_hdr_map_size;
}

AVSegyErrCode av_segy_reader_trc_hdr_remap(AVSegyReader *sgy,
                                           uint64_t const *restrict offset,
                                           int64_t const *restrict format,
                                           int64_t *restrict hdr_names,
                                           size_t remap_size) {
  size_t new_size = sgy->trc_hdr_map_size + remap_size;
  if (!realloc(sgy->trc_hdr_format, sizeof(int64_t) * new_size)) {
    sgy->err.message = "Can't allocate memory for trace header remap";
    return sgy->err.code = AV_SEGY_ERR_NO_MEM;
  }
  memcpy(sgy->trc_hdr_format + sgy->trc_hdr_map_size, format, remap_size);
  if (!realloc(sgy->trc_hdr_offset, sizeof(int64_t) * new_size)) {
    sgy->err.message = "Can't allocate memory for trace header remap";
    return sgy->err.code = AV_SEGY_ERR_NO_MEM;
  }
  memcpy(sgy->trc_hdr_offset + sgy->trc_hdr_map_size, offset, remap_size);
  for (size_t i = 0; i < remap_size; ++i)
    hdr_names[i] = sgy->trc_hdr_map_size + i;
  sgy->trc_hdr_map_size = new_size;
  return sgy->err.code;
}
AVSegyErrCode fill_bin_hdr(AVSegyReader *sgy) {
  if (fseek(sgy->file, AV_SEGY_TEXT_HEADER_SIZE, SEEK_SET)) {
    sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
    sgy->err.message = "Can't seek to binary header";
    goto error;
  }
  char bin_buf[AV_SEGY_BIN_HEADER_SIZE];
  size_t rd = fread(bin_buf, 1, AV_SEGY_BIN_HEADER_SIZE, sgy->file);
  if (rd != AV_SEGY_BIN_HEADER_SIZE) {
    sgy->err.code = AV_SEGY_ERR_FILE_READ;
    sgy->err.message = "Read less than binary header size";
    goto error;
  }
  AVSegyBinHdr *bh = &sgy->bh;
  memcpy(&bh->endianness, bin_buf + 96, sizeof(int32_t));
  if (assign_raw_readers(sgy))
    goto error;
  char const *ptr = bin_buf + 300;
  bh->SEGY_rev_major_ver = sgy->read_u8(&ptr);
  bh->SEGY_rev_minor_ver = sgy->read_u8(&ptr);
  if (bh->SEGY_rev_major_ver) {
    bh->fixed_tr_length = sgy->read_i16(&ptr);
    bh->ext_text_headers_num = sgy->read_i16(&ptr);
  }
  if (bh->SEGY_rev_major_ver > 1) {
    bh->max_num_add_tr_headers = sgy->read_i32(&ptr);
    bh->time_basis_code = sgy->read_i16(&ptr);
    bh->num_of_tr_in_file = sgy->read_i64(&ptr);
    bh->byte_off_of_first_tr = sgy->read_u64(&ptr);
    bh->num_of_trailer_stanza = sgy->read_i32(&ptr);
  }
  ptr = bin_buf;
  bh->job_id = sgy->read_i32(&ptr);
  bh->line_num = sgy->read_i32(&ptr);
  bh->reel_num = sgy->read_i32(&ptr);
  bh->tr_per_ens = sgy->read_i16(&ptr);
  bh->aux_per_ens = sgy->read_i16(&ptr);
  bh->samp_int = sgy->read_i16(&ptr);
  bh->samp_int_orig = sgy->read_i16(&ptr);
  bh->samp_per_tr = sgy->read_i16(&ptr);
  bh->samp_per_tr_orig = sgy->read_i16(&ptr);
  bh->format_code = sgy->read_i16(&ptr);
  bh->ens_fold = sgy->read_i16(&ptr);
  bh->sort_code = sgy->read_i16(&ptr);
  bh->vert_sum_code = sgy->read_i16(&ptr);
  bh->sw_freq_at_start = sgy->read_i16(&ptr);
  bh->sw_freq_at_end = sgy->read_i16(&ptr);
  bh->sw_length = sgy->read_i16(&ptr);
  bh->sw_type_code = sgy->read_i16(&ptr);
  bh->sw_ch_tr_num = sgy->read_i16(&ptr);
  bh->taper_at_start = sgy->read_i16(&ptr);
  bh->taper_at_end = sgy->read_i16(&ptr);
  bh->taper_type = sgy->read_i16(&ptr);
  bh->corr_traces = sgy->read_i16(&ptr);
  bh->bin_gain_recov = sgy->read_i16(&ptr);
  bh->amp_recov_meth = sgy->read_i16(&ptr);
  bh->measure_system = sgy->read_i16(&ptr);
  bh->impulse_sig_pol = sgy->read_i16(&ptr);
  bh->vib_pol_code = sgy->read_i16(&ptr);
  if (bh->SEGY_rev_major_ver > 1) {
    bh->ext_tr_per_ens = sgy->read_i32(&ptr);
    bh->ext_aux_per_ens = sgy->read_i32(&ptr);
    bh->ext_samp_per_tr = sgy->read_i32(&ptr);
    bh->ext_samp_int = sgy->read_ieee_double(sgy, &ptr);
    bh->ext_samp_int_orig = sgy->read_ieee_double(sgy, &ptr);
    bh->ext_samp_per_tr_orig = sgy->read_i32(&ptr);
    bh->ext_ens_fold = sgy->read_i32(&ptr);
    sgy->samp_per_tr =
        bh->ext_samp_per_tr ? bh->ext_samp_per_tr : bh->samp_per_tr;
  } else {
    sgy->samp_per_tr = bh->samp_per_tr;
  }
error:
  return sgy->err.code;
}

AVSegyErrCode assign_raw_readers(AVSegyReader *sgy) {
  sgy->read_i8 = read_i8;
  sgy->read_u8 = read_u8;
  switch (sgy->bh.endianness) {
  case 0x01020304:
    sgy->read_i16 = read_i16;
    sgy->read_u16 = read_u16;
    sgy->read_i24 = read_i24;
    sgy->read_u24 = read_u24;
    sgy->read_i32 = read_i32;
    sgy->read_u32 = read_u32;
    sgy->read_i64 = read_i64;
    sgy->read_u64 = read_u64;
    break;
  case 0:
  case 0x04030201:
    sgy->read_i16 = read_i16_sw;
    sgy->read_u16 = read_u16_sw;
    sgy->read_i24 = read_i24_sw;
    sgy->read_u24 = read_u24_sw;
    sgy->read_i32 = read_i32_sw;
    sgy->read_u32 = read_u32_sw;
    sgy->read_i64 = read_i64_sw;
    sgy->read_u64 = read_u64_sw;
    break;
  default:
    sgy->err.code = AV_SEGY_ERR_UNKNOWN_ENDIANNESS;
    sgy->err.message = "unsupported endianness";
  }
  if (FLT_RADIX == 2 && DBL_MANT_DIG == 53) {
    sgy->read_ieee_double = read_ieee_double_native;
    sgy->read_ieee_float = read_ieee_float_native;
  } else {
    sgy->read_ieee_double = read_ieee_double;
    sgy->read_ieee_float = read_ieee_float;
  }
  return sgy->err.code;
}

AVSegyErrCode get_first_trc_pos(AVSegyReader *sgy) {
  if (sgy->bh.byte_off_of_first_tr) {
    sgy->first_trc_pos = sgy->bh.byte_off_of_first_tr;
    goto success;
  }
  int num = sgy->bh.ext_text_headers_num;
  if (!num) {
    sgy->first_trc_pos = ftell(sgy->file);
    if (sgy->first_trc_pos == -1L) {
      sgy->err.code = AV_SEGY_ERR_FILE_TELL;
      sgy->err.message = "Errot while getting position of first trace";
      goto error;
    }
  } else {
    if (num == -1) {
      char *end_stanza = "((SEG: EndText))";
      char *buf = (char *)malloc(AV_SEGY_TEXT_HEADER_SIZE);
      if (!buf) {
        sgy->err.code = AV_SEGY_ERR_NO_MEM;
        sgy->err.message =
            "Can't allocate memory to read external text headers";
        goto error;
      }
      while (1) {
        size_t rd = fread(buf, 1, AV_SEGY_TEXT_HEADER_SIZE, sgy->file);
        if (rd != AV_SEGY_TEXT_HEADER_SIZE) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Error while extended text header reading";
          goto error;
        }
        if (strstr(buf, end_stanza)) {
          sgy->first_trc_pos = ftell(sgy->file);
          if (sgy->first_trc_pos == -1L) {
            sgy->err.code = AV_SEGY_ERR_FILE_TELL;
            sgy->err.message = "Errot while getting position of first trace";
            goto error;
          }
          break;
        }
      }
      free(buf);
    } else {
      if (fseek(sgy->file, AV_SEGY_TEXT_HEADER_SIZE * num, SEEK_SET)) {
        sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
        sgy->err.message = "Can't seek to first trace position";
        goto error;
      }
      sgy->first_trc_pos = ftell(sgy->file);
      if (sgy->first_trc_pos == -1L) {
        sgy->err.code = AV_SEGY_ERR_FILE_TELL;
        sgy->err.message = "Errot while getting position of first trace";
        goto error;
      }
    }
  }
success:
  return sgy->err.code;
error:
  return sgy->err.code;
}

AVSegyErrCode assign_bytes_per_sample(AVSegyReader *sgy) {
  switch (sgy->bh.format_code) {
  case 1:
  case 2:
  case 4:
  case 5:
  case 10:
    sgy->bytes_per_sample = 4;
    break;
  case 3:
  case 11:
    sgy->bytes_per_sample = 2;
    break;
  case 6:
  case 9:
  case 12:
    sgy->bytes_per_sample = 8;
    break;
  case 7:
  case 15:
    sgy->bytes_per_sample = 3;
    break;
  case 8:
  case 16:
    sgy->bytes_per_sample = 1;
    break;
  default:
    sgy->err.code = AV_SEGY_ERR_UNSUPPORTED_FORMAT;
    sgy->err.message = "unknown format code in binary header";
  }
  return sgy->err.code;
}

AVSegyErrCode get_data_end_pos(AVSegyReader *sgy) {
  int32_t stanz_num = sgy->bh.num_of_trailer_stanza;
  /* just go to the end and get position */
  if (!stanz_num) {
    if (fseek(sgy->file, 0, SEEK_END)) {
      sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
      sgy->err.message = "Can't seek to the end of file";
      goto error;
    }
    sgy->data_end_pos = ftell(sgy->file);
    if (sgy->first_trc_pos == -1L) {
      sgy->err.code = AV_SEGY_ERR_FILE_TELL;
      sgy->err.message = "Errot while getting position of first trace";
      goto error;
    }
    /* if number of stanzas is unknown */
  } else if (stanz_num == -1) {
    /* we need to know number of traces to skip them */
    if (!sgy->bh.num_of_tr_in_file) {
      sgy->err.code = AV_SEGY_ERR_BROKEN_FILE;
      sgy->err.message = "unable to determine end of trace data";
      goto error;
    }
    /* if traces has fixed length */
    if (sgy->bh.fixed_tr_length) {
      /* assums that fixed length trace should have
       * fixed additional trace headers */
      sgy->data_end_pos = sgy->bytes_per_sample * sgy->samp_per_tr +
                          AV_SEGY_TRACE_HEADER_SIZE *
                              (sgy->bh.max_num_add_tr_headers + 1) *
                              sgy->bh.num_of_tr_in_file +
                          sgy->first_trc_pos;
      /* if traces has variable trace length */
    } else {
      for (uint64_t i = 0; i < sgy->bh.num_of_tr_in_file; ++i) {
        char buf[AV_SEGY_TRACE_HEADER_SIZE];
        size_t rd = fread(buf, 1, AV_SEGY_TRACE_HEADER_SIZE, sgy->file);
        if (rd != AV_SEGY_TRACE_HEADER_SIZE) {
          sgy->err.code = AV_SEGY_ERR_FILE_READ;
          sgy->err.message = "Can't read trace header";
          goto error;
        }
        char const *ptr = buf + 114;
        uint32_t trc_samp_num = sgy->read_i16(&ptr);
        if (sgy->bh.max_num_add_tr_headers) {
          size_t rd = fread(buf, 1, AV_SEGY_TRACE_HEADER_SIZE, sgy->file);
          if (rd != AV_SEGY_TRACE_HEADER_SIZE) {
            sgy->err.code = AV_SEGY_ERR_FILE_READ;
            sgy->err.message = "Can't read trace header";
            goto error;
          }
          ptr = buf + 136;
          trc_samp_num = sgy->read_u32(&ptr);
          ptr = buf + 156;
          uint16_t add_tr_hdr_num = sgy->read_i16(&ptr);
          add_tr_hdr_num =
              add_tr_hdr_num ? add_tr_hdr_num : sgy->bh.max_num_add_tr_headers;
          if (fseek(sgy->file, (add_tr_hdr_num - 1) * AV_SEGY_TRACE_HEADER_SIZE,
                    SEEK_CUR)) {
            sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
            sgy->err.message = "Can't seek to the end of trace headers";
            goto error;
          }
        }
        if (fseek(sgy->file, trc_samp_num * sgy->bytes_per_sample, SEEK_CUR)) {
          sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
          sgy->err.message = "Can't seek to the end od trace";
          goto error;
        }
      }
      sgy->data_end_pos = ftell(sgy->file);
      if (sgy->data_end_pos == -1L) {
        sgy->err.code = AV_SEGY_ERR_FILE_TELL;
        sgy->err.message = "Errot while getting position of first trace";
        goto error;
      }
    }
    /* if we know number of stanzas just skip them */
  } else {
    if (fseek(sgy->file,
              sgy->bh.num_of_trailer_stanza * AV_SEGY_TEXT_HEADER_SIZE,
              SEEK_END)) {
      sgy->err.code = AV_SEGY_ERR_FILE_SEEK;
      sgy->err.message = "Can't seek to the end of data in file";
      goto error;
    }
    sgy->data_end_pos = ftell(sgy->file);
    if (sgy->data_end_pos == -1L) {
      sgy->err.code = AV_SEGY_ERR_FILE_TELL;
      sgy->err.message = "Errot while getting position of first trace";
      goto error;
    }
  }
error:
  return sgy->err.code;
}

int8_t read_i8(char const **buf) {
  int8_t res;
  memcpy(&res, *buf, sizeof(int8_t));
  ++*buf;
  return res;
}

uint8_t read_u8(char const **buf) {
  uint8_t res;
  memcpy(&res, *buf, sizeof(uint8_t));
  ++*buf;
  return res;
}

int16_t read_i16(char const **buf) {
  int16_t res;
  memcpy(&res, *buf, sizeof(int16_t));
  *buf += sizeof(int16_t);
  return res;
}

uint16_t read_u16(char const **buf) {
  uint16_t res;
  memcpy(&res, *buf, sizeof(uint16_t));
  *buf += sizeof(uint16_t);
  return res;
}

int32_t read_i24(char const **buf) {
  int32_t res;
  memcpy(&res, *buf, sizeof(int16_t) + sizeof(int8_t));
  *buf += sizeof(int16_t) + sizeof(int8_t);
  return res;
}

uint32_t read_u24(char const **buf) {
  uint32_t res;
  memcpy(&res, *buf, sizeof(uint16_t) + sizeof(uint8_t));
  *buf += sizeof(uint16_t) + sizeof(uint8_t);
  return res;
}

int32_t read_i32(char const **buf) {
  int32_t res;
  memcpy(&res, *buf, sizeof(int32_t));
  *buf += sizeof(int32_t);
  return res;
}

uint32_t read_u32(char const **buf) {
  uint32_t res;
  memcpy(&res, *buf, sizeof(uint32_t));
  *buf += sizeof(uint32_t);
  return res;
}

int64_t read_i64(char const **buf) {
  int64_t res;
  memcpy(&res, *buf, sizeof(int64_t));
  *buf += sizeof(int64_t);
  return res;
}

uint64_t read_u64(char const **buf) {
  uint64_t res;
  memcpy(&res, *buf, sizeof(uint64_t));
  *buf += sizeof(uint64_t);
  return res;
}

int16_t read_i16_sw(char const **buf) {
  uint16_t res;
  memcpy(&res, *buf, sizeof(int16_t));
  *buf += sizeof(uint16_t);
  return av_segy_bswap16s(res);
}

uint16_t read_u16_sw(char const **buf) {
  uint16_t res;
  memcpy(&res, *buf, sizeof(uint16_t));
  *buf += sizeof(uint16_t);
  return av_segy_bswap16(res);
}

int32_t read_i24_sw(char const **buf) {
  uint32_t res = 0;
  memcpy(&res, *buf, sizeof(int16_t) + sizeof(int8_t));
  *buf += sizeof(int16_t) + sizeof(int8_t);
  return av_segy_bswap32s(res);
}

uint32_t read_u24_sw(char const **buf) {
  uint32_t res = 0;
  memcpy(&res, *buf, sizeof(uint16_t) + sizeof(uint8_t));
  *buf += sizeof(uint16_t) + sizeof(uint8_t);
  return av_segy_bswap32(res);
}

int32_t read_i32_sw(char const **buf) {
  uint32_t res;
  memcpy(&res, *buf, sizeof(int32_t));
  *buf += sizeof(uint32_t);
  return av_segy_bswap32s(res);
}

uint32_t read_u32_sw(char const **buf) {
  uint32_t res;
  memcpy(&res, *buf, sizeof(int32_t));
  *buf += sizeof(uint32_t);
  return av_segy_bswap32(res);
}

int64_t read_i64_sw(char const **buf) {
  uint64_t res;
  memcpy(&res, *buf, sizeof(int64_t));
  *buf += sizeof(uint64_t);
  return av_segy_bswap64s(res);
}

uint64_t read_u64_sw(char const **buf) {
  uint64_t res;
  memcpy(&res, *buf, sizeof(int64_t));
  *buf += sizeof(uint64_t);
  return av_segy_bswap64(res);
}

double read_ieee_double(AVSegyReader *sgy, char const **buf) {
  uint64_t tmp = sgy->read_u64(buf);
  int sign = tmp >> 63;
  int exp = (tmp & 0x7fffffffffffffff) >> 52;
  uint64_t fraction = tmp & 0x000fffffffffffff;
  double res = ldexp((double)(1 + fraction) / (2ull << 52), exp - 1023);
  return sign ? -res : res;
}

double read_ieee_double_native(AVSegyReader *sgy, char const **buf) {
  uint64_t tmp = sgy->read_u64(buf);
  double result;
  memcpy(&result, &tmp, sizeof(result));
  return result;
}

float read_ieee_float(AVSegyReader *sgy, char const **buf) {
  uint32_t tmp = sgy->read_u32(buf);
  int sign = tmp >> 31;
  int exp = (tmp & 0x7fffffff) >> 23;
  uint32_t fraction = tmp & 0x7fffff;
  float res = sign * ldexpf(1 + (float)fraction / (1 << 23), exp - 127);
  return sign ? -res : res;
}

float read_ieee_float_native(AVSegyReader *sgy, char const **buf) {
  uint32_t tmp = sgy->read_u32(buf);
  float result;
  memcpy(&result, &tmp, sizeof(result));
  return result;
}

static const uint32_t SHIFT_TABLE = 0 | (3 << 2) | (2 << 4) | (2 << 6) |
                                    (1 << 8) | (1 << 10) | (1 << 12) |
                                    (1 << 14);
static void ibm_float_decode(void *buf, size_t samp_num) {
  uint32_t x;
  float *p = buf;
  for (size_t i = 0; i < samp_num; ++i) {
    memcpy(&x, p, sizeof(uint32_t));
    x = __builtin_bswap32(x);
    uint32_t frac = x & 0x00ffffffu;
    if (frac == 0) {
      uint32_t bits = x & 0x80000000u;
      memcpy(p++, &bits, sizeof bits);
      continue;
    }
    unsigned shift = (SHIFT_TABLE >> ((frac >> 20) * 2)) & 3;
    uint32_t mant = frac << shift;
    int e = 4 * ((x >> 24) & 0x7fu) - 130 - shift;
    uint32_t bits = (x & 0x80000000u) | (e << 23) | (mant & 0x007fffffu);
    memcpy(p++, &bits, sizeof bits);
  }
}

void ieee32_decode(AVSegyReader *sgy, void *buf, size_t samp_num) {
  float *dest = buf;
  char const *src = buf;
  for (uint32_t i = 0; i < samp_num; ++i)
    *dest++ = read_ieee_float(sgy, &src);
}

void ieee64_decode(AVSegyReader *sgy, void *buf, size_t samp_num) {
  double *dest = buf;
  char const *src = buf;
  for (uint32_t i = 0; i < samp_num; ++i)
    *dest++ = read_ieee_double(sgy, &src);
}
