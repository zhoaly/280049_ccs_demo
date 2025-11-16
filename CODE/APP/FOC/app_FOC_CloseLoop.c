/**
 * @file app_FOC_CloseLoop.c
 * @brief 力矩-位置 PI 闭环控制实现。
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

    if((controller == NULL) || (deltaTime <= 0.0f))
    {
        return 0.0f;
    }

    controller->integral += controller->ki * error * deltaTime;
    controller->integral = FOC_clamp(controller->integral,
                                     controller->integralMin,
                                     controller->integralMax);

    output = controller->kp * error + controller->integral;

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
    while(error > PI)
    {
        error -= TWO_PI;
    }

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
    state->positionLoop.outputLimit = 3.0f; // 力矩参考（电压占空）的最大幅值

    state->torqueLoop.kp = 0.8f;
    state->torqueLoop.ki = 200.0f;
    state->torqueLoop.integral = 0.0f;
    state->torqueLoop.integralMin = -5.0f;
    state->torqueLoop.integralMax = 5.0f;
    state->torqueLoop.outputLimit = 0.0f; // 由电压限幅统一裁剪

    state->targetPositionRad = 0.0f;
    state->torqueCommand     = 0.0f;
    state->torqueLimit       = 3.0f;
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

void FOC_CloseLoop_SetTorqueLimit(FOC_CloseLoopState *state, float torqueLimit)
{
    if(state == NULL)
    {
        return;
    }

    if((torqueLimit <= 0.0f) || !isfinite(torqueLimit))
    {
        return;
    }

    state->torqueLimit = torqueLimit;
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
    float torqueRef;
    float torqueError;
    float voltageCommand;
    float voltageLimit;
    float supplyVoltage;

    if((state == NULL) || (handle == NULL))
    {
        return 0.0f;
    }

    timeStep = (samplePeriodSeconds > 0.0f) ? samplePeriodSeconds : 1e-3f;

    DRV_EQEP_update(timeStep);
    DRV_EQEP_getState(&eqepState);
    state->measurement = eqepState;

    positionError = state->targetPositionRad - eqepState.mechanicalAngleRad;
    positionError = FOC_CloseLoop_wrapAngleError(positionError);

    torqueRef = FOC_CloseLoop_runPI(&state->positionLoop, positionError, timeStep);
    state->torqueCommand = FOC_clamp(torqueRef, -state->torqueLimit, state->torqueLimit);

    torqueError = state->torqueCommand - handle->voltageDQ.q;
    voltageCommand = FOC_CloseLoop_runPI(&state->torqueLoop, torqueError, timeStep);

    supplyVoltage = handle->config.voltagePowerSupply;
    if((state->voltageLimit > 0.0f) && isfinite(state->voltageLimit))
    {
        voltageLimit = state->voltageLimit;
    }
    else if(supplyVoltage > 0.0f)
    {
        voltageLimit = supplyVoltage * 0.5f;
    }
    else
    {
        voltageLimit = 0.0f;
    }

    voltageCommand = FOC_clamp(voltageCommand, -voltageLimit, voltageLimit);

    handle->electricalAngle = eqepState.electricalAngleRad;
    handle->voltageDQ.d = 0.0f;
    handle->voltageDQ.q = voltageCommand;

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
