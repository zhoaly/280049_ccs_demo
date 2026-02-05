/**
 * @file app_pintest.h
 * @brief APP 层引脚电平互连测试（Pin-to-Pin）模块接口。
 *
 * 功能概述：
 * - 主机通过 PROTO 发送“设置某引脚电平”的命令给从机；
 * - 从机将对应引脚拉高/拉低，并返回 ACK；
 * - 主机在收到 ACK 后读取本机同名引脚电平（GPIO 或 ADC），判断是否虚焊/断连。
 *
 * 使用约束：
 * - 主从引脚必须一一对应直连（如 GPIO5 ? GPIO5）。
 * - 已被 SPI/SCI/LED/EPWM/EQEP 等占用的引脚需从列表中剔除。
 * - AD 引脚需要在主机侧配置 ADC 通道映射（从机仍以 GPIO 方式拉高/拉低）。
 */
#ifndef APP_PINTEST_H
#define APP_PINTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_proto.h"
#include "driverlib.h"

/* ----------------------------- 角色/通道配置 ----------------------------- */
/**
 * @brief PROTO 通道号（主机发送/接收的通道）。
 */
#ifndef APP_PINTEST_CHANNEL
#define APP_PINTEST_CHANNEL APP_PROTO_CH1
#endif

/* ------------------------------- 时序相关参数 ------------------------------ */
/**
 * @brief 等待 ACK 的超时时间（ms）。
 */
#ifndef APP_PINTEST_ACK_TIMEOUT_MS
#define APP_PINTEST_ACK_TIMEOUT_MS 100u
#endif

/**
 * @brief 收到 ACK 后，主机等待电平稳定的延时（ms）。
 */
#ifndef APP_PINTEST_SETTLE_DELAY_MS
#define APP_PINTEST_SETTLE_DELAY_MS 2u
#endif

/**
 * @brief 每轮扫描全部测试点后的周期延时（ms）。
 */
#ifndef APP_PINTEST_PERIOD_MS
#define APP_PINTEST_PERIOD_MS 1000u
#endif

/**
 * @brief 是否在每个测试点完成后拉回低电平（1=是，0=否）。
 */
#ifndef APP_PINTEST_RESET_AFTER
#define APP_PINTEST_RESET_AFTER 1u
#endif

/**
 * @brief 主机侧 GPIO 输入是否启用内部上拉。
 * @note 若用于虚焊检测，建议关闭上拉以避免“开路被拉高”的误判。
 */
#ifndef APP_PINTEST_INPUT_PULLUP
#define APP_PINTEST_INPUT_PULLUP 0u
#endif

/* ----------------------------- 协议载荷定义 ------------------------------ */
/**
 * @brief Payload 格式：[CMD, PIN_ID, LEVEL]
 */
#define APP_PINTEST_CMD_SET_LEVEL  0x01u
#define APP_PINTEST_LEVEL_LOW      0u
#define APP_PINTEST_LEVEL_HIGH     1u
#define APP_PINTEST_PAYLOAD_LEN    3u

/* ---------------------------- GPIO 测试点列表 ---------------------------- */
/**
 * @brief GPIO 测试点（pin-to-pin）列表，使用 GPIO_n_GPIOn 引脚配置。
 *
 * 说明：
 * - 使用 X(pinId, pinConfig) 宏追加条目；
 * - pinId 为 GPIO 编号（与主/从板同名引脚对应）；
 * - pinConfig 为 GPIO_n_GPIOn；
 * - 请移除已占用引脚（SPI/SCI/LED/EPWM/EQEP 等）。
 */
#ifndef APP_PINTEST_GPIO_POINT_LIST
#define APP_PINTEST_GPIO_POINT_LIST \
    X(12u, GPIO_12_GPIO12) \
    X(14u, GPIO_14_GPIO14) \
    X(15u, GPIO_15_GPIO15) \
    X(18u, GPIO_18_GPIO18_X2) \
    X(20u, GPIO_20_GPIO20) \
    X(21u, GPIO_21_GPIO21) \
    X(22u, GPIO_22_GPIO22_VFBSW) \
    X(24u, GPIO_24_GPIO24) \
    X(25u, GPIO_25_GPIO25) \
    X(26u, GPIO_26_GPIO26) \
    X(27u, GPIO_27_GPIO27) \
    X(30u, GPIO_30_GPIO30) \
    X(31u, GPIO_31_GPIO31)
#endif

/* ----------------------------- ADC 测试点列表 ----------------------------- */
/**
 * @brief ADC 测试点（pin-to-pin）列表。
 *
 * 每个条目格式：
 *   X(gpioPin, gpioPinConfig, adcBase, adcResultBase, adcChannel)
 *
 * 示例：
 *   X(0u, GPIO_0_GPIO0, ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN0)
 *
 * 说明：
 * - 主机侧会将该引脚配置为模拟输入并采样 ADC；
 * - 从机侧仍以 GPIO 输出拉高/拉低；
 * - 若某 AD 引脚无法作为 GPIO 输出，则不适合该测试流程。
 */
#ifndef APP_PINTEST_ADC_POINT_LIST
#define APP_PINTEST_ADC_POINT_LIST
#endif

/* ----------------------------- ADC 采样参数 ----------------------------- */
/**
 * @brief ADC 采样保持时间（ACQPS）。
 */
#ifndef APP_PINTEST_ADC_ACQPS
#define APP_PINTEST_ADC_ACQPS 14u
#endif

/**
 * @brief ADC 判定高电平阈值（12bit 默认中点 2048）。
 */
#ifndef APP_PINTEST_ADC_HIGH_THRESHOLD
#define APP_PINTEST_ADC_HIGH_THRESHOLD 2048u
#endif

/* ------------------------------ ACK 同步对象 ------------------------------ */
/**
 * @brief 由 SysCfg 创建的 ACK 信号量，PROTO 回调中释放。
 */
extern SemaphoreHandle_t PROTO_ACK_CH0Handle;
extern SemaphoreHandle_t PROTO_ACK_CH1Handle;

/* ------------------------------- 对外接口 -------------------------------- */
/**
 * @brief 初始化 PINTEST 模块（GPIO/ADC 配置 + 必要资源准备）。
 */
void APP_PINTEST_Init(void);

/**
 * @brief PINTEST 任务函数（主机侧逐点测试/从机侧保持就绪）。
 */
void APP_PINTEST_Task_Func(void *pvParameters);

/**
 * @brief 从机侧处理 PROTO WRITE 帧。
 * @return 1 表示该命令已被 PINTEST 处理。
 */
uint16_t APP_PINTEST_OnProtoWrite(uint16_t channel,
                                  const uint16_t *payload,
                                  uint16_t len);

/**
 * @brief 主机侧处理 ACK 帧（释放对应 ACK 信号量）。
 */
void APP_PINTEST_OnProtoAck(uint16_t channel);

#ifdef __cplusplus
}
#endif

#endif /* APP_PINTEST_H */
