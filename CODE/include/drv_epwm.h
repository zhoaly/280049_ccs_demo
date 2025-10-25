/**
 * @file drv_epwm.h
 * @brief ePWM 驱动接口定义，提供互补 PWM 的初始化与参数配置 API。
 */

#ifndef DRV_EPWM_H
#define DRV_EPWM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_EPWM_CHANNEL_COUNT    (3U) /**< ePWM 互补通道数量。 */

/**
 * @brief ePWM 运行状态信息。
 */
typedef struct
{
    /**< 当前 PWM 频率，单位 Hz。 */
    uint32_t frequencyHz;
    /**< 各通道占空比（0.0~1.0）。 */
    float dutyCycle[DRV_EPWM_CHANNEL_COUNT];
    /**< 上升沿死区计数值。 */
    uint16_t risingEdgeDelayCount;
    /**< 下降沿死区计数值。 */
    uint16_t fallingEdgeDelayCount;
} DRV_EPWM_State;

/**
 * @brief 初始化 ePWM 模块。
 */
void DRV_EPWM_init(void);

/**
 * @brief 设置三对互补 PWM 的频率。
 *
 * @param[in] frequencyHz 目标频率，单位 Hz。
 *
 * @retval true  配置成功。
 * @retval false 参数非法。
 */
bool DRV_EPWM_setFrequency(uint32_t frequencyHz);

/**
 * @brief 设置指定通道的占空比。
 *
 * @param[in] channelIndex 通道索引（0~DRV_EPWM_CHANNEL_COUNT-1）。
 * @param[in] dutyCycle    目标占空比，范围 0.0~1.0。
 *
 * @retval true  配置成功。
 * @retval false 参数非法。
 */
bool DRV_EPWM_setDutyCycle(uint32_t channelIndex, float dutyCycle);

/**
 * @brief 配置互补输出的死区计数。
 *
 * @param[in] risingEdgeCount  上升沿死区计数。
 * @param[in] fallingEdgeCount 下降沿死区计数。
 */
void DRV_EPWM_setDeadbandCounts(uint16_t risingEdgeCount, uint16_t fallingEdgeCount);

/**
 * @brief 读取当前 ePWM 状态。
 *
 * @param[out] state 状态结构体指针。
 */
void DRV_EPWM_getState(DRV_EPWM_State *state);

#ifdef __cplusplus
}
#endif

#endif /* DRV_EPWM_H */
