/**
 * \file AVSegyCommon.h
 * \brief Type and functions for common SEGY parts that could be shared.
 * \author andalevor
 * \date 2026\04\05
 */

#ifndef AV_SEGY_COMMON_H
#define AV_SEGY_COMMON_H

#include <stdint.h>

#define AV_SEGY_TEXT_HEADER_SIZE 3200
#define AV_SEGY_BIN_HEADER_SIZE 400
#define AV_SEGY_TRACE_HEADER_SIZE 240

/**
 * \enum AVSegyErrCode
 * \brief Enumeration for error codes.
 */
typedef enum AVSegyErrCode {
  AV_SEGY_ERR_OK,
  AV_SEGY_ERR_NO_MEM,
  AV_SEGY_ERR_FILE_OPEN,
  AV_SEGY_ERR_FILE_SEEK,
  AV_SEGY_ERR_FILE_READ,
  AV_SEGY_ERR_FILE_TELL,
  AV_SEGY_ERR_UNKNOWN_ENDIANNESS,
  AV_SEGY_ERR_UNSUPPORTED_FORMAT,
  AV_SEGY_ERR_BROKEN_FILE,
} AVSegyErrCode;

/**
 * \struct AVSegyErr
 * \brief Type for SEGY manipulations error checking.
 */
typedef struct AVSegyErr {
  AVSegyErrCode code;
  char *message;
} AVSegyErr;

/**
 * \struct AVSegyBinHdr
 * \brief SEGY binary header struct
 */
typedef struct AVSegyBinHdr {
  int32_t job_id;
  int32_t line_num;
  int32_t reel_num;
  int16_t tr_per_ens;
  int16_t aux_per_ens;
  int16_t samp_int;
  int16_t samp_int_orig;
  int16_t samp_per_tr;
  int16_t samp_per_tr_orig;
  int16_t format_code;
  int16_t ens_fold;
  int16_t sort_code;
  int16_t vert_sum_code;
  int16_t sw_freq_at_start;
  int16_t sw_freq_at_end;
  int16_t sw_length;
  int16_t sw_type_code;
  int16_t sw_ch_tr_num;
  int16_t taper_at_start;
  int16_t taper_at_end;
  int16_t taper_type;
  int16_t corr_traces;
  int16_t bin_gain_recov;
  int16_t amp_recov_meth;
  int16_t measure_system;
  int16_t impulse_sig_pol;
  int16_t vib_pol_code;
  int32_t ext_tr_per_ens;
  int32_t ext_aux_per_ens;
  int32_t ext_samp_per_tr;
  double ext_samp_int;
  double ext_samp_int_orig;
  int32_t ext_samp_per_tr_orig;
  int32_t ext_ens_fold;
  int32_t endianness;
  uint8_t SEGY_rev_major_ver;
  uint8_t SEGY_rev_minor_ver;
  int16_t fixed_tr_length;
  int16_t ext_text_headers_num;
  int32_t max_num_add_tr_headers;
  int16_t time_basis_code;
  uint64_t num_of_tr_in_file;
  uint64_t byte_off_of_first_tr;
  int32_t num_of_trailer_stanza;
} AVSegyBinHdr;

