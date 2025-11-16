/**
 * @file app_FOC_CloseLoop.c
 * @brief 位置-电压 PI 闭环控制实现。
 */

#include "app_FOC_CloseLoop.h"

#include <math.h>

#include "drv_eqep.h"

/**
 * @brief 计算带限 PI 控制输出。
 */
static float FOC_CloseLoop_runPI(FOC_PIController *controller,
                                 float error,
                                 float deltaTime)
{
    float output;

    // PI 控制器不存在或时间步长无效时直接返回 0，避免后续计算出现异常。
    if((controller == NULL) || (deltaTime <= 0.0f))
    {
        return 0.0f;
    }

    // I 环积累：ki * 误差 * 采样周期，并在积分区间内钳位，防止积分饱和。
    controller->integral += controller->ki * error * deltaTime;
    controller->integral = FOC_clamp(controller->integral,
                                     controller->integralMin,
                                     controller->integralMax);

    // PI 输出：比例项 + 积分项。
    output = controller->kp * error + controller->integral;

    // 若设置了输出限幅，再对 PI 输出进行钳位，用于限制力矩或电压命令。
    if(controller->outputLimit > 0.0f)
    {
        output = FOC_clamp(output,
                           -controller->outputLimit,
                           controller->outputLimit);
    }

    return output;
}

/**
 * @brief 归一化角度误差至 [-pi, pi)。
 */
static float FOC_CloseLoop_wrapAngleError(float error)
{
    // 连续减去 2π，将误差限制在 [-π, π) 区间。
    while(error > PI)
    {
        error -= TWO_PI;
    }

    // 连续加上 2π，将误差限制在 [-π, π) 区间。
    while(error < -PI)
    {
        error += TWO_PI;
    }

    return error;
}

void FOC_CloseLoop_Init(FOC_CloseLoopState *state)
{
    if(state == NULL)
    {
        return;
    }

    state->positionLoop.kp = 4.0f;
    state->positionLoop.ki = 80.0f;
    state->positionLoop.integral = 0.0f;
    state->positionLoop.integralMin = -5.0f;
    state->positionLoop.integralMax = 5.0f;
    // 输出直接对应 q 轴电压，由统一电压限幅裁剪，因此禁用控制器自身限幅。
    state->positionLoop.outputLimit = 0.0f;

    state->targetPositionRad = 0.0f;
    state->voltageLimit      = 0.0f;

    state->measurement.countsPerRevolution = 0U;
    state->measurement.polePairs           = 0U;
    state->measurement.rawPosition         = 0U;
    state->measurement.deltaCounts         = 0;
    state->measurement.mechanicalAngleRad  = 0.0f;
    state->measurement.electricalAngleRad  = 0.0f;
    state->measurement.mechanicalSpeedRps  = 0.0f;
}

void FOC_CloseLoop_SetTargetPosition(FOC_CloseLoopState *state, float targetRad)
{
    if(state == NULL)
    {
        return;
    }

    if(!isfinite(targetRad))
    {
        return;
    }

    state->targetPositionRad = FOC_normalizeAngle(targetRad);
}

void FOC_CloseLoop_SetVoltageLimit(FOC_CloseLoopState *state, float voltageLimit)
{
    if(state == NULL)
    {
        return;
    }

    if(voltageLimit <= 0.0f)
    {
        state->voltageLimit = 0.0f;
        return;
    }

    state->voltageLimit = voltageLimit;
}

float FOC_CloseLoop_Run(FOC_CloseLoopState *state,
                        FOC_Handle *handle,
                        float samplePeriodSeconds)
{
    DRV_EQEP_State eqepState;
    float timeStep;
    float positionError;
    float voltageCommand;
    float voltageLimit;
    float supplyVoltage;

    if((state == NULL) || (handle == NULL))
    {
        return 0.0f;
    }

    // 若未给定有效采样时间，则默认采用 1ms 作为时间步长。
    timeStep = (samplePeriodSeconds > 0.0f) ? samplePeriodSeconds : 1e-3f;

    // 更新编码器测量并保存当前机械/电气角度与速度。
    DRV_EQEP_update(timeStep);
    DRV_EQEP_getState(&eqepState);
    state->measurement = eqepState;

    // 计算目标角与实际机械角之间的误差，并归一化到 [-π, π) 以消除整圈偏差。
    positionError = state->targetPositionRad - eqepState.mechanicalAngleRad;
    positionError = FOC_CloseLoop_wrapAngleError(positionError);

    // 位置 PI -> 直接输出 q 轴电压参考。
    voltageCommand = FOC_CloseLoop_runPI(&state->positionLoop, positionError, timeStep);

    supplyVoltage = handle->config.voltagePowerSupply;
    if((state->voltageLimit > 0.0f) && isfinite(state->voltageLimit))
    {
        // 使用外部设置的电压上限。
        voltageLimit = state->voltageLimit;
    }
    else if(supplyVoltage > 0.0f)
    {
        // 若未显式设置，则默认使用母线电压的一半以留出调制裕度。
        voltageLimit = supplyVoltage * 0.5f;
    }
    else
    {
        voltageLimit = 0.0f;
    }

    // 对位置 PI 输出的电压命令进行限幅，防止调制超出允许范围。
    voltageCommand = FOC_clamp(voltageCommand, -voltageLimit, voltageLimit);

    // 更新 FOC 句柄中的当前电角度以及 dq 轴电压，用于后续逆变换。
    handle->electricalAngle = eqepState.electricalAngleRad;
    handle->voltageDQ.d = 0.0f;
    handle->voltageDQ.q = voltageCommand;

    // 将 dq 电压逆变换为三相电压，并写入底层驱动（SVPWM/PWM）。
    FOC_InverseParkTransform(handle);
    FOC_InverseClarkeTransform(handle);
    FOC_SetPhaseVoltage(handle);

    return voltageCommand;
}

const DRV_EQEP_State *FOC_CloseLoop_GetLatestMeasurement(const FOC_CloseLoopState *state)
{
    if(state == NULL)
    {
        return NULL;
    }

    return &state->measurement;
}
