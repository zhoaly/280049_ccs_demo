//#############################################################################
//
// FILE:   filter_fo.h
//
// TITLE:  C28x InstaSPIN filter library, first-order
//
//#############################################################################
#ifndef FILTER_FO_H
#define FILTER_FO_H

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
//! \defgroup FILTER_FO FILTER_FO
//! @{
//
//*****************************************************************************

#include "types.h"

//*****************************************************************************
//
//! \brief 定义一阶滤波器（FILTER_FO）对象
//
//*****************************************************************************
typedef struct _FILTER_FO_Obj_
{
    float32_t a1;       //!< z^(-1) 的分母滤波系数值
    float32_t b0;       //!< z^0 的分子滤波系数值
    float32_t b1;       //!< z^(-1) 的分子滤波系数值
    float32_t x1;       //!< 采样时刻 n=-1 的输入值
    float32_t y1;       //!< 采样时刻 n=-1 的输出值
} FILTER_FO_Obj;

//*****************************************************************************
//
//! \brief 定义一阶滤波器（FILTER_FO）句柄
//
//*****************************************************************************
typedef struct _FILTER_FO_Obj_ *FILTER_FO_Handle;

//*****************************************************************************
//
//! \brief     获取一阶滤波器的分母系数 a1
//!
//! \param[in] handle  滤波器句柄
//!
//! \return    z^(-1) 的滤波器系数值
//
//*****************************************************************************
static inline float32_t
FILTER_FO_get_a1(FILTER_FO_Handle handle)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    return(obj->a1);
} // FILTER_FO_get_a1() 函数结束

//*****************************************************************************
//
//! \brief     获取一阶滤波器的分子系数 b0
//!
//! \param[in] handle  滤波器句柄
//!
//! \return    z^0 的滤波器系数值
//
//*****************************************************************************
static inline float32_t
FILTER_FO_get_b0(FILTER_FO_Handle handle)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    return(obj->b0);
} // FILTER_FO_get_b0() 函数结束

//*****************************************************************************
//
//! \brief     获取一阶滤波器的分子系数 b1
//!
//! \param[in] handle  滤波器句柄
//!
//! \return    z^(-1) 的滤波器系数值
//
//*****************************************************************************
static inline float32_t
FILTER_FO_get_b1(FILTER_FO_Handle handle)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    return(obj->b1);
} // FILTER_FO_get_b1() 函数结束

//*****************************************************************************
//
//! \brief     获取一阶滤波器在采样时刻 n=-1 的输入值
//!
//! \param[in] handle  滤波器句柄
//!
//! \return    采样时刻 n=-1 的输入值
//
//*****************************************************************************
static inline float32_t
FILTER_FO_get_x1(FILTER_FO_Handle handle)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    return(obj->x1);
} // FILTER_FO_get_x1() 函数结束

//*****************************************************************************
//
//! \brief     获取一阶滤波器在采样时刻 n=-1 的输出值
//!
//! \param[in] handle  滤波器句柄
//!
//! \return    采样时刻 n=-1 的输出值
//
//*****************************************************************************
static inline float32_t
FILTER_FO_get_y1(FILTER_FO_Handle handle)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    return(obj->y1);
} // FILTER_FO_get_y1() 函数结束

//*****************************************************************************
//
//! \brief     获取一阶滤波器的分母系数
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] pa1     指向 z^(-1) 滤波器系数值的内存指针
//!
//! \return    无
//
//*****************************************************************************
extern void
FILTER_FO_getDenCoeffs(FILTER_FO_Handle handle, float32_t *pa1);

//*****************************************************************************
//
//! \brief     获取一阶滤波器的初始条件
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] px1     指向采样时刻 n=-1 输入值的内存指针
//!
//! \param[in] py1     指向采样时刻 n=-1 输出值的内存指针
//!
//! \return    无
//
//*****************************************************************************
extern void
FILTER_FO_getInitialConditions(FILTER_FO_Handle handle, float32_t *px1,
                               float32_t *py1);

//*****************************************************************************
//
//! \brief     获取一阶滤波器的分子系数
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] pb0     指向 z^0 滤波器系数值的内存指针
//!
//! \param[in] pb1     指向 z^(-1) 滤波器系数值的内存指针
//!
//! \return    无
//
//*****************************************************************************
extern void
FILTER_FO_getNumCoeffs(FILTER_FO_Handle handle, float32_t *pb0, float32_t *pb1);

//*****************************************************************************
//
//! \brief     初始化一阶滤波器
//!
//! \param[in] pMemory   指向一阶滤波器对象内存的指针
//!
//! \param[in] numBytes  为一阶滤波器对象分配的字节数
//!
//! \return    滤波器（FILTER）对象句柄
//
//*****************************************************************************
extern FILTER_FO_Handle
FILTER_FO_init(void *pMemory, const size_t numBytes);

