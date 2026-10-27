/**
 * \file PyAVSegyError.h
 * \brief error handling for python
 * \author andalevor
 * \date 2026\07\07
 */

#ifndef PY_AV_SEGY_ERROR_H
#define PY_AV_SEGY_ERROR_H

#include "AVSegyCommon.h"

void py_av_segy_check_error(AVSegyErr const *err);

#endif /* PY_AV_SEGY_ERROR_H */
