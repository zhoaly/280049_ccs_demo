//#############################################################################
//
// FILE:   pid.h
//
// TITLE:  C28x InstaSPIN Proportional-Integral-Derivative (PID) controller
//         library (floating point)
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

#ifndef PID_H
#define PID_H

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
//! \defgroup PID PID
//! @{
//
//*****************************************************************************

#include "types.h"
#include "filter_fo.h"
#include "math.h"

//*****************************************************************************
//
//! \brief 定义 PID 控制器对象
//
//*****************************************************************************
typedef struct _PID_Obj_
{
    float32_t Kp;                       //!< PID 控制器的比例增益
    float32_t Ki;                       //!< PID 控制器的积分增益
    float32_t Kd;                       //!< PID 控制器的微分增益
    float32_t Ui;                       //!< PID 控制器积分器的初始值
    float32_t refValue;                 //!< 参考输入值
    float32_t fbackValue;               //!< 反馈输入值
    float32_t ffwdValue;                //!< 前馈输入值

    float32_t outMin;                   //!< PID 控制器允许的最小输出值
    float32_t outMax;                   //!< PID 控制器允许的最大输出值
    FILTER_FO_Handle derFilterHandle; //!< 微分滤波器句柄
    FILTER_FO_Obj derFilter;          //!< 微分滤波器对象
} PID_Obj;

//*****************************************************************************
//
//! \brief 定义 PID 控制器句柄
//
//*****************************************************************************
typedef struct _PID_Obj_ *PID_Handle;

//*****************************************************************************
//
//! \brief     获取微分滤波器参数
//!
//!            y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] b0      z^0 的分子滤波系数值
//!
//! \param[in] b1      z^(-1) 的分子滤波系数值
//!
//! \param[in] a1      z^(-1) 的分母滤波系数值
//!
//! \param[in] x1      采样时刻 n=-1 的输入值
//!
//! \param[in] y1      采样时刻 n=-1 的输出值
//!
//! \return    无
//
//*****************************************************************************
extern void
PID_getDerFilterParams(PID_Handle handle, float32_t *b0, float32_t *b1,
                       float32_t *a1, float32_t *x1, float32_t *y1);

//*****************************************************************************
//
//! \brief     获取 PID 控制器中的反馈值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器中的反馈值
//
//*****************************************************************************
static inline float32_t
PID_getFbackValue(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->fbackValue);
} // PID_getFbackValue() 函数结束

//*****************************************************************************
//
//! \brief     获取 PID 控制器中的前馈值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器中的前馈值
//
//*****************************************************************************
static inline float32_t
PID_getFfwdValue(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->ffwdValue);
} // PID_getFfwdValue() 函数结束

//*****************************************************************************
//
//! \brief      获取 PID 控制器的增益
//!
//! \param[in]  handle  PID 控制器句柄
//!
//! \param[out] pKp     指向比例增益的指针
//!
//! \param[out] pKi     指向积分增益的指针
//!
//! \param[out] pKd     指向微分增益的指针
//!
//! \return     无
//
//*****************************************************************************
static inline void
PID_getGains(PID_Handle handle, float32_t *pKp, float32_t *pKi, float32_t *pKd)
{
    PID_Obj *obj = (PID_Obj *)handle;

    *pKp = obj->Kp;
    *pKi = obj->Ki;
    *pKd = obj->Kd;

    return;
} // PID_getGains() 函数结束

//*****************************************************************************
//
//! \brief     获取 PID 控制器的微分增益
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器的微分增益
//
//*****************************************************************************
static inline float32_t
PID_getKd(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->Kd);
} // PID_getKd() 函数结束

//*****************************************************************************
//
//! \brief     Gets the integral gain in the PID controller
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器的积分增益
//
//*****************************************************************************
static inline float32_t
PID_getKi(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->Ki);
} // PID_getKi() 函数结束

//*****************************************************************************
//
//! \brief     获取 PID 控制器的比例增益
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器的比例增益
//
//*****************************************************************************
static inline float32_t
PID_getKp(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->Kp);
} // PID_getKp() 函数结束

//*****************************************************************************
//
//! \brief      获取 PID 控制器允许的最小与最大输出值
//!
//! \param[in]  handle     PID 控制器句柄
//!
//! \param[out] pOutMin    指向允许的最小输出值的指针
//!
//! \param[out] pOutMax    指向允许的最大输出值的指针
//!
//! \return     无
//
//*****************************************************************************
static inline void
PID_getMinMax(PID_Handle handle, float32_t *pOutMin, float32_t *pOutMax)
{
    PID_Obj *obj = (PID_Obj *)handle;

    *pOutMin = obj->outMin;
    *pOutMax = obj->outMax;

    return;
} // PID_getMinMax() 函数结束

