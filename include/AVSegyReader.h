/**
 * \file AVSegyReader.h
 * \brief Type and functions for common SEGY parts that could be shared.
 * \author andalevor
 * \date 2026\04\07
 */

#ifndef AV_SEGY_READER_H
#define AV_SEGY_READER_H

#include "AVSegyCommon.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * \struct AVSegyReader
 * \brief Struct to hold information for SEGY reading.
 */
typedef struct AVSegyReader AVSegyReader;

/**
 * \fn av_segy_reader_new
 * \brief Initialize AVSegyReader instance.
 * \return NULLable.
 */
AVSegyReader *av_segy_reader_new(void);

/**
 * \fn av_segy_reader_delete
 * \brief Frees memory.
 * \param sgy Pointer to AVSegyReader object.
 */
void av_segy_reader_delete(AVSegyReader **sgy);

/**
 * \fn av_segy_reader_get_error
 * \brief Gets AVSegyErr structure for error checking.
 * \return nonNULL. You should not free this memory.
 */
AVSegyErr const *av_segy_reader_get_error(AVSegyReader const *sgy);

/**
 * \fn av_segy_reader_get_max_number_of_samples
 * \brief Could be used to get samples number after file open
 * \return Number of sample
 */
int32_t av_segy_reader_get_max_number_of_samples(AVSegyReader const *sgy);

/**
 * \fn av_segy_reader_get_bytes_per_sample
 * \brief Could be used to number of bytes per sample after file open
 * \return Number of bytes per sample
 */
int av_segy_reader_get_bytes_per_sample(AVSegyReader const *sgy);

/**
 * \fn av_segy_reader_open
 * \brief Opens file, loads headers, prepares for trace reading.
 * \param sgy AVSegyReader instance.
 * \param file_name Name of file to open.
 * \return Error code.
 */
AVSegyErrCode av_segy_reader_open(AVSegyReader *restrict sgy,
                                  char const *restrict file_name);

/**
 * \fn av_segy_reader_open
 * \brief Opens file, loads headers, prepares for trace reading.
 * \param sgy AVSegyReader instance.
 * \param file_name Name of file to open.
 * \return Error code.
 */
AVSegyErrCode
av_segy_reader_open_override(AVSegyReader *restrict sgy,
                             char const *restrict file_name,
                             AVSegyBinHdr const *restrict bin_hdr);

/**
 * \fn av_segy_reader_read_raw_text_header
 * \brief Reads 3200 bytes of text header as is
 * \param sgy AVSegyReader instance.
 * \param buf Buffer to read to. Must be at least 3200 byte long
 * \return Error code
 */
AVSegyErrCode av_segy_reader_read_raw_text_header(AVSegyReader *restrict sgy,
                                                  char *restrict buf);

/**
 * \fn av_segy_reader_read_text_header
 * \brief Reads 3200 bytes of text header and tries to guess encoding
 * \param sgy AVSegyReader instance.
 * \param buf Buffer to read to. Must be at least 3200 byte long
 * \return Error code.
 */
AVSegyErrCode av_segy_reader_read_text_header(AVSegyReader *restrict sgy,
                                              char *restrict buf);

/**
 * \fn av_segy_reader_get_binary_header
 * \brief Getter for struct AVSegyBinHdr
 * \param sgy AVSegyReader instance.
 * \return nonNULL. You should not free this memory.
 */
AVSegyBinHdr const *av_segy_reader_get_binary_header(AVSegyReader const *sgy);

/**
 * \fn av_segy_reader_get_number_of_traces
 * \brief Use only for fixed trace length files. Calculates from current
 * position
 * \param sgy AVSegyReader instance.
 * \return number of trace in file
 */
uint64_t av_segy_reader_get_number_of_traces(AVSegyReader const *sgy);

/**
 * \fn av_segy_reader_read_headers_single
 * \brief Reads headers by reading only needed values
 * \param sgy AVSegyReader instance.
 * \param samp Pointer to buffer to store headers
 * \param samp_num Number of samples to read
 * \return Error code
 */
AVSegyErrCode av_segy_reader_read_headers_single(AVSegyReader *restrict sgy,
                                                 void **restrict hdrs,
                                                 int64_t *restrict hdr_names,
                                                 size_t hdr_names_len,
                                                 size_t trc_num);