//*****************************************************************************
//
//! \brief     运行如下形式的一阶滤波器
//!            y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
//!
//! \param[in] handle      滤波器句柄
//!
//! \param[in] inputValue  待滤波的输入值
//!
//! \return    滤波后的输出值
//
//*****************************************************************************
#ifdef __TMS320C28XX_CLA__
#pragma FUNC_ALWAYS_INLINE(FILTER_FO_run)
#endif // __TMS320C28XX_CLA__

static inline float32_t
FILTER_FO_run(FILTER_FO_Handle handle, const float32_t inputValue)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    float32_t a1 = obj->a1;
    float32_t b0 = obj->b0;
    float32_t b1 = obj->b1;
    float32_t x1 = obj->x1;
    float32_t y1 = obj->y1;
    float32_t y0 = 1.0;
    float32_t x0 = inputValue;

    //
    // 计算输出
    //
    y0 = (b0 * x0) + (b1 * x1) - (a1 * y1);
    // y0 = (b0 * inputValue) + (a1 * y1);

    //
    // 保存数值以供下次使用
    //
    obj->x1 = inputValue;
    obj->y1 = y0;

    return(y0);
} // FILTER_FO_run() 函数结束

//*****************************************************************************
//
//! \brief     运行如下形式的一阶滤波器
//!            y[n] = b0*x[n] - a1*y[n-1]
//!
//! \param[in] handle      滤波器句柄
//!
//! \param[in] inputValue  待滤波的输入值
//!
//! \return    滤波后的输出值
//
//*****************************************************************************
static inline float32_t
FILTER_FO_run_form_0(FILTER_FO_Handle handle, const float32_t inputValue)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    float32_t a1 = obj->a1;
    float32_t b0 = obj->b0;
    float32_t y1 = obj->y1;

    //
    // 计算输出
    //
    float32_t y0 = (b0 * inputValue) - (a1 * y1);

    //
    // 保存数值以供下次使用
    //
    obj->y1 = y0;

    return(y0);
} // FILTER_FO_run_form_0() 函数结束

//*****************************************************************************
//
//! \brief     设置一阶滤波器的分母系数 a1
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] a1      z^(-1) 的滤波器系数值
//!
//! \return    无
//
//*****************************************************************************
static inline void
FILTER_FO_set_a1(FILTER_FO_Handle handle, const float32_t a1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->a1 = a1;

    return;
} // FILTER_FO_set_a1() 函数结束

//*****************************************************************************
//
//! \brief     设置一阶滤波器的分子系数 b0
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] b0      z^0 的滤波器系数值
//!
//! \return    无
//
//*****************************************************************************
static inline void
FILTER_FO_set_b0(FILTER_FO_Handle handle, const float32_t b0)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->b0 = b0;

    return;
} // FILTER_FO_set_b0() 函数结束

//*****************************************************************************
//
//! \brief     设置一阶滤波器的分子系数 b1
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] b1      z^(-1) 的滤波器系数值
//!
//! \return    无
//
//*****************************************************************************
static inline void
FILTER_FO_set_b1(FILTER_FO_Handle handle, const float32_t b1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->b1 = b1;

    return;
} // FILTER_FO_set_b1() 函数结束

//*****************************************************************************
//
//! \brief     设置一阶滤波器在采样时刻 n=-1 的输入值
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] x1      采样时刻 n=-1 的输入值
//!
//! \return    无
//
//*****************************************************************************
static inline void
FILTER_FO_set_x1(FILTER_FO_Handle handle, const float32_t x1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->x1 = x1;

    return;
} // FILTER_FO_set_x1() 函数结束

//*****************************************************************************
//
//! \brief     设置一阶滤波器在采样时刻 n=-1 的输出值
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] y1      采样时刻 n=-1 的输出值
//!
//! \return    无
//
//*****************************************************************************
static inline void
FILTER_FO_set_y1(FILTER_FO_Handle handle, const float32_t y1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->y1 = y1;

    return;
} // FILTER_FO_set_y1() 函数结束

//*****************************************************************************
//
//! \brief     设置一阶滤波器的分母系数
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] a1      z^(-1) 的滤波器系数值
//!
//! \return    无
//
//*****************************************************************************
extern void
FILTER_FO_setDenCoeffs(FILTER_FO_Handle handle, const float32_t a1);

//*****************************************************************************
//
//! \brief     设置一阶滤波器的初始条件
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] x1      采样时刻 n=-1 的输入值
//!
//! \param[in] y1      采样时刻 n=-1 的输出值
//!
//! \return    无
//
//*****************************************************************************
extern void
FILTER_FO_setInitialConditions(FILTER_FO_Handle handle, const float32_t x1,
                               const float32_t y1);

//*****************************************************************************
//
//! \brief     设置一阶滤波器的分子系数
//!
//! \param[in] handle  滤波器句柄
//!
//! \param[in] b0      z^0 的滤波器系数值
//!
//! \param[in] b1      z^(-1) 的滤波器系数值
//!
//! \return    无
//
//*****************************************************************************
extern void
FILTER_FO_setNumCoeffs(FILTER_FO_Handle handle, const float32_t b0,
                       const float32_t b1);

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

#endif // FILTER_FO_H