//*****************************************************************************
//
//! \brief      获取 PID 控制器允许的最大输出值
//!
//! \param[in]  handle  PID 控制器句柄
//!
//! \return     允许的最大输出值
//
//*****************************************************************************
static inline float32_t
PID_getOutMax(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->outMax);
} // PID_getOutMax() 函数结束

//*****************************************************************************
//
//! \brief      获取 PID 控制器允许的最小输出值
//!
//! \param[in]  handle  PID 控制器句柄
//!
//! \return     允许的最小输出值
//
//*****************************************************************************
static inline float32_t
PID_getOutMin(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->outMin);
} // PID_getOutMin() 函数结束

//*****************************************************************************
//
//! \brief     获取 PID 控制器中的参考值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器中的参考值
//
//*****************************************************************************
static inline float32_t
PID_getRefValue(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->refValue);
} // PID_getRefValue() 函数结束

//*****************************************************************************
//
//! \brief     获取 PID 控制器积分器的初始值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \return    PID 控制器积分器的初始值
//
//*****************************************************************************
static inline float32_t
PID_getUi(PID_Handle handle)
{
    PID_Obj *obj = (PID_Obj *)handle;

    return(obj->Ui);
} // PID_getUi() 函数结束

//*****************************************************************************
//
//! \brief     初始化 PID 控制器
//!
//! \param[in] pMemory   指向 PID 控制器对象内存的指针
//!
//! \param[in] numBytes  为 PID 控制器对象分配的字节数
//!
//! \return PID 控制器（PID）对象句柄
//
//*****************************************************************************
extern PID_Handle
PID_init(void *pMemory, const size_t numBytes);

//*****************************************************************************
//
//! \brief     设置微分滤波器参数
//!
//!            y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] b0      z^0 的分子滤波系数值
//!
//! \param[in] b1      z^(-1) 的分子滤波系数值
//!
//! \param[in] a1      z^(-1) 的分母滤波系数值
//!
//! \param[in] x1      采样时刻 n=-1 的输入值
//!
//! \param[in] y1      采样时刻 n=-1 的输出值
//!
//! \return    无
//
//*****************************************************************************
extern void
PID_setDerFilterParams(PID_Handle handle, const float32_t b0, const float32_t b1,
                       const float32_t a1, const float32_t x1, const float32_t y1);

//*****************************************************************************
//
//! \brief     设置 PID 控制器中的反馈值
//!
//! \param[in] handle      PID 控制器句柄
//!
//! \param[in] fbackValue  反馈值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setFbackValue(PID_Handle handle, const float32_t fbackValue)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->fbackValue = fbackValue;

    return;
} // PID_setFbackValue() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器中的前馈值
//!
//! \param[in] handle     PID 控制器句柄
//!
//! \param[in] ffwdValue  前馈值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setFfwdValue(PID_Handle handle, const float32_t ffwdValue)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->ffwdValue = ffwdValue;

    return;
} // PID_setFfwdValue() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器的增益
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] Kp      PID 控制器的比例增益
//!
//! \param[in] Ki      PID 控制器的积分增益
//!
//! \param[in] Kd      PID 控制器的微分增益
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setGains(PID_Handle handle, const float32_t Kp, const float32_t Ki,
             const float32_t Kd)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->Kp = Kp;
    obj->Ki = Ki;
    obj->Kd = Kd;

    return;
} // PID_setGains() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器的微分增益
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] Kd      PID 控制器的微分增益
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setKd(PID_Handle handle, const float32_t Kd)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->Kd = Kd;

    return;
} // PID_setKd() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器的积分增益
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] Ki      PID 控制器的积分增益
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setKi(PID_Handle handle, const float32_t Ki)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->Ki = Ki;

    return;
} // PID_setKi() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器的比例增益
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] Kp      PID 控制器的比例增益
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setKp(PID_Handle handle, const float32_t Kp)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->Kp = Kp;

    return;
} // PID_setKp() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器允许的最小与最大输出值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] outMin  允许的最小输出值
//!
//! \param[in] outMax  允许的最大输出值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setMinMax(PID_Handle handle, const float32_t outMin, const float32_t outMax)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->outMin = outMin;
    obj->outMax = outMax;

    return;
} // PID_setMinMax() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器允许的最大输出值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] outMax  允许的最大输出值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setOutMax(PID_Handle handle, const float32_t outMax)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->outMax = outMax;

    return;
} // PID_setOutMax() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器允许的最小输出值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] outMax  允许的最小输出值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setOutMin(PID_Handle handle, const float32_t outMin)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->outMin = outMin;

    return;
} // PID_setOutMin() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器中的参考值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] refValue   参考值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setRefValue(PID_Handle handle, const float32_t refValue)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->refValue = refValue;

    return;
} // PID_setRefValue() 函数结束