/**
 * \fn av_segy_reader_read_headers_group
 * \brief Reads headers by reading all the header
 * \param sgy AVSegyReader instance.
 * \param samp Pointer to buffer to store headers
 * \param samp_num Number of samples to read
 * \return Error code
 */
AVSegyErrCode av_segy_reader_read_headers_group(AVSegyReader *restrict sgy,
                                                void **restrict hdrs,
                                                int64_t *restrict hdr_names,
                                                size_t hdr_names_len,
                                                size_t trc_num);

/**
 * \fn av_segy_reader_skip_headers
 * \brief Skips trace headers. Can be used to read only samples
 * \param sgy AVSegyReader instance.
 * \return Error code
 */
AVSegyErrCode av_segy_reader_skip_headers(AVSegyReader *sgy);

/**
 * \fn av_segy_reader_read_samples
 * \brief Reads samples
 * \param sgy AVSegyReader instance.
 * \param samp Pointer to buffer to store the samples
 * \param samp_num Number of samples to read
 * \return Error code
 */
AVSegyErrCode av_segy_reader_read_samples(AVSegyReader *restrict sgy,
                                          void *restrict samp,
                                          uint32_t samp_num);

/**
 * \fn av_segy_reader_skip_samples
 * \brief Skips trace samples. Can be used to read headers only
 * \param sgy AVSegyReader instance.
 * \param samp_num Number of samples to skip
 * \return Error code
 */
AVSegyErrCode av_segy_reader_skip_samples(AVSegyReader *sgy, int32_t samp_num);

/**
 * \fn av_segy_reader_rewind
 * \brief Sets reading position to the first trace of file
 * \param sgy AVSegyReader instance.
 * \return Error code
 */
AVSegyErrCode av_segy_reader_rewind(AVSegyReader *sgy);

/**
 * \fn av_segy_reader_end_of_data
 * \brief Checks if it's possible to read trace
 * \param sgy AVSegyReader instance.
 * \return True if end of data reached
 */
bool av_segy_reader_end_of_data(AVSegyReader *sgy);

/**
 * \fn av_segy_reader_decode_samples
 * \brief Transfors read data to native types
 * \param sgy AVSegyReader instance.
 * \param samp Pointer to buffer to write samples
 * \param samp_num Number of samples to read
 * \return Error code
 */
AVSegyErrCode av_segy_reader_samples_to_native(AVSegyReader *restrict sgy,
                                               void *restrict samp,
                                               size_t samp_num);

/**
 * \fn av_segy_reader_get_trc_hdr_map_offset
 * \brief Getter for trace header map offsets
 * \param sgy AVSegyReader instance.
 * \return Pointer to array of offsets
 */
uint64_t const *av_segy_reader_get_trc_hdr_map_offset(AVSegyReader *sgy);

/**
 * \fn av_segy_reader_get_trc_hdr_map_format
 * \brief Getter for trace header map formats
 * \param sgy AVSegyReader instance.
 * \return Pointer to array of formats
 */
int64_t const *av_segy_reader_get_trc_hdr_map_format(AVSegyReader *sgy);

/**
 * \fn av_segy_reader_get_trc_hdr_map_size
 * \brief Getter for trace header map size
 * \param sgy AVSegyReader instance.
 * \return Size of offsets and formats arrays
 */
size_t av_segy_reader_get_trc_hdr_map_size(AVSegyReader *sgy);

/**
 * \fn av_segy_reader_trc_hdr_remap
 * \brief Adds new pairs of offsets and formats. Invalidates offset and format
 * pointers.
 * \param sgy AVSegyReader instance.
 * \param offset Array of new offsets
 * \param format Array of new formats
 * \param hdr_names Array to write new headers names. Shoud be the same size as
 * other arrays
 * \return Error code
 */
AVSegyErrCode av_segy_reader_trc_hdr_remap(AVSegyReader *sgy,
                                           uint64_t const *restrict offset,
                                           int64_t const *restrict format,
                                           int64_t *restrict hdr_names,
                                           size_t remap_size);

#endif /* AV_SEGY_READER_H */
