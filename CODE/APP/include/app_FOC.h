/**
 * @file app_FOC.h
 * @brief FOC控制逻辑实现
 */

#ifndef APP_FOC_H
#define APP_FOC_H

#include <stdint.h>

#include <stdbool.h>

#define PI 3.14159f

/**
 * @brief 三相电压指令结构体，使用结构体封装以便于参数校验与扩展。
 */
typedef struct
{
    float Ua; /**< A 相电压指令，单位 V。 */
    float Ub; /**< B 相电压指令，单位 V。 */
    float Uc; /**< C 相电压指令，单位 V。 */
} FOC_PhaseVoltage;

/**
 * @brief 三相电流测量值结构体。
 */
typedef struct
{
    float Ia; /**< A 相电流，单位 A。 */
    float Ib; /**< B 相电流，单位 A。 */
    float Ic; /**< C 相电流，单位 A。 */
} FOC_ThreePhaseCurrent;

/**
 * @brief αβ 坐标系向量。
 */
typedef struct
{
    float alpha; /**< α 轴分量。 */
    float beta;  /**< β 轴分量。 */
} FOC_AlphaBeta;

/**
 * @brief dq 坐标系向量。
 */
typedef struct
{
    float d; /**< 直轴（d 轴）分量。 */
    float q; /**< 交轴（q 轴）分量。 */
} FOC_DQ;

/**
 * @brief FOC 全局配置参数，统一封装可变配置项。
 */
typedef struct
{
    float    defaultDuty;       /**< PWM 默认占空比，范围 0.0f~1.0f。 */
    uint16_t defaultDeadband;   /**< PWM 默认死区时间，单位 TBCLK 周期。 */
    uint32_t defaultFrequency;  /**< PWM 默认频率，单位 Hz。 */
    float    voltagePowerSupply;/**< 直流母线电压，用于占空比归一化。 */
    float    zeroElectricAngle; /**< 电角度零点校准值，单位 rad。 */
} FOC_Config;

/**
 * @brief FOC 句柄，整合所有运行态数据，作为函数之间的统一参数传递实体。
 */
typedef struct
{
    FOC_Config            config;            /**< 全局配置参数集合。 */
    FOC_PhaseVoltage      phaseVoltage;      /**< 最近一次三相电压指令。 */
    FOC_ThreePhaseCurrent phaseCurrent;      /**< 最近一次三相电流采样。 */
    FOC_AlphaBeta         voltageAlphaBeta;  /**< dq->αβ 逆变换后的电压指令缓存。 */
    FOC_AlphaBeta         currentAlphaBeta;  /**< αβ 坐标系的电流值。 */
    FOC_DQ                voltageDQ;         /**< dq 坐标系的电压指令。 */
    FOC_DQ                currentDQ;         /**< dq 坐标系的电流值。 */
    float                 electricalAngle;   /**< 当前电角度，单位 rad，需在调用变换函数前更新。 */
} FOC_Handle;

void FOC_Task_Func(void *pvParameters);
void FOC_HandleInit(FOC_Handle *handle);
bool FOC_Configure(FOC_Handle *handle, const FOC_Config *config);
FOC_Config FOC_GetDefaultConfig(void);
bool FOC_SetZeroElectricAngle(FOC_Handle *handle, float zeroAngle);
void FOC_SetPhaseVoltage(FOC_Handle *handle);
void FOC_ClarkeTransform(FOC_Handle *handle);
void FOC_ParkTransform(FOC_Handle *handle);
void FOC_InverseClarkeTransform(FOC_Handle *handle);
void FOC_InverseParkTransform(FOC_Handle *handle);
void FOC_SetAlphaBetaVoltage(FOC_Handle *handle);
const FOC_AlphaBeta *FOC_GetAlphaBetaVoltage(const FOC_Handle *handle);
const FOC_AlphaBeta *FOC_GetAlphaBetaCurrent(const FOC_Handle *handle);
const FOC_DQ *FOC_GetDQVoltage(const FOC_Handle *handle);
const FOC_DQ *FOC_GetDQCurrent(const FOC_Handle *handle);

#endif /* APP_FOC_H */
