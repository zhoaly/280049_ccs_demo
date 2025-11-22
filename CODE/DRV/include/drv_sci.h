/**
 * @file drv_sci.h
 * @brief SCI 驱动接口，提供串口初始化、收发与中断回调注册能力。
 */

#ifndef DRV_SCI_H
#define DRV_SCI_H

#include <stdbool.h>
#include <stdint.h>

#define DRV_SCI_USE_SYSCFG   (1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SCI 中断回调函数指针类型。
 */
typedef void (*DRV_SCI_Callback)(void);

/**
 * @brief SCI 运行时状态。
 */
typedef struct
{
    uint32_t base;           /**< SCI 外设基地址。*/
    uint32_t baudRate;       /**< 当前波特率。*/
    bool     initialized;    /**< 初始化标记。*/
    DRV_SCI_Callback onRx;   /**< RX 中断回调。*/
    DRV_SCI_Callback onTx;   /**< TX 中断回调。*/
} DRV_SCI_State;

/**
 * @brief 初始化 SCI 模块。若已初始化则直接返回。
 */
void DRV_SCI_init(void);

/**
 * @brief 注册 RX/TX 中断回调函数。
 *
 * @param[in] rx RX 中断回调，可为空。
 * @param[in] tx TX 中断回调，可为空。
 */
void DRV_SCI_registerCallbacks(DRV_SCI_Callback rx, DRV_SCI_Callback tx);

/**
 * @brief 尝试读取一个字节。
 *
 * @param[out] data 读取到的字节指针，不能为空。
 * @retval true  读取成功。
 * @retval false 无数据或未初始化。
 */
bool DRV_SCI_readChar(uint16_t *data);

/**
 * @brief 向 SCI 发送一个字节（非阻塞）。
 *
 * @param[in] data 要发送的字节。
 * @retval true  写入成功。
 * @retval false FIFO 满或未初始化。
 */
bool DRV_SCI_writeChar(uint16_t data);

/**
 * @brief 获取当前 SCI 状态快照。
 *
 * @param[out] state 状态输出指针，不能为空。
 */
void DRV_SCI_getState(DRV_SCI_State *state);

/**
 * @brief SCI RX 中断服务函数，由 SysCfg 配置引用。
 */
__interrupt void INT_mySCI0_RX_ISR(void);

/**
 * @brief SCI TX 中断服务函数，由 SysCfg 配置引用。
 */
__interrupt void INT_mySCI0_TX_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SCI_H */
