//#############################################################################
//
// FILE:   pid.c
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

#include "pid.h"

//*****************************************************************************
//
// PID_getDerFilterParams 获取微分滤波器参数
//
//*****************************************************************************
void
PID_getDerFilterParams(PID_Handle handle, float32_t *b0, float32_t *b1,
                       float32_t *a1, float32_t *x1, float32_t *y1)
{
    PID_Obj *obj = (PID_Obj *)handle;

    *b0 = FILTER_FO_get_b0(obj->derFilterHandle);
    *b1 = FILTER_FO_get_b1(obj->derFilterHandle);
    *a1 = FILTER_FO_get_a1(obj->derFilterHandle);
    *x1 = FILTER_FO_get_x1(obj->derFilterHandle);
    *y1 = FILTER_FO_get_y1(obj->derFilterHandle);

    return;
} // PID_getDerFilterParams() 函数结束

//*****************************************************************************
//
// PID_init 初始化 PID 对象
//
//*****************************************************************************
PID_Handle
PID_init(void *pMemory, const size_t numBytes)
{
    PID_Handle handle;
    PID_Obj    *obj;

    //
    // 检查分配的内存是否满足对象所需大小
    //
    if(numBytes < sizeof(PID_Obj))
    {
        return((PID_Handle)NULL);
    }

    //
    // 赋值句柄
    //
    handle = (PID_Handle)pMemory;

    //
    // 获取对象指针
    //
    obj = (PID_Obj *)handle;

    //
    // 初始化微分滤波器对象
    //
    obj->derFilterHandle = FILTER_FO_init(&(obj->derFilter),
                                          sizeof(obj->derFilter));

    return(handle);
} // PID_init() 函数结束

//*****************************************************************************
//
// PID_setDerFilterParams 设置微分滤波器参数
//
//*****************************************************************************
void
PID_setDerFilterParams(PID_Handle handle, const float32_t b0, const float32_t b1,
                       const float32_t a1, const float32_t x1, const float32_t y1)
{
    PID_Obj *obj = (PID_Obj *)handle;

    FILTER_FO_setNumCoeffs(obj->derFilterHandle,b0,b1);
    FILTER_FO_setDenCoeffs(obj->derFilterHandle,a1);
    FILTER_FO_setInitialConditions(obj->derFilterHandle,x1,y1);

    return;
} // PID_setDerFilterParams() 函数结束

// 文件结束