typedef enum AVSegyTrcHdrNames {
  AV_SEGY_TRC_SEQ_LINE,
  AV_SEGY_TRC_SEQ_SEGY,
  AV_SEGY_FFID,
  AV_SEGY_CHAN,
  AV_SEGY_ESP,
  AV_SEGY_ENS_NO,
  AV_SEGY_SEQ_NO,
  AV_SEGY_TRACE_ID,
  AV_SEGY_VERT_SUM,
  AV_SEGY_HOR_SUM,
  AV_SEGY_DATA_USE,
  AV_SEGY_OFFSET,
  AV_SEGY_R_ELEV,
  AV_SEGY_S_ELEV,
  AV_SEGY_S_DEPTH,
  AV_SEGY_R_DATUM,
  AV_SEGY_S_DATUM,
  AV_SEGY_S_WATER,
  AV_SEGY_R_WATER,
  AV_SEGY_ELEV_SCALAR,
  AV_SEGY_COORD_SCALAR,
  AV_SEGY_SOU_X,
  AV_SEGY_SOU_Y,
  AV_SEGY_REC_X,
  AV_SEGY_REC_Y,
  AV_SEGY_COORD_UNITS,
  AV_SEGY_WEATH_VEL,
  AV_SEGY_SUBWEATH_VEL,
  AV_SEGY_S_UPHOLE,
  AV_SEGY_R_UPHOLE,
  AV_SEGY_S_STAT,
  AV_SEGY_R_STAT,
  AV_SEGY_TOT_STAT,
  AV_SEGY_LAG_A,
  AV_SEGY_LAG_B,
  AV_SEGY_DELAY_TIME,
  AV_SEGY_MUTE_START,
  AV_SEGY_MUTE_END,
  AV_SEGY_SAMP_NUM,
  AV_SEGY_SAMP_INT,
  AV_SEGY_GAIN_TYPE,
  AV_SEGY_GAIN_CONST,
  AV_SEGY_INIT_GAIN,
  AV_SEGY_CORRELATED,
  AV_SEGY_SW_START,
  AV_SEGY_SW_END,
  AV_SEGY_SW_LENGTH,
  AV_SEGY_SW_TYPE,
  AV_SEGY_SW_TAPER_START,
  AV_SEGY_SW_TAPER_END,
  AV_SEGY_TAPER_TYPE,
  AV_SEGY_ALIAS_FILT_FREQ,
  AV_SEGY_ALIAS_FILT_SLOPE,
  AV_SEGY_NOTCH_FILT_FREQ,
  AV_SEGY_NOTCH_FILT_SLOPE,
  AV_SEGY_LOW_CUT_FREQ,
  AV_SEGY_HIGH_CUT_FREQ,
  AV_SEGY_LOW_CUT_SLOPE,
  AV_SEGY_HIGH_CUT_SLOPE,
  AV_SEGY_YEAR,
  AV_SEGY_DAY,
  AV_SEGY_HOUR,
  AV_SEGY_MINUTE,
  AV_SEGY_SECOND,
  AV_SEGY_TIME_BASIS_CODE,
  AV_SEGY_TRACE_WEIGHT,
  AV_SEGY_GROUP_NUM_ROLL,
  AV_SEGY_GROUP_NUM_FIRST,
  AV_SEGY_GROUP_NUM_LAST,
  AV_SEGY_GAP_SIZE,
  AV_SEGY_OVER_TRAVEL,
  AV_SEGY_CDP_X,
  AV_SEGY_CDP_Y,
  AV_SEGY_INLINE,
  AV_SEGY_XLINE,
  AV_SEGY_SP_NUM,
  AV_SEGY_SP_NUM_SCALAR,
  AV_SEGY_TR_VAL_UNIT,
  AV_SEGY_TRANS_CONST_MANT,
  AV_SEGY_TRANS_CONST_EXP,
  AV_SEGY_TRANS_UNITS,
  AV_SEGY_DEVICE_ID,
  AV_SEGY_TIME_SCALAR,
  AV_SEGY_SOURCE_TYPE,
  AV_SEGY_SOU_V_DIR,
  AV_SEGY_SOU_X_DIR,
  AV_SEGY_SOU_I_DIR,
  AV_SEGY_SOU_MEAS_MANT,
  AV_SEGY_SOU_MEAS_EXP,
  AV_SEGY_SOU_MEAS_UNIT,
  AV_SEGY_EXT_TRC_SEQ_LINE,
  AV_SEGY_EXT_TRC_SEQ_SEGY,
  AV_SEGY_EXT_FFID,
  AV_SEGY_EXT_ENS_NO,
  AV_SEGY_EXT_R_ELEV,
  AV_SEGY_R_DEPTH,
  AV_SEGY_EXT_S_ELEV,
  AV_SEGY_EXT_S_DEPTH,
  AV_SEGY_EXT_R_DATUM,
  AV_SEGY_EXT_S_DATUM,
  AV_SEGY_EXT_S_WATER,
  AV_SEGY_EXT_R_WATER,
  AV_SEGY_EXT_SOU_X,
  AV_SEGY_EXT_SOU_Y,
  AV_SEGY_EXT_REC_X,
  AV_SEGY_EXT_REC_Y,
  AV_SEGY_EXT_OFFSET,
  AV_SEGY_EXT_SAMP_NUM,
  AV_SEGY_NANOSECOND,
  AV_SEGY_EXT_SAMP_INT,
  AV_SEGY_CABLE_NUM,
  AV_SEGY_ADD_TRC_HDR_NUM,
  AV_SEGY_LAST_TRC_FLAG,
  AV_SEGY_EXT_CDP_X,
  AV_SEGY_EXT_CDP_Y,
} AVSegyTrcHdrNames;

typedef enum AVSegyTrcHdrTypes {
  AV_SEGY_I8,
  AV_SEGY_I16,
  AV_SEGY_I32,
  AV_SEGY_I64,
  AV_SEGY_U8,
  AV_SEGY_U16,
  AV_SEGY_U32,
  AV_SEGY_U64,
  AV_SEGY_F32,
  AV_SEGY_F64,
} AVSegyTrcHdrTypes;

#define AV_SEGY_STD_TRC_HDR_MAP_SIZE 115

extern uint64_t const av_segy_std_trc_hdr_offset[AV_SEGY_STD_TRC_HDR_MAP_SIZE];
extern int64_t const av_segy_std_trc_hdr_format[AV_SEGY_STD_TRC_HDR_MAP_SIZE];
extern char const *av_segy_default_text_header;

#endif /* AV_SEGY_COMMON_H */
