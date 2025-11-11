/**
 * @file app_FOC_OpenLoop.h
 * @brief 提供 FOC 速度环开环控制相关的数据结构与接口。
 *
 * 该模块封装了开环速度调节所需的状态变量（如机械角度、时间戳等），
 * 并对外提供初始化、参数设置以及主控制例程。调用者只需在循环内
 * 周期性执行运行接口，即可在电机传感器缺失或调试阶段实现稳定的
 * 速度输出指令。
 */

#ifndef APP_FOC_OPEN_LOOP_H
#define APP_FOC_OPEN_LOOP_H

#include <stdint.h>

#include "app_FOC.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FOC 开环速度控制的状态缓存。
 *
 * 该结构体负责记录电机的机械角度、极对数以及最近一次控制周期的
 * 时间戳等信息，用于支撑速度积分与电角度换算。所有字段均以浮点或
 * 标准整型存储，满足在 MCU 浮点单元上进行高效计算的需求。
 */
typedef struct
{
    float     shaftAngle;   /**< 当前机械角度，单位 rad，采用数学正方向。 */
    uint32_t  polePairs;    /**< 电机极对数，取值需大于等于 1，用于机械角度映射至电角度。 */
    uint32_t  timestampUs;  /**< 上一次调用时的时间戳，单位 μs，用于计算离散积分步长。 */
    float     voltageLimit; /**< 期望的 q 轴电压幅值上限，单位 V，允许在运行期动态调整。 */
} FOC_OpenLoopState;

/**
 * @brief 初始化开环速度控制状态。
 *
 * 该函数在调用时会同步刷新时间戳，以确保第一次进入控制循环时能够
 * 获得正确的积分步长。默认电压限幅置零，代表后续自动从供电电压推导。
 *
 * @param[in,out] state     状态缓存指针，不能为空。
 * @param[in]     polePairs 电机极对数，若为 0 将自动回退为 1。
 */
void FOC_OpenLoop_Init(FOC_OpenLoopState *state, uint32_t polePairs);

/**
 * @brief 设置开环运行时的机械角度。
 *
 * 该接口可在调试阶段手动对齐电机机械角度，便于在无传感器条件下复现
 * 特定位置。角度值会立即归一化，避免后续积分导致数值溢出。
 *
 * @param[in,out] state       状态缓存指针，不能为空。
 * @param[in]     shaftAngle  机械角度，单位 rad，将被归一化至 [0, 2π)。
 */
void FOC_OpenLoop_SetShaftAngle(FOC_OpenLoopState *state, float shaftAngle);

/**
 * @brief 设置开环运行时的电压幅值上限。
 *
 * 若调用方提供的限幅值非法（小于等于零或非有限数），该接口将清空限幅
 * 并在运行时退化为根据母线电压自动估算合理值的策略。
 *
 * @param[in,out] state        状态缓存指针，不能为空。
 * @param[in]     voltageLimit 期望的 q 轴电压幅值，若小于等于 0 则按需回退。
 */
void FOC_OpenLoop_SetVoltageLimit(FOC_OpenLoopState *state, float voltageLimit);

/**
 * @brief 以目标速度指令执行一次开环控制，并输出对应的电压矢量。
 *
 * 调用流程包括：依据 FreeRTOS 系统节拍计算离散时间步长、对机械角速度
 * 进行积分、通过极对数映射至电角度、计算 dq 电压指令并复用 FOC 既有
 * 接口写回三相电压。若目标速度或电压限幅为非法输入，函数将采取保护
 * 策略，确保输出电压矢量始终处于安全范围内。
 *
 * @param[in,out] state          开环控制状态指针，需提前完成初始化。
 * @param[in,out] handle         FOC 句柄指针，用于访问供电电压并写回电压指令。
 * @param[in]     targetVelocity 目标机械角速度，单位 rad/s。
 *
 * @return 输出的 q 轴电压幅值，单位 V。
 */
float FOC_OpenLoop_RunVelocity(FOC_OpenLoopState *state,
                               FOC_Handle *handle,
                               float targetVelocity);

#ifdef __cplusplus
}
#endif

#endif /* APP_FOC_OPEN_LOOP_H */

