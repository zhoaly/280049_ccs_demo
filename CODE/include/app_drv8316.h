/**
 * @file app_drv8316.h
 * @brief DRV8316 应用层管理模块接口。
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
 * @param[in] config 若为 NULL 则使用默认配置。
 */
void APP_DRV8316_init(const APP_DRV8316_Config *config);

/**
 * @brief 查询模块是否已经完成初始化。
 */
bool APP_DRV8316_isReady(void);

/**
 * @brief 获取最新的 DRV8316 寄存器快照。
 *
 * @param[out] outVars 结果缓冲区，非空时返回寄存器内容。
 * @return 读取是否成功。
 */
bool APP_DRV8316_getStatusSnapshot(DRV8316_VARS_t *outVars);

/**
 * @brief 根据传入配置更新控制寄存器，并在后台写入。
 *
 * @param[in] ctrlRegs 需要写入的控制寄存器值。
 * @return 请求是否接受。
 */
bool APP_DRV8316_scheduleControlUpdate(const DRV8316_VARS_t *ctrlRegs);

/**
 * @brief 请求手动读取指定寄存器。
 *
 * @param[in] address DRV8316 寄存器地址，取值范围 0~31。
 * @return 请求是否接受。
 */
bool APP_DRV8316_requestManualRead(uint16_t address);

/**
 * @brief 获取最近一次手动读取的结果。
 *
 * @param[out] data 读取到的数据。
 * @return 若当前没有正在执行的手动读取则返回 true。
 */
bool APP_DRV8316_getManualReadResult(uint16_t *data);

/**
 * @brief 获取底层 DRV8316 句柄。
 */
DRV8316_Handle APP_DRV8316_getHandle(void);

/**
 * @brief DRV8316 周期性处理任务。
 */
void APP_DRV8316_TASK(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* APP_DRV8316_H */
