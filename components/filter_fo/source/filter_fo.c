//#############################################################################
//
// FILE:   filter_fo.c
//
// TITLE:  C28x InstaSPIN filter library, first-order
//
//#############################################################################
#ifdef __TMS320C28XX_CLA__
#pragma CODE_SECTION(FILTER_FO_getDenCoeffs,"Cla1Prog2");
#pragma CODE_SECTION(FILTER_FO_getInitialConditions,"Cla1Prog2");
#pragma CODE_SECTION(FILTER_FO_getNumCoeffs,"Cla1Prog2");
#pragma CODE_SECTION(FILTER_FO_init,"Cla1Prog2");
#pragma CODE_SECTION(FILTER_FO_setDenCoeffs,"Cla1Prog2");
#pragma CODE_SECTION(FILTER_FO_setInitialConditions,"Cla1Prog2");
#pragma CODE_SECTION(FILTER_FO_setNumCoeffs,"Cla1Prog2");
#endif // __TMS320C28XX_CLA__

#include "filter_fo.h"

//*****************************************************************************
//
// FILTER_FO_getDenCoeffs 获取分母系数
//
//*****************************************************************************
void
FILTER_FO_getDenCoeffs(FILTER_FO_Handle handle, float32_t *pa1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    *pa1 = obj->a1;

    return;
} // FILTER_FO_getDenCoeffs() 函数结束

//*****************************************************************************
//
// FILTER_FO_getInitialConditions 获取初始条件
//
//*****************************************************************************
void
FILTER_FO_getInitialConditions(FILTER_FO_Handle handle, float32_t *px1,
                               float32_t *py1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    *px1 = obj->x1;

    *py1 = obj->y1;

    return;
} // FILTER_FO_getInitialConditions() 函数结束

//*****************************************************************************
//
// FILTER_FO_getNumCoeffs 获取分子系数
//
//*****************************************************************************
void
FILTER_FO_getNumCoeffs(FILTER_FO_Handle handle, float32_t *pb0, float32_t *pb1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    *pb0 = obj->b0;
    *pb1 = obj->b1;

    return;
} // FILTER_FO_getNumCoeffs() 函数结束

//*****************************************************************************
//
// FILTER_FO_init 初始化对象
//
//*****************************************************************************
FILTER_FO_Handle FILTER_FO_init(void *pMemory,
                                const size_t numBytes)
{
    FILTER_FO_Handle handle;

    if((int16_t)numBytes < (int16_t)sizeof(FILTER_FO_Obj))
    {
        return((FILTER_FO_Handle)NULL);
    }

    //
    // 赋值句柄
    //
    handle = (FILTER_FO_Handle)pMemory;

    return(handle);
} // FILTER_FO_init() 函数结束

//*****************************************************************************
//
// FILTER_FO_setDenCoeffs 设置分母系数
//
//*****************************************************************************
void
FILTER_FO_setDenCoeffs(FILTER_FO_Handle handle, const float32_t a1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->a1 = a1;

    return;
} // FILTER_FO_setDenCoeffs() 函数结束

//*****************************************************************************
//
// FILTER_FO_setInitialConditions 设置初始条件
//
//*****************************************************************************
void
FILTER_FO_setInitialConditions(FILTER_FO_Handle handle, const float32_t x1,
                               const float32_t y1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->x1 = x1;

    obj->y1 = y1;

    return;
} // FILTER_FO_setInitialConditions() 函数结束

//*****************************************************************************
//
// FILTER_FO_setNumCoeffs 设置分子系数
//
//*****************************************************************************
void
FILTER_FO_setNumCoeffs(FILTER_FO_Handle handle, const float32_t b0,
                       const float32_t b1)
{
    FILTER_FO_Obj *obj = (FILTER_FO_Obj *)handle;

    obj->b0 = b0;
    obj->b1 = b1;

    return;
} // FILTER_FO_setNumCoeffs() 函数结束

// 文件结束
