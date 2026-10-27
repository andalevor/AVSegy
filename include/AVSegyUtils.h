/**
 * \file AVSegyUtils.h
 * \brief functions for ascii and ebcdic conversion.
 * \author andalevor
 * \date 2026\04\04
 */

#ifndef AV_SEGY_UTILS_H
#define AV_SEGY_UTILS_H

#include <stddef.h>
#include <stdint.h>

/**
 * \fn ebcdic_to_ascii
 * \brief change every byte in string from ebcdic to ascii encoding.
 * \param s null terminated string
 */
void av_segy_ebcdic_to_ascii(char *buf, size_t size);

/**
 * \fn ascii_to_ebcdic
 * \brief change every byte in string from ascii to ebcdic encoding.
 * \param s null terminated string
 */
void av_segy_ascii_to_ebcdic(char *buf, size_t size);

/* byte swap */
#if defined(__GNUC__) || defined(__clang__)
#define AV_SEGY_HAVE_BUILTIN_BSWAP 1
#elif defined(_MSC_VER)
#include <stdlib.h>
#define AV_SEGY_HAVE_MSVC_BSWAP 1
#endif

static inline uint16_t av_segy_bswap16(uint16_t x) {
#if defined(AV_SEGY_HAVE_BUILTIN_BSWAP)
  return __builtin_bswap16(x);
#elif defined(AV_SEGY_HAVE_MSVC_BSWAP)
  return _byteswap_ushort(x);
#else
  return (x << 8) | (x >> 8);
#endif
}

static inline int16_t av_segy_bswap16s(int16_t x) {
  return (int16_t)av_segy_bswap16((uint16_t)x);
}

static inline uint32_t av_segy_bswap32(uint32_t x) {
#if defined(AV_SEGY_HAVE_BUILTIN_BSWAP)
  return __builtin_bswap32(x);
#elif defined(AV_SEGY_HAVE_MSVC_BSWAP)
  return _byteswap_ulong(x);
#else
  return ((x << 24) & 0xFF000000) | ((x << 8) & 0x00FF0000) |
         ((x >> 8) & 0x0000FF00) | ((x >> 24) & 0x000000FF);
#endif
}

static inline int32_t av_segy_bswap32s(int32_t x) {
  return (int32_t)av_segy_bswap32((uint32_t)x);
}

static inline uint64_t av_segy_bswap64(uint64_t x) {
#if defined(AV_SEGY_HAVE_BUILTIN_BSWAP)
  return __builtin_bswap64(x);
#elif defined(AV_SEGY_HAVE_MSVC_BSWAP)
  return _byteswap_uint64(x);
#else
  return ((x << 56) & 0xFF00000000000000ULL) |
         ((x << 40) & 0x00FF000000000000ULL) |
         ((x << 24) & 0x0000FF0000000000ULL) |
         ((x << 8) & 0x000000FF00000000ULL) |
         ((x >> 8) & 0x00000000FF000000ULL) |
         ((x >> 24) & 0x0000000000FF0000ULL) |
         ((x >> 40) & 0x000000000000FF00ULL) |
         ((x >> 56) & 0x00000000000000FFULL);
#endif
}

static inline int64_t av_segy_bswap64s(int64_t x) {
  return (int64_t)av_segy_bswap64((uint64_t)x);
}

void av_segy_bswap64s_arr(int64_t *arr, size_t samp_num);
void av_segy_bswap32s_arr(int32_t *arr, size_t samp_num);
void av_segy_bswap24s_arr(int32_t *arr, size_t samp_num);
void av_segy_bswap16s_arr(int16_t *arr, size_t samp_num);
void av_segy_bswap64_arr(uint64_t *arr, size_t samp_num);
void av_segy_bswap32_arr(uint32_t *arr, size_t samp_num);
void av_segy_bswap24_arr(uint32_t *arr, size_t samp_num);
void av_segy_bswap16_arr(uint16_t *arr, size_t samp_num);

#endif /* AV_SEGY_UTILS_H */
