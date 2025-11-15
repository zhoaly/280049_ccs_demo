/**
 * @file drv_eqep.h
 * @brief eQEP 驱动接口定义，为磁编码器读数提供统一 API。
 */

#ifndef DRV_EQEP_H
#define DRV_EQEP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief eQEP 运行状态与关键参数。
 */
typedef struct
{
    uint32_t countsPerRevolution;   /**< 编码器每圈脉冲数（CPR）。 */
    uint32_t polePairs;             /**< 电机极对数，用于计算电角度。 */
    uint32_t rawPosition;           /**< 原始位置计数值（QPOSCNT）。 */
    int32_t  deltaCounts;           /**< 与上一采样相比的位置增量。 */
    float    mechanicalAngleRad;    /**< 机械角度（弧度，0~2π）。 */
    float    electricalAngleRad;    /**< 电角度（弧度，0~2π）。 */
    float    mechanicalSpeedRps;    /**< 机械角速度（转每秒）。 */
} DRV_EQEP_State;

/**
 * @brief 初始化 eQEP 模块与相关 GPIO。
 */
void DRV_EQEP_init(void);

/**
 * @brief 设置编码器每圈脉冲数（CPR）。
 *
 * @param[in] counts CPR 数值，需大于 0。
 *
 * @retval true  设置成功并在已初始化时立即生效。
 * @retval false 参数非法。
 */
bool DRV_EQEP_setCountsPerRevolution(uint32_t counts);

/**
 * @brief 设置电机极对数。
 *
 * @param[in] polePairs 极对数，需大于 0。
 *
 * @retval true  设置成功。
 * @retval false 参数非法。
 */
bool DRV_EQEP_setPolePairs(uint32_t polePairs);

/**
 * @brief 将位置计数复位为 0。
 */
void DRV_EQEP_resetPosition(void);

/**
 * @brief 读取编码器并刷新角度/速度结果。
 *
 * @param[in] samplePeriodSeconds 采样周期（秒）。若传入 <=0，则速度输出为 0。
 */
void DRV_EQEP_update(float samplePeriodSeconds);

/**
 * @brief 获取 eQEP 当前状态。
 *
 * @param[out] state 状态结构体指针，不能为空。
 */
void DRV_EQEP_getState(DRV_EQEP_State *state);

/**
 * @brief 获取最新原始位置计数。
 */
uint32_t DRV_EQEP_getRawPosition(void);

/**
 * @brief 获取最新机械角度（弧度）。
 */
float DRV_EQEP_getMechanicalAngleRad(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_EQEP_H */
