/**
 * @file app_drv8316.h
 * @brief DRV8316 应用层管理模块接口说明。
 *
 * 该模块对底层 DRV8316 驱动库与 SPI 适配层进行封装，提供线程安全的寄存器配置、
 * 状态轮询与手动读取接口。通过 SysConfig 生成的 APP_DRV8316_TASK 任务可直接调
 * 用本接口，完成对驱动芯片的周期性维护。
 */

#ifndef APP_DRV8316_H
#define APP_DRV8316_H

#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"

#include "drv_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 默认的周期性刷新时间（毫秒）。
 */
#define APP_DRV8316_DEFAULT_REFRESH_MS     (10U)

/**
 * @brief DRV8316 应用层初始化配置。
 */
typedef struct
{
    uint32_t  csGpio;              /**< DRV8316 片选 GPIO，使用 @ref DRV_SPI_INVALID_GPIO 表示无效。 */
    uint32_t  enableGpio;          /**< DRV8316 使能 GPIO，使用 @ref DRV_SPI_INVALID_GPIO 表示无效。 */
    TickType_t refreshPeriodTicks; /**< 任务刷新周期（FreeRTOS 时钟节拍数）。 */
    bool      autoEnable;          /**< 初始化后是否自动拉低 EN 引脚使能驱动。 */
} APP_DRV8316_Config;

/**
 * @brief 初始化 DRV8316 应用层模块。
 *
 * 若 @p config 为空，将沿用模块默认设置：使用 SPI 默认片选、禁用自动使能并以
 * APP_DRV8316_DEFAULT_REFRESH_MS 作为刷新周期。该函数内部会创建互斥量并初始化
 * 底层驱动句柄，因此务必在任务调度或其他 API 调用之前执行。
 */
void APP_DRV8316_init(const APP_DRV8316_Config *config);

/**
 * @brief 查询模块是否已经完成初始化。
 *
 * @retval true  模块已经可用，可安全调用其余接口。
 * @retval false 模块尚未准备好，需要等待 APP_DRV8316_init 完成。
 */
bool APP_DRV8316_isReady(void);

/**
 * @brief 获取最新的 DRV8316 寄存器快照。
 *
 * @param[out] outVars 结果缓冲区，必须为有效指针。
 * @retval true  已将底层缓存拷贝至 @p outVars；
 * @retval false 模块未初始化或参数非法。
 */
bool APP_DRV8316_getStatusSnapshot(DRV8316_VARS_t *outVars);

/**
 * @brief 根据传入配置更新控制寄存器，并在后台写入。
 *
 * @param[in] ctrlRegs 待写入的控制寄存器集合，指针需有效。
 * @retval true  请求已缓存，任务将在下一刷新周期完成写入；
 * @retval false 参数无效或模块尚未初始化。
 */
bool APP_DRV8316_scheduleControlUpdate(const DRV8316_VARS_t *ctrlRegs);

/**
 * @brief 请求手动读取指定寄存器。
 *
 * @param[in] address DRV8316 寄存器地址，取值范围 0~31。
 * @retval true  请求已接受，读取结果可通过 APP_DRV8316_getManualReadResult 获取；
 * @retval false 当前忙碌或参数错误。
 */
bool APP_DRV8316_requestManualRead(uint16_t address);

/**
 * @brief 获取最近一次手动读取的结果。
 *
 * @param[out] data 指向接收数据的缓冲区。
 * @retval true  已成功返回最新手动读取数据；
 * @retval false 尚未完成手动读取或模块未初始化。
 */
bool APP_DRV8316_getManualReadResult(uint16_t *data);

/**
 * @brief 获取底层 DRV8316 句柄。
 *
 * 该接口主要用于诊断或特殊场景下直接访问底层驱动 API，常规业务无需调用。
 */
DRV8316_Handle APP_DRV8316_getHandle(void);

/**
 * @brief DRV8316 周期性处理任务。
 *
 * 建议在 SysConfig 生成的 APP_DRV8316_TASK 任务入口中直接调用，用于在后台维护
 * 寄存器镜像、处理手动读写请求以及更新驱动状态。
 */
void APP_DRV8316_TASK(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* APP_DRV8316_H */
