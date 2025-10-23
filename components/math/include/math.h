//#############################################################################
//
// FILE:   math.h
//
// TITLE:  C28x math library (floating point)
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

#ifndef TI_MATH_H
#define TI_MATH_H

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
//! \defgroup MATH MATH
//! @{
//
//*****************************************************************************

#ifndef __TMS320C28XX_CLA__
#include <math.h>
#endif

#include "libraries/utilities/types/include/types.h"

//*****************************************************************************
//
//! \brief 定义从 N*m 转换到 lb*in 的比例系数
//
//*****************************************************************************
#define MATH_Nm_TO_lbin_SF        ((float32_t)(8.8507457913f))

//*****************************************************************************
//
//! \brief 定义 2/3
//
//*****************************************************************************
#define MATH_TWO_OVER_THREE       ((float32_t)(0.6666666666666666666666666667f))

//*****************************************************************************
//
//! \brief 定义 1/3
//
//*****************************************************************************
#define MATH_ONE_OVER_THREE       ((float32_t)(0.3333333333333333333333333333f))

//*****************************************************************************
//
//! \brief 定义 1/π
//
//*****************************************************************************
#define MATH_ONE_OVER_PI          ((float32_t)(0.318309886183791f))


//*****************************************************************************
//
//! \brief 定义 √2
//
//*****************************************************************************
#define MATH_SQRT_TWO             ((float32_t)(1.414213562373095f))


//*****************************************************************************
//
//! \brief 定义 √3
//
//*****************************************************************************
#define MATH_SQRT_THREE           ((float32_t)(1.73205080756887f))

//*****************************************************************************
//
//! \brief 定义 1/√3
//
//*****************************************************************************
#define MATH_ONE_OVER_SQRT_THREE  ((float32_t)(0.5773502691896257645091487805f))

//*****************************************************************************
//
//! \brief 定义 1/(4π)
//
//*****************************************************************************
#define MATH_ONE_OVER_FOUR_PI     ((float32_t)(0.07957747154594767f))

//*****************************************************************************
//
//! \brief 定义 1/(2π)
//
//*****************************************************************************
#define MATH_ONE_OVER_TWO_PI      ((float32_t) (0.1591549430918954f))

//*****************************************************************************
//
//! \brief 定义 π
//
//*****************************************************************************
#define MATH_PI                   ((float32_t)(3.1415926535897932384626433832f))

//*****************************************************************************
//
//! \brief 定义 每单位 π
//
//*****************************************************************************
#define MATH_PI_PU                ((float32_t)(0.5f))

//*****************************************************************************
//
//! \brief 定义 2π
//
//*****************************************************************************
#define MATH_TWO_PI               ((float32_t)(6.283185307179586f))

//*****************************************************************************
//
//! \brief 定义 2每单位 π
//
//*****************************************************************************
#define MATH_TWO_PI_PU            ((float32_t)(1.0f))

//*****************************************************************************
//
//! \brief 定义 4π
//
//*****************************************************************************
#define MATH_FOUR_PI               ((float32_t)(12.56637061435917f))

//*****************************************************************************
//
//! \brief 定义 4每单位 π
//
//*****************************************************************************
#define MATH_FOUR_PI_PU            ((float32_t)(2.0f))

//*****************************************************************************
//
//! \brief 定义 π/2
//
//*****************************************************************************
#define MATH_PI_OVER_TWO           ((float32_t)(1.570796326794897f))

//*****************************************************************************
//
//! \brief 定义 每单位 π/2
//
//*****************************************************************************
#define MATH_PI_OVER_TWO_PU        ((float32_t)(0.25f))


//*****************************************************************************
//
//! \brief 定义 π/3
//
//*****************************************************************************
#define MATH_PI_OVER_THREE         ((float32_t)(1.047197551196598f))

//*****************************************************************************
//
//! \brief 定义 π/4
//
//*****************************************************************************
#define MATH_PI_OVER_FOUR          ((float32_t)(0.785398163397448f))

//*****************************************************************************
//
//! \brief 定义 每单位 π/4
//
//*****************************************************************************
#define MATH_PI_OVER_FOUR_PU        ((float32_t)(0.125f))

//*****************************************************************************
//
//! \brief 定义两个元素的向量
//
//*****************************************************************************
typedef struct _MATH_Vec2_
{
    float32_t value[2];
} MATH_Vec2;

typedef MATH_Vec2 MATH_vec2;

//*****************************************************************************
//
//! \brief 定义三个元素的向量
//
//*****************************************************************************
typedef struct _MATH_Vec3_
{
    float32_t value[3];
} MATH_Vec3;

typedef MATH_Vec3 MATH_vec3;

//*****************************************************************************
//
//! \brief     计算绝对值
//!
//! \param[in] in   输入值
//!
//! \return    绝对值
//
//*****************************************************************************
static inline
float32_t MATH_abs(const float32_t in)
{
    float32_t out = in;

    if(in < (float32_t)0.0f)
    {
        out = -in;
    }

    return(out);
} // MATH_abs() 函数结束


//*****************************************************************************
//
//! \brief     计算两个输入值中的最大值
//!
//! \param[in] in1  输入值 1
//!
//! \param[in] in2  输入值 2
//!
//! \return    最大值
//
//*****************************************************************************
static inline
float32_t MATH_max(const float32_t in1, const float32_t in2)
{
  float32_t out = in1;


  if(in1 < in2)
  {
    out = in2;
  }

  return(out);
} // MATH_max() 函数结束


//*****************************************************************************
//
//! \brief     计算两个输入值中的最小值
//!
//! \param[in] in1  输入值 1
//!
//! \param[in] in2  输入值 2
//!
//! \return    最小值
//
//*****************************************************************************
static inline
float32_t MATH_min(const float32_t in1, const float32_t in2)
{
  float32_t out = in1;


  if(in1 > in2)
    {
      out = in2;
    }

  return(out);
} // MATH_min() 函数结束

