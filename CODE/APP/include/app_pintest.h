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
    X(0u,  GPIO_0_GPIO0) \
    X(1u,  GPIO_1_GPIO1) \
    X(4u,  GPIO_4_GPIO4) \
    X(7u,  GPIO_7_GPIO7) \
    X(12u, GPIO_12_GPIO12) \
    X(13u, GPIO_13_GPIO13) \
    X(18u, GPIO_18_GPIO18_X2) \
    X(22u, GPIO_22_GPIO22_VFBSW) \
    X(23u, GPIO_23_GPIO23_VSW) \
    X(24u, GPIO_24_GPIO24) \
    X(28u, GPIO_28_GPIO28) \
    X(29u, GPIO_29_GPIO29) \
    X(32u, GPIO_32_GPIO32) \
    X(33u, GPIO_33_GPIO33)
    /*已占用引脚:
        LED：GPIO6
        从机就绪：GPIO5
        SCI：GPIO16 / GPIO17 / GPIO2 / GPIO3
        SPI：GPIO8 / GPIO9 / GPIO10 / GPIO11
        JTAG: GPIO35/GPIO37
    */
#endif

/* ----------------------------- ADC 测试点列表 ----------------------------- */
/**
 * @brief ADC 测试点（pin-to-pin）列表。
 *
 /* ---------------------------- F280049 64-Pin ADC IO List ----------------------------
 * 条目格式：
 *   X(gpioPin, gpioPinConfig, adcBase, adcResultBase, adcChannel)
 *
 * 说明：
 * - 这里按 ADCA（ADCA_BASE/ADCARESULT_BASE）给出外部可用通道；
 * - gpioPin/gpioPinConfig 对于 AIO/AGPIO 使用 pin_map.h 中的 GPIO_<n>_GPIO<n>；
 * - ADCIN13 在 64-Pin PM 上对应 VREFLOABC（参考低端），通常不作为外部采样通道使用。
 */
#ifndef APP_PINTEST_ADC_POINT_LIST
#define APP_PINTEST_ADC_POINT_LIST \
    /* ADCINA0 : (A0/B15/C15/DACA_OUT)  -> AGPIO23, AIO231 */ \
    X(23u,  GPIO_23_GPIO23,    ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN0)  \
    /* ADCINA1 : (A1/B7/DACB_OUT)       -> AGPIO22, AIO232 */ \
    X(22u,  GPIO_22_GPIO22,    ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN1)  \
    /* ADCINA2 : (A2/B6/C9/PGA1_OF)     -> AIO224 */ \
    X(224u, GPIO_224_GPIO224,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN2)  \
    /* ADCINA3 : (A3/B3/C5/VDAC)        -> AIO242 */ \
    X(242u, GPIO_242_GPIO242,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN3)  \
    /* ADCINA4 : (A4/B8/C14/PGA2_OF)    -> AIO225 */ \
    X(225u, GPIO_225_GPIO225,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN4)  \
    /* ADCINA5 : (C2, A5/B12/C2)        -> AIO244*/  \
    X(244u, GPIO_244_GPIO244,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN5)  \
    /* 可选：ADCINA5 : (A5) -> AIO234（按硬件实际接线决定是否使用） */ \
    X(234u, GPIO_234_GPIO234,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN5)  \
    /* ADCINA6 : (A6/PGA5_OF)            -> AIO228 */ \
    X(228u, GPIO_228_GPIO228,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN6)  \
    /* ADCINA7 : (C3, A7/C3)             -> AIO245 */ \
    X(245u, GPIO_245_GPIO245,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN7)  \
    /* ADCINA8 : (PGA2_GND, A8/B0/C11)   -> AIO241 */ \
    X(241u, GPIO_241_GPIO241,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN8)  \
    /* ADCINA9 : (B4/C8/PGA4_OF)         -> AIO236 */ \
    X(236u, GPIO_236_GPIO236,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN9)  \
    /* ADCINA10: (A10/B1/C10/PGA7_OF)    -> AIO230 */ \
    X(230u, GPIO_230_GPIO230,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN10) \
    /* ADCINA11: (C0, A11/B10/C0)        -> AIO237 */ \
    X(237u, GPIO_237_GPIO237,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN11) \
    /* ADCINA12: (C1, A12/C1)            -> AIO238 */ \
    X(238u, GPIO_238_GPIO238,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN12) \
    /* ADCINA13: 64-Pin PM 上对应 VREFLOABC（参考低端），通常不列为外部采样 */ \
    /* ADCINA14: (C4, A14/B14/C4)        -> AIO239 */ \
    X(239u, GPIO_239_GPIO239,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN14) \
    /* ADCINA15: (PGA1_GND, A15/B9/C7)   -> AIO233 */ \
    X(233u, GPIO_233_GPIO233,  ADCA_BASE, ADCARESULT_BASE, ADC_CH_ADCIN15)
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
