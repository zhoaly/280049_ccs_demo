/**
 * @file app_FOC_OpenLoop.c
 * @brief 基于 FOC 的速度环开环控制实现。
 *
 * 该实现沿用应用层既有的编码风格，通过内部状态缓存实现机械角度积分、
 * 电角度换算与电压矢量生成。函数接口保持轻量化设计，便于在调试阶段
 * 快速复用，也方便未来扩展闭环控制所需的额外算法模块。
 */

#include "app_FOC_OpenLoop.h"

#include <math.h>

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief 获取当前系统时间的微秒级计数。
 *
 * 该实现基于 FreeRTOS 的节拍计数，假设调度器已启动。若调度器尚未
 * 运行，则返回值将保持 0。通过乘以 portTICK_PERIOD_MS 将节拍转换成
 * 毫秒，再进而转换为微秒，满足速度环开环积分对高分辨率时间的需求。
 */
static uint32_t FOC_OpenLoop_getTimeUs(void)
{
    const TickType_t tick = xTaskGetTickCount();
    const uint32_t usPerTick = (uint32_t)portTICK_PERIOD_MS * 1000U;
    return (uint32_t)tick * usPerTick;
}

void FOC_OpenLoop_Init(FOC_OpenLoopState *state, uint32_t polePairs)
{
    if(state == NULL)
    {
        return;
    }

    /*
     * 将机械角度初始化为 0 rad，并在初始化阶段直接读取当前系统时间，
     * 以确保后续进入运行函数时能够获得准确的离散采样周期。
     */
    state->shaftAngle   = 0.0f;
    state->polePairs    = (polePairs == 0U) ? 1U : polePairs;
    state->timestampUs  = FOC_OpenLoop_getTimeUs();
    state->voltageLimit = 0.0f;
}

void FOC_OpenLoop_SetShaftAngle(FOC_OpenLoopState *state, float shaftAngle)
{
    if(state == NULL)
    {
        return;
    }

    if(!isfinite(shaftAngle))
    {
        return;
    }

    /*
     * 归一化外部写入的机械角度，避免由于输入值越界导致内部积分累计
     * 出现精度损失，同时保证角度始终落在 [0, 2π) 区间。
     */
    state->shaftAngle = FOC_normalizeAngle(shaftAngle);
}

void FOC_OpenLoop_SetVoltageLimit(FOC_OpenLoopState *state, float voltageLimit)
{
    if(state == NULL)
    {
        return;
    }

    if((voltageLimit <= 0.0f) || !isfinite(voltageLimit))
    {
        state->voltageLimit = 0.0f;
        return;
    }

    state->voltageLimit = voltageLimit;
}

float FOC_OpenLoop_RunVelocity(FOC_OpenLoopState *state,
                               FOC_Handle *handle,
                               float targetVelocity)
{
    float supplyVoltage;
    float uqCommand;
    float timeStep;
    float electricalAngle;
    float polePairs;
    uint32_t nowUs;
    uint32_t deltaUs;

    if((state == NULL) || (handle == NULL))
    {
        return 0.0f;
    }

    /*
     * 通过当前时间与历史时间戳差值计算离散步长，转换至秒以便与速度指令
     * 相乘。若检测到时间戳回绕或异常抖动，则使用 1 ms 的默认步长兜底，
     * 防止积分发散。
     */
    nowUs = FOC_OpenLoop_getTimeUs();
    deltaUs = nowUs - state->timestampUs;
    timeStep = (float)deltaUs * 1e-6f;

    if((timeStep <= 0.0f) || (timeStep > 0.5f))
    {
        timeStep = 1e-3f;
    }

    /*
     * 将非法的速度指令（如 NaN、Inf）钳位为 0 rad/s，避免传感器故障或
     * 上层逻辑异常时对电机施加无意义的积分运算。
     */
    if(!isfinite(targetVelocity))
    {
        targetVelocity = 0.0f;
    }

    /*
     * 对目标速度进行积分以更新机械角度。若角速度存在抖动，该归一化
     * 操作可将角度保持在一周范围内，避免浮点数在长时间运行下累积漂移。
     */
    state->shaftAngle = FOC_normalizeAngle(state->shaftAngle + targetVelocity * timeStep);

    supplyVoltage = handle->config.voltagePowerSupply;

    if((state->voltageLimit > 0.0f) && isfinite(state->voltageLimit))
    {
        /*
         * 若外部已设定电压限幅，则直接使用该值作为 q 轴电压指令，便于
         * 在软硬件联调期间灵活调整输出强度。
         */
        uqCommand = state->voltageLimit;
    }
    else if(supplyVoltage > 0.0f)
    {
        /*
         * 在未提供限幅的情况下，默认使用母线电压的三分之一作为初始输出，
         * 兼顾起动转矩与调试安全性。
         */
        uqCommand = supplyVoltage / 3.0f;
    }
    else
    {
        /*
         * 若供电电压无效，则返回 0 V，防止对逆变器输出无意义的指令。
         */
        uqCommand = 0.0f;
    }

    if(supplyVoltage > 0.0f)
    {
        /*
         * 在供电有效的情况下，将指令限定在 0~母线电压一半的范围内，
         * 避免三相合成电压超过硬件能力导致饱和。
         */
        float maxAllowable = supplyVoltage * 0.5f;
        uqCommand = FOC_clamp(uqCommand, 0.0f, maxAllowable);
    }
    else
    {
        /*
         * 当供电参数未知时，最安全的做法是将电压指令钳位在 0 附近，
         * 防止出现负向饱和或异常值。
         */
        uqCommand = FOC_clamp(uqCommand, 0.0f, uqCommand);
    }

    /*
     * 根据极对数将机械角度映射为电角度，并叠加零位校准值。该电角度
     * 随后供逆 Park 变换与 SVPWM 等算法使用，必须在调用相关接口前更新。
     */
    polePairs = (state->polePairs == 0U) ? 1.0f : (float)state->polePairs;
    electricalAngle = FOC_normalizeAngle(state->shaftAngle * polePairs + handle->config.zeroElectricAngle);
    handle->electricalAngle = electricalAngle;

    handle->voltageDQ.d = 0.0f;
    handle->voltageDQ.q = uqCommand;

    /*
     * 复用 FOC 模块提供的 dq→αβ 变换接口，确保与闭环控制路径一致，
     * 也可共享零电角度等配置参数。
     */
    /*
     * 调用既有 dq→αβ 变换接口，将当前的 dq 电压矢量旋转至静止坐标系，
     * 以便后续进一步生成三相电压。
     */
    FOC_SetAlphaBetaVoltage(handle);

    /*
     * 根据逆 Clarke 变换关系推导三相电压，其中 Ua 直接对应 α 分量，
     * Ub/Uc 则需叠加 ±√3/2 的 β 分量，以确保三相矢量平衡。
     */
    handle->phaseVoltage.Ua = handle->voltageAlphaBeta.alpha;
    handle->phaseVoltage.Ub = (-handle->voltageAlphaBeta.alpha +
                               SQRT3 * handle->voltageAlphaBeta.beta) * 0.5f;
    handle->phaseVoltage.Uc = (-handle->voltageAlphaBeta.alpha -
                               SQRT3 * handle->voltageAlphaBeta.beta) * 0.5f;

    /*
     * 将三相电压指令写入底层驱动模块，确保逆变器输出与计算结果保持一致。
     */
    FOC_SetPhaseVoltage(handle);

    /*
     * 缓存当前时间戳，为下一次积分提供参考，确保积分周期的连续性。
     */
    state->timestampUs = nowUs;

    return uqCommand;
}