//*****************************************************************************
//
//! \brief     增加角度值并处理回绕
//!
//! \param[in] angle_rad       角度值，单位 rad
//!
//! \param[in] angleDelta_rad  角度增量，单位 rad
//!
//! \return    递增后的角度值，单位 rad
//
//*****************************************************************************
#ifdef __TMS320C28XX_CLA__
#pragma FUNC_ALWAYS_INLINE(MATH_incrAngle)
#endif

static inline float32_t
MATH_incrAngle(const float32_t angle_rad, const float32_t angleDelta_rad)
{
    float32_t angleNew_rad;

    //
    // 增加角度
    //
    angleNew_rad = angle_rad + angleDelta_rad;

    //
    // 检查边界
    //
//    if(angleNew_rad > MATH_PI)
//    {
//        angleNew_rad -= MATH_TWO_PI;
//    }
//    else if(angleNew_rad < (-MATH_PI))
//    {
//        angleNew_rad += MATH_TWO_PI;
//    }

    angleNew_rad = (angleNew_rad > MATH_PI)  ? angleNew_rad - MATH_TWO_PI : angleNew_rad;
    angleNew_rad = (angleNew_rad < -MATH_PI) ? angleNew_rad + MATH_TWO_PI : angleNew_rad;

    return(angleNew_rad);
} // MATH_incrAngle() 函数结束

//*****************************************************************************
//
//! \brief     在最小值和最大值之间对输入进行限幅
//!
//! \param[in] in   输入值
//!
//! \param[in] max  允许的最大值
//!
//! \param[in] min  允许的最小值
//!
//! \return    限幅结果
//
//*****************************************************************************
static inline float32_t
MATH_sat(const float32_t in, const float32_t max, const float32_t min)
{
    float32_t out = in;

//    if(in < min)
//    {
//        out = min;
//    }
//    else if(in > max)
//    {
//        out = max;
//    }

    out = (out > max) ? max : out;
    out = (out < min) ? min : out;

    return(out);
} // MATH_sat() 函数结束

//----------------------------------------------------------------------------
// 用于电机故障诊断
//-----------------------------------------------------------------------------
//*****************************************************************************
//
//! \brief 定义两个元素的向量
//
//*****************************************************************************
typedef struct _MATH_cplx_vec2_
{

    cplx_float_t  value[2];

} MATH_cplx_vec2;

//*****************************************************************************
//
//! \brief 定义三个元素的向量
//
//*****************************************************************************
typedef struct _MATH_cplx_vec3_
{

    cplx_float_t  value[3];

} MATH_cplx_vec3;

//*****************************************************************************
//
//! \brief 定义 6π
//
//*****************************************************************************
#define MATH_SIX_PI               ((float32_t)(18.8495559215f))

//*****************************************************************************
//
//! \brief 定义 √3/2
//
//*****************************************************************************
#define MATH_SQRTTHREE_OVER_TWO       ((float32_t)(0.8660254038f))


//*****************************************************************************
//
//! \brief     计算复数的模平方
//!
//! \param[in] in   输入复数
//!
//! \return    模平方结果
//
//*****************************************************************************
#ifdef __TMS320C28XX_CLA__
static inline
float32_t cAbsSq(const cplx_float_t* x1)
{
    float32_t y;

    y = 0.0f;

    return y;
}
#else
static inline
float32_t cAbsSq(const cplx_float_t* x1)
{
    float32_t y;

    #ifdef __TMS320C28XX_TMU__
    y = pow(x1->real,2) + pow(x1->imag,2);
    #else
    y = (float32_t)(pow((double_t)(x1->real),(double_t)(2.0)) +
            pow((double_t)(x1->imag),(double_t)(2.0)));
    #endif  // __TMS320C28XX_TMU__

    return y;
}
#endif // __TMS320C28XX_CLA__


static inline
float32_t MATH_sign(const float32_t in)
{
  float32_t out = 1.0f;


  if(in < 0.0f)
    {
      out = -1.0f;
    }

  return(out);
} // MATH_sign() 函数结束

static inline void
MATH_mult_cc(cplx_float_t* x1, cplx_float_t* x2, cplx_float_t * y)
{
    y->real = x1->real * x2->real - x1->imag * x2->imag;
    y->imag = x1->real * x2->imag + x1->imag * x2->real;
    return;
}

static inline void
MATH_add_cc(cplx_float_t* x1, cplx_float_t* x2, cplx_float_t * y)
{
    y->real = x1->real  +  x2->real;
    y->imag = x1->imag  +  x2->imag;
    return;
}

static inline void
MATH_sub_cc(cplx_float_t* x1, cplx_float_t* x2, cplx_float_t * y)
{
    y->real = x1->real - x2->real;
    y->imag = x1->imag - x2->imag;
    return;
}

static inline void
MATH_mult_rc(float32_t x1, cplx_float_t* x2, cplx_float_t * y)
{
    y->real = x1 * x2->real;
    y->imag = x1 * x2->imag;
    return;
}

static inline void
MATH_add_rc(float32_t x1, cplx_float_t* x2, cplx_float_t * y)
{
    y->real = x1  +  x2->real;
    y->imag = x2->imag;
    return;
}

static inline void
MATH_sub_rc(float32_t x1, cplx_float_t* x2, cplx_float_t* y)
{
    y->real = x1 - x2->real;
    y->imag = - x2->imag;
    return;
}

static inline void
MATH_sub_cr(cplx_float_t* x1, float32_t x2, cplx_float_t* y)
{
    y->real = x1->real - x2;
    y->imag = x1->imag;

    return;
}



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

#endif // TI_MATH_H
