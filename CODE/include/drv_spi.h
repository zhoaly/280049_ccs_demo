/**
 * @file drv_spi.h
 * @brief SPI 驱动接口定义，负责 SPI 外设的初始化与状态查询。
 */

#ifndef DRV_SPI_H
#define DRV_SPI_H

#include <stdint.h>
#include <stdbool.h>

#ifndef DRV8316S_H
#include "drv8316s.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 无效 GPIO 标识。
 *
 * 当上层未提供片选或使能脚时，可使用该宏显式声明参数无效，
 * 驱动内部会据此跳过对应的 IO 配置与控制逻辑。
 */
#define DRV_SPI_INVALID_GPIO        (0xFFFFFFFFUL)

/**
 * @brief SPI 驱动运行状态。
 */
typedef struct
{
    uint32_t base;          /**< SPI 模块基地址，由硬件抽象层提供，例如 ::SPIA_BASE。 */
    uint32_t bitRate;       /**< SPI 传输速率（Hz），直接映射到 SPI 波特率寄存器。 */
    uint16_t dataWidth;     /**< SPI 数据位宽，取值范围 1~16 bit，对应 DRV8316 寄存器宽度。 */
    uint32_t csGpio;        /**< 软件片选 GPIO 编号，使用 Device 层宏定义的逻辑引脚编号。 */
    uint32_t enableGpio;    /**< 使能信号 GPIO 编号，用于控制驱动器 EN 引脚。 */
    bool     initialized;   /**< SPI 是否已完成初始化，避免多次重复配置。 */
} DRV_SPI_State;

/**
 * @brief 初始化 SPI 外设并配置基础通信参数。
 *
 * 该接口需在 FreeRTOS 调度启动前调用，确保 SPI 通道处于稳定状态。
 */
void DRV_SPI_init(void);

/**
 * @brief 设置软件片选 GPIO。
 *
 * 适用于板级片选通过普通 GPIO 实现的场景，驱动将自动配置方向、
 * 电气特性并在初始化后保持默认高电平。
 *
 * @param[in] gpio GPIO 引脚编号；若无需片选输出，则传入 ::DRV_SPI_INVALID_GPIO。
 */
void DRV_SPI_setChipSelectGPIO(uint32_t gpio);

/**
 * @brief 设置驱动器使能 GPIO。
 *
 * 配合 DRV8316 使能脚（EN）使用，默认输出高电平保持关断，
 * 上层可根据初始化流程择机下拉使能。
 *
 * @param[in] gpio GPIO 引脚编号；若未连接使能脚，则传入 ::DRV_SPI_INVALID_GPIO。
 */
void DRV_SPI_setEnableGPIO(uint32_t gpio);

/**
 * @brief 读取当前 SPI 状态。
 *
 * @param[out] state 状态结构体指针，非空时返回当前的运行参数快照。
 */
void DRV_SPI_getState(DRV_SPI_State *state);

/**
 * @brief 绑定 SPI 驱动到 DRV8316 对象，保持接口兼容性。
 *
 * 函数会自动完成 SPI 初始化、软件片选及 EN 引脚绑定，并将 SPI 基地址
 * 传递给 DRV8316 底层驱动，便于其执行寄存器访问。
 *
 * @param[in] handle    DRV8316 句柄，需由上层通过 DRV8316_init 获取。
 * @param[in] csGpio    片选 GPIO 编号，未连接时传入 ::DRV_SPI_INVALID_GPIO。
 * @param[in] enableGpio 使能 GPIO 编号，未连接时传入 ::DRV_SPI_INVALID_GPIO。
 */
void DRV_SPI_attachToDRV8316(DRV8316_Handle handle, uint32_t csGpio, uint32_t enableGpio);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SPI_H */
