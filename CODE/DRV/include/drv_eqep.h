/**
 * @file drv_eqep.h
 * @brief eQEP 驱动接口定义，面向 MT6701 等磁编码器的底层适配。
 */

#ifndef DRV_EQEP_H
#define DRV_EQEP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief eQEP 初始化配置参数。
 */
typedef struct
{
    uint32_t maxPositionCount;           /**< 编码器一圈的计数值，MT6701 默认为 16384。 */
    uint32_t polePairs;                  /**< 电机极对数，用于计算电角度，为 0 表示不计算。 */
    uint32_t unitTimerPeriodMicroseconds;/**< Unit Timer 的采样周期，单位 us。 */
} DRV_EQEP_Config;

/**
 * @brief eQEP 状态信息。
 */
typedef struct
{
    uint32_t rawPositionCount;  /**< 当前 QPOSCNT 原始计数值。 */
    float mechanicalAngleRad;   /**< 机械角度（弧度）。 */
    float mechanicalAngleDeg;   /**< 机械角度（度）。 */
    float electricalAngleRad;   /**< 电角度（弧度），当极对数为 0 时返回 0。 */
    int16_t direction;          /**< 运动方向：1 表示正向，-1 表示反向。 */
} DRV_EQEP_State;

/**
 * @brief 初始化 eQEP 模块。
 *
 * @param[in] config 初始化参数，传入 NULL 时使用默认配置。
 */
void DRV_EQEP_init(const DRV_EQEP_Config *config);

/**
 * @brief 使能 eQEP 模块。
 */
void DRV_EQEP_enable(void);

/**
 * @brief 关闭 eQEP 模块。
 */
void DRV_EQEP_disable(void);

/**
 * @brief 读取当前编码器计数。
 *
 * @return QPOSCNT 当前值。
 */
uint32_t DRV_EQEP_getPositionCount(void);

/**
 * @brief 强制设置当前位置计数。
 *
 * @param[in] position 目标计数值。
 */
void DRV_EQEP_setPositionCount(uint32_t position);

/**
 * @brief 获取 eQEP 状态信息。
 *
 * @param[out] state 状态结构体指针。
 */
void DRV_EQEP_getState(DRV_EQEP_State *state);

#ifdef __cplusplus
}
#endif

#endif /* DRV_EQEP_H */
