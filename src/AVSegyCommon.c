#include "AVSegyCommon.h"

uint64_t const av_segy_std_trc_hdr_offset[AV_SEGY_STD_TRC_HDR_MAP_SIZE] = {
    0,   4,   8,   12,  16,  20,  24,  28,  30,  32,  34,  36,  40,  44,  48,
    52,  56,  60,  64,  68,  70,  72,  76,  80,  84,  88,  90,  92,  94,  96,
    98,  100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126,
    128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156,
    158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 184, 188, 192,
    196, 200, 202, 204, 208, 210, 212, 214, 216, 218, 220, 222, 224, 228, 230,
    240, 248, 256, 264, 272, 280, 288, 296, 304, 312, 320, 328, 336, 344, 352,
    360, 368, 376, 380, 384, 392, 396, 398, 400, 408};
int64_t const av_segy_std_trc_hdr_format[AV_SEGY_STD_TRC_HDR_MAP_SIZE] = {
    AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32,
    AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32,
    AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32, AV_SEGY_I32,
    AV_SEGY_I32, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I32, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_I16, AV_SEGY_I16, AV_SEGY_I32, AV_SEGY_I16, AV_SEGY_I16,
    AV_SEGY_U64, AV_SEGY_U64, AV_SEGY_I64, AV_SEGY_I64, AV_SEGY_F64,
    AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_F64,
    AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_F64,
    AV_SEGY_F64, AV_SEGY_F64, AV_SEGY_U32, AV_SEGY_I32, AV_SEGY_F64,
    AV_SEGY_I32, AV_SEGY_U16, AV_SEGY_U16, AV_SEGY_F64, AV_SEGY_F64};

char const *av_segy_default_text_header = "\
C 1 CLIENT                        COMPANY                       CREW NO         \
C 2 LINE            AREA                        MAP ID                          \
C 3 REEL NO           DAY-START OF REEL     YEAR      OBSERVER                  \
C 4 INSTRUMENT: MFG            MODEL            SERIAL NO                       \
C 5 DATA TRACES/RECORD        AUXILIARY TRACES/RECORD         CDP FOLD          \
C 6 SAMPLE INTERNAL         SAMPLES/TRACE       BITS/IN      BYTES/SAMPLE       \
C 7 RECORDING FORMAT        FORMAT THIS REEL        MEASUREMENT SYSTEM          \
C 8 SAMPLE CODE: FLOATING PT     FIXED PT     FIXED PT-GAIN     CORRELATED      \
C 9 GAIN  TYPE: FIXED     BINARY     FLOATING POINT     OTHER                   \
C10 FILTERS: ALIAS     HZ  NOTCH     HZ  BAND    -     HZ  SLOPE    -    DB/OCT \
C11 SOURCE: TYPE            NUMBER/POINT        POINT INTERVAL                  \
C12     PATTERN:                           LENGTH        WIDTH                  \
C13 SWEEP: START     HZ  END     HZ  LENGTH      MS  CHANNEL NO     TYPE        \
C14 TAPER: START LENGTH       MS  END LENGTH       MS  TYPE                     \
C15 SPREAD: OFFSET        MAX DISTANCE        GROUP INTERVAL                    \
C16 GEOPHONES: PER GROUP     SPACING     FREQUENCY     MFG          MODEL       \
C17     PATTERN:                           LENGTH        WIDTH                  \
C18 TRACES SORTED BY: RECORD     CDP     OTHER                                  \
C19 AMPLITUDE RECOVEY: NONE      SPHERICAL DIV       AGC    OTHER               \
C20 MAP PROJECTION                      ZONE ID       COORDINATE UNITS          \
C21 PROCESSING:                                                                 \
C22 PROCESSING:                                                                 \
C23                                                                             \
C24                                                                             \
C25                                                                             \
C26                                                                             \
C27                                                                             \
C28                                                                             \
C29                                                                             \
C30                                                                             \
C31                                                                             \
C32                                                                             \
C33                                                                             \
C34                                                                             \
C35                                                                             \
C36                                                                             \
C37                                                                             \
C38                                                                             \
C39 SEG Y REV1                                                                  \
C40 END EBCDIC                                                                  ";
