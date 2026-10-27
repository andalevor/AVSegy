/**
 * \file PyAVSegySizeMac.h
 * \brief simple macro to map fixed size type to python macro
 * \author andalevor
 * \date 2026\09\21
 */

#ifndef SIZEMAC_H
#define SIZEMAC_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <limits.h>
#include <stdint.h>

#if INT32_MAX == INT_MAX
#define Py_T_INT32 Py_T_INT
#define Py_T_UINT32 Py_T_UINT
#elif INT32_MAX == LONG_MAX
#define Py_T_INT32 Py_T_LONG
#define Py_T_UINT32 Py_T_ULONG
#else
#error "No suitable 32-bit integer type found"
#endif

#if INT64_MAX == LONG_MAX
#define Py_T_INT64 Py_T_LONG
#define Py_T_UINT64 Py_T_ULONG
#elif INT64_MAX == LLONG_MAX
#define Py_T_INT64 Py_T_LONGLONG
#define Py_T_UINT64 Py_T_ULONGLONG
#else
#error "No suitable 64-bit integer type found"
#endif

#define Py_T_INT8 Py_T_BYTE
#define Py_T_UINT8 Py_T_UBYTE

#if SHRT_MAX == INT16_MAX
#define Py_T_INT16 Py_T_SHORT
#define Py_T_UINT16 Py_T_USHORT
#else
#error "No suitable 16-bit integer type found"
#endif

static_assert(sizeof(int32_t) == 4, "int32_t must be exactly 4 bytes");
static_assert(sizeof(int64_t) == 8, "int64_t must be exactly 8 bytes");
static_assert(sizeof(int16_t) == 2, "int16_t must be exactly 2 bytes");
static_assert(sizeof(int8_t) == 1, "int8_t must be exactly 1 byte");

#endif /* end SIZEMAC_H */
