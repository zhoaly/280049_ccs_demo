//#############################################################################
//
// FILE:   types.h
//
// TITLE:  C28x type definitions library
//
//#############################################################################
// $Copyright:
// Copyright (C) 2017-2025 Texas Instruments Incorporated - http://www.ti.com/
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

#ifndef TYPES_H
#define TYPES_H

//*****************************************************************************
//
// 若使用 C++ 编译器构建，请确保此头文件中的所有定义均采用 C 语言链接。
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
//! \defgroup TYPES TYPES
//! @{
//
//*****************************************************************************

#include "stdbool.h"
#include "stdint.h"

#if !defined(__TMS320C28XX_CLA__)
#include "string.h"
#endif

//*****************************************************************************
//
// 32-bit & 64-bit float type
//
//*****************************************************************************
#ifndef C2000_IEEE754_TYPES
#define C2000_IEEE754_TYPES
typedef float               float32_t;
typedef long double         float64_t;
#endif // C2000_IEEE754_TYPES

//*****************************************************************************
//
//! \brief 定义高电平
//
//*****************************************************************************
#ifndef HIGH
#define   HIGH          1
#endif

//*****************************************************************************
//
//! \brief 定义低电平
//
//*****************************************************************************
#define   LOW          0

//*****************************************************************************
//
//! \brief 定义关闭状态
//
//*****************************************************************************
#define   OFF           0

//*****************************************************************************
//
//! \brief 定义 OK 状态
//
//*****************************************************************************
#define   OK            0

//*****************************************************************************
//
//! \brief 定义开启状态
//
//*****************************************************************************
#define   ON            1

//*****************************************************************************
//
//! \brief 定义通用错误标志
//
//*****************************************************************************
#define   ERROR         1

//*****************************************************************************
//
//! \brief 定义通过标志
//
//*****************************************************************************
#define   PASS          1

//*****************************************************************************
//
//! \brief 定义失败标志
//
//*****************************************************************************
#define   FAIL          0

//*****************************************************************************
//
//! \brief 定义状态结果的可移植数据类型
//
//*****************************************************************************
typedef unsigned int status;

//*****************************************************************************
//
//! \brief 定义 32 位有符号浮点数据的可移植类型
//!        数据
//
//*****************************************************************************
typedef float float_t;

//*****************************************************************************
//
//! \brief 定义 64 位有符号浮点数据的可移植类型
//!        数据
//
//*****************************************************************************
//typedef long double double_t;
typedef double double_t;

#ifdef __TMS320C28XX_CLA__
#ifndef NULL
#define NULL 0
#endif

typedef unsigned int size_t;
#endif // __TMS320C28XX_CLA__

//*****************************************************************************
//
//! \brief 定义至少 8 位有符号实部与虚部的复数数据类型
//!        对应的虚部
//
//*****************************************************************************
typedef struct _cplx_int_least8_t
{
    int_least8_t imag;
    int_least8_t real;
} cplx_int_least8_t;

//*****************************************************************************
//
//! \brief 定义至少 8 位无符号实部与虚部的复数数据类型
//!        对应的虚部
//
//*****************************************************************************
typedef struct _cplx_uint_least8_t
{
    uint_least8_t imag;
    uint_least8_t real;
} cplx_uint_least8_t;

//*****************************************************************************
//
//! \brief 定义至少 16 位有符号实部与虚部的复数数据类型
//!        对应的虚部
//
//*****************************************************************************
typedef struct _cplx_least16_t
{
    int_least16_t imag;
    int_least16_t real;
} cplx_int_least16_t;

//*****************************************************************************
//
//! \brief 定义至少 16 位无符号实部与虚部的复数数据类型
//!        对应的虚部
//
//*****************************************************************************
typedef struct _cplx_uleast16_t
{
    uint_least16_t imag;
    uint_least16_t real;
} cplx_uint_least16_t;

//*****************************************************************************
//
//! \brief 定义至少 32 位有符号实部与虚部的复数数据类型
//!        对应的虚部
//
//*****************************************************************************
typedef struct _cplx_int_least32_t_
{
    int_least32_t imag;
    int_least32_t real;
} cplx_int_least32_t;

//*****************************************************************************
//
//! \brief 定义至少 32 位无符号实部与虚部的复数数据类型
//!        对应的虚部
//
//*****************************************************************************
typedef struct _cplx_uint_least32_t_
{
    uint_least32_t imag;
    uint_least32_t real;
} cplx_uint_least32_t;

//*****************************************************************************
//
//! \brief 定义 16 位有符号实部与虚部的复数数据类型
//!        components
//
//*****************************************************************************
typedef struct _cplx_int16_t_
{
    int16_t imag;
    int16_t real;
} cplx_int16_t;

//*****************************************************************************
//
//! \brief 定义 16 位无符号实部与虚部的复数数据类型
//!        components
//
//*****************************************************************************
typedef struct _cplx_uint16_t_
{
    uint16_t imag;
    uint16_t real;
} cplx_uint16_t;

//*****************************************************************************
//
//! \brief 定义 32 位有符号实部与虚部的复数数据类型
//!        components
//
//*****************************************************************************
typedef struct _cplx_int32_t
{
    int32_t imag;
    int32_t real;
} cplx_int32_t;

//*****************************************************************************
//
//! \brief 定义 32 位无符号实部与虚部的复数数据类型
//!        components
//
//*****************************************************************************
typedef struct _cplx_uint32_t
{
    uint32_t imag;
    uint32_t real;
} cplx_uint32_t;


//-----------------------------------------------------------------------------
//*****************************************************************************
//
//! \brief 定义浮点实部与虚部的复数数据类型
//!        components
//!
//
//*****************************************************************************
typedef struct _cplx_float_t
{
    float_t  real;
    float_t  imag;
} cplx_float_t;



//*****************************************************************************
//
// 关闭 Doxygen 分组。
//! @}
//
//*****************************************************************************

//*****************************************************************************
//
// 标记 C++ 编译器下 C 语言绑定区的结束。
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif  // TYPES_H
