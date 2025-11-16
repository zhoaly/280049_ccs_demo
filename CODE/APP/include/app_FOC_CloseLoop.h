/**
 * @file app_FOC_CloseLoop.h
 * @brief 基于 eQEP 的位置-电压 PI 闭环控制接口。
 */

#ifndef APP_FOC_CLOSE_LOOP_H
#define APP_FOC_CLOSE_LOOP_H

#include "app_FOC.h"
#include "drv_eqep.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 通用 PI 控制器参数与状态。
 */
typedef struct
{
    float kp;             /**< 比例增益。 */
    float ki;             /**< 积分增益。 */
    float integral;       /**< 当前积分量。 */
    float integralMin;    /**< 积分下限，用于抗风up。 */
    float integralMax;    /**< 积分上限，用于抗风up。 */
    float outputLimit;    /**< 输出幅值限制，绝对值生效。 */
} FOC_PIController;

/**
 * @brief FOC 位置-电压闭环控制状态。
 */
typedef struct
{
    FOC_PIController positionLoop;      /**< 位置环 PI 控制器。 */
    float            targetPositionRad; /**< 期望机械角度，单位 rad。 */
    float            voltageLimit;      /**< q 轴电压限幅，0 表示跟随母线电压。 */
    DRV_EQEP_State   measurement;       /**< 最近一次编码器采样缓存。 */
} FOC_CloseLoopState;

/**
 * @brief 初始化闭环状态并写入默认 PI 参数。
 */
void FOC_CloseLoop_Init(FOC_CloseLoopState *state);

/**
 * @brief 设置位置参考值。
 */
void FOC_CloseLoop_SetTargetPosition(FOC_CloseLoopState *state, float targetRad);

/**
 * @brief 设置 q 轴电压限幅。
 */
void FOC_CloseLoop_SetVoltageLimit(FOC_CloseLoopState *state, float voltageLimit);

/**
 * @brief 执行一次位置-电压 PI 闭环计算并更新 PWM 输出。
 *
 * @param[in,out] state               闭环状态，需先完成初始化。
 * @param[in,out] handle              FOC 句柄。
 * @param[in]     samplePeriodSeconds 本周期采样时间，单位 s。
 *
 * @return 本次计算得到的 q 轴电压指令。
 */
float FOC_CloseLoop_Run(FOC_CloseLoopState *state,
                        FOC_Handle *handle,
                        float samplePeriodSeconds);

/**
 * @brief 获取最近一次编码器测量结果。
 */
const DRV_EQEP_State *FOC_CloseLoop_GetLatestMeasurement(const FOC_CloseLoopState *state);

#ifdef __cplusplus
}
#endif

#endif /* APP_FOC_CLOSE_LOOP_H */
