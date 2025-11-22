/**
 * @file app_FOC.h
 * @brief 提供磁场定向控制（FOC）应用层接口定义。
 *
 * 本头文件集中声明了 FOC 算法在应用层暴露的所有数据结构与函数接口，
 * 包括三相电压、电流的抽象表示，dq/αβ 坐标系之间的变换结果缓存，
 * 以及运行时所需的配置参数。调用者可通过这些接口完成电机控制系统的
 * 任务调度、矢量变换、电压输出等操作，所有注释均采用专业术语进行详尽
 * 说明，方便团队成员快速理解并维护该模块。
 */

#ifndef APP_FOC_H
#define APP_FOC_H

#include <stdint.h>
#include "driverlib.h"

#include <stdbool.h>

/**
 * @brief 近似值 π，用于角度换算。可根据精度需求在实现中替换为更高精度常量。
 */
#define PI      3.14159f

/**
 * @brief 2π 常量，常用于角度归一化与周期换算。
 */
#define TWO_PI  6.28318530718f

/**
 * @brief √3 常量，广泛用于 Clarke/逆 Clarke 变换及空间矢量调制。
 */
#define SQRT3   1.73205080757f

/**
 * @brief 1/√3 常量，配合 αβ 坐标变换使用。
 */
#define INV_SQRT3 0.57735026919f

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
    FOC_AlphaBeta         voltageAlphaBeta;  /**< dq->αβ 逆变换后的电压指令缓存。 */
    FOC_DQ                voltageDQ;         /**< dq 坐标系的电压指令。 */
    float                 electricalAngle;   /**< 当前电角度，单位 rad，需在调用变换函数前更新。 */
} FOC_Handle;

/**
 * @brief FOC 任务入口函数，适用于 FreeRTOS 任务创建。
 *
 * 该函数将自动完成句柄初始化、底层外设配置及循环调度。若启动参数
 * 为空，则默认使用模块内部的静态句柄。
 *
 * @param[in] pvParameters 任务创建时传入的句柄指针，若为 NULL 则使用内部静态句柄。
 */
void FOC_Task_Func(void *pvParameters);

/**
 * @brief 初始化 FOC 句柄并清空所有运行态缓存。
 *
 * 建议在系统上电或复位后立即调用，确保句柄处于确定状态。
 *
 * @param[in,out] handle 需初始化的句柄指针，不能为空。
 */
void FOC_HandleInit(FOC_Handle *handle);

/**
 * @brief 执行零位校准,给d轴小电流将电机拉到零位,在主函数循环开始时调用
*
 * @param[in,out] handle FOC 句柄指针，需提供有效配置并接收初始化后的状态。
 */
void FOC_RunZeroCalibration(FOC_Handle *handle);

/**
 * @brief 三相位驱动的使能位,在初始化时使能
 */
static void FOC_DriverEnable();

/**
 * @brief 设置自定义配置。
 *
 * @param[in,out] handle FOC 句柄指针，不能为空。
 * @param[in]     config 待应用的配置结构体指针，需提前完成字段填充及校验。
 *
 * @retval true  配置通过合法性检查并被应用。
 * @retval false 参数指针为空或配置项超出允许范围。
 */
bool FOC_Configure(FOC_Handle *handle, const FOC_Config *config);

/**
 * @brief 获取一份安全的默认配置。
 *
 * @return 默认配置结构体，调用者可按需修改后重新写回句柄。
 */
FOC_Config FOC_GetDefaultConfig(void);

/**
 * @brief 设置电角度零点校准值。
 *
 * @param[in,out] handle    FOC 句柄指针，不能为空。
 * @param[in]     zeroAngle 校准值，单位 rad。
 *
 * @retval true  设置成功。
 * @retval false 参数非法（指针为空或角度非有限值）。
 */
bool FOC_SetZeroElectricAngle(FOC_Handle *handle, float zeroAngle);

/**
 * @brief 将三相电压指令转换为 PWM 占空比并写入驱动层。
 *
 * @param[in,out] handle FOC 句柄指针，需事先完成电压指令填充及供电电压配置。
 */
void FOC_SetPhaseVoltage(FOC_Handle *handle);

/**
 * @brief 执行逆 Clarke 变换，重建三相量。
 *
 * @param[in,out] handle FOC 句柄指针，将依据 αβ 电压指令生成三相电压。
 */
void FOC_InverseClarkeTransform(FOC_Handle *handle);

/**
 * @brief 执行逆 Park 变换，将 dq 量回到 αβ 坐标系。
 *
 * @param[in,out] handle FOC 句柄指针，需要提供 dq 电压指令及电角度。
 */
void FOC_InverseParkTransform(FOC_Handle *handle);

/**
 * @brief 获取最新的 αβ 电压矢量指令。
 *
 * @param[in] handle FOC 句柄指针，不能为空。
 *
 * @return 指向内部 αβ 电压缓存的常量指针，若句柄为空则返回 NULL。
 */
const FOC_AlphaBeta *FOC_GetAlphaBetaVoltage(const FOC_Handle *handle);

/**
 * @brief 归一化角度值，保证角度输入始终位于 [0, 2π)。
 */
float FOC_normalizeAngle(float angle);

/**
 * @brief 软限幅函数，确保输入值位于指定范围内。
 */
float FOC_clamp(float value, float minValue, float maxValue);

/**
 * @brief 获取最近一次写入的 dq 电压矢量。
 *
 * @param[in] handle FOC 句柄指针，不能为空。
 *
 * @return 指向内部 dq 电压缓存的常量指针，若句柄为空则返回 NULL。
 */
const FOC_DQ *FOC_GetDQVoltage(const FOC_Handle *handle);

/**
 * @brief 内部初始化例程，完成 PWM 配置等底层准备工作。
 *
 * @param[in,out] handle FOC 句柄指针，需提供有效配置并接收初始化后的状态。
 *
 * @retval true  初始化成功。
 * @retval false 参数非法或底层外设配置失败。
 */
static bool FOC_init(FOC_Handle *handle);



#endif /* APP_FOC_H */