//*****************************************************************************
//
//! \brief     设置 PID 控制器积分器的初始值
//!
//! \param[in] handle  PID 控制器句柄
//!
//! \param[in] Ui      PID 控制器的积分初始值
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_setUi(PID_Handle handle, const float32_t Ui)
{
    PID_Obj *obj = (PID_Obj *)handle;

    obj->Ui = Ui;

    return;
} // PID_setUi() 函数结束

//*****************************************************************************
//
//! \brief     运行 PID 控制器的并联形式
//!
//! \param[in] handle      PID 控制器句柄
//!
//! \param[in] refValue    控制器的参考值
//!
//! \param[in] fbackValue  控制器的反馈值
//!
//! \param[in] ffwdValue   控制器的前馈值
//!
//! \param[in] pOutValue   指向控制器输出值的指针
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_run_parallel(PID_Handle handle, const float32_t refValue,
                 const float32_t fbackValue, const float32_t ffwdValue,
                 float32_t *pOutValue)
{
    PID_Obj *obj = (PID_Obj *)handle;

    float32_t Error;
    float32_t Kp = PID_getKp(handle);
    float32_t Ki = PID_getKi(handle);
    float32_t Kd = PID_getKd(handle);
    float32_t Up;
    float32_t Ui = PID_getUi(handle);
    float32_t Ud_tmp,Ud;
    float32_t outMax = PID_getOutMax(handle);
    float32_t outMin = PID_getOutMin(handle);

    Error = refValue - fbackValue;

    //
    // 计算比例输出
    //
    Up = Kp * Error;
    
    //
    // 计算积分输出
    //
    Ui = MATH_sat(Ui + (Ki * Error),outMax,outMin);

    Ud_tmp = Kd * Error;                               
    Ud = FILTER_FO_run(obj->derFilterHandle,Ud_tmp);

    //
    // 计算微分项
    //
    PID_setUi(handle,Ui);
    PID_setRefValue(handle,refValue);
    PID_setFbackValue(handle,fbackValue);
    PID_setFfwdValue(handle,ffwdValue);

    //
    // 限幅输出
    //
    *pOutValue = MATH_sat(Up + Ui + Ud + ffwdValue,outMax,outMin);

    return;
} // PID_run_parallel() 函数结束

//*****************************************************************************
//
//! \brief     运行 PID 控制器的串联形式
//!
//! \param[in] handle      PID 控制器句柄
//!
//! \param[in] refValue    控制器的参考值
//!
//! \param[in] fbackValue  控制器的反馈值
//!
//! \param[in] ffwdValue   控制器的前馈值
//!
//! \param[in] pOutValue   指向控制器输出值的指针
//!
//! \return    无
//
//*****************************************************************************
static inline void
PID_run_series(PID_Handle handle, const float32_t refValue,
               const float32_t fbackValue, const float32_t ffwdValue,
               float32_t *pOutValue)
{
    PID_Obj *obj = (PID_Obj *)handle;

    float32_t Error;
    float32_t Kp = PID_getKp(handle);
    float32_t Ki = PID_getKi(handle);
    float32_t Kd = PID_getKd(handle);
    float32_t Up;
    float32_t Ui = PID_getUi(handle);
    float32_t Ud_tmp,Ud;
    float32_t outMax = PID_getOutMax(handle);
    float32_t outMin = PID_getOutMin(handle);

    Error = refValue - fbackValue;

    //
    // 计算比例输出
    //
    Up = Kp * Error;
    
    //
    // 计算带限幅的积分输出
    //
    Ui = MATH_sat(Ui + (Ki * Up),outMax,outMin);

    //
    // 计算微分项
    //
    Ud_tmp = Kd * Ui;
    Ud = FILTER_FO_run(obj->derFilterHandle,Ud_tmp);

    PID_setUi(handle,Ui);
    PID_setRefValue(handle,refValue);
    PID_setFbackValue(handle,fbackValue);
    PID_setFfwdValue(handle,ffwdValue);

    //
    // 限幅输出
    //
    *pOutValue = MATH_sat(Up + Ui + Ud + ffwdValue,outMax,outMin);

    return;
} // PID_run_series() 函数结束

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

#endif // PID_H
