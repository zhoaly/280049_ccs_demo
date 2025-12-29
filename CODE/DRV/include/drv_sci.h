/**
 * @file drv_sci.h
 * @brief SCI 驱动接口，提供串口初始化、收发与中断回调注册能力。
 * 当前版本代码里,为实现对SCI的原子化操作,应当在调用时,自行使用信号量等措施
 */

#ifndef DRV_SCI_H
#define DRV_SCI_H

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"


#define DRV_SCI_USE_SYSCFG   (1)

#ifdef __cplusplus
extern "C" {
#endif

//SCI邮箱(两个环形缓冲区)
#define SCI0_RX_BUF_LEN   128U
#define SCI0_TX_BUF_LEN   128U

/**
 * @brief SCI 环形缓冲区抽象。
 */
typedef struct
{
    volatile uint16_t *buffer; /**< 实际存储空间指针。*/
    uint16_t           length; /**< 缓冲区总长度。*/
    volatile uint16_t  head;   /**< 写指针。*/
    volatile uint16_t  tail;   /**< 读指针。*/
} DRV_SCI_RingBuffer;


/**
 * @brief SCI 缓冲区环形标准进位。
 * @param[in] idx 待进位序号。
 * @param[in] len buf总长度。
 */
static inline uint16_t nextIndex(uint16_t idx, uint16_t len)
{
    ++idx;
    if (idx >= len)
    {
        idx = 0;
    }
    return idx;
}


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
 * @brief 从 SCI0 环形缓冲区读取最多 len 个字节（非阻塞，低 8 位有效）。
 *
 * @param[out] pBuf  输出缓冲区指针（uint16_t 数组，每项低 8 位为有效字节）
 * @param[in]  len   期望读取的最大字节数
 *
 * @return 实际读取到的字节数（0 表示当前无数据）
 *
 * @note  该函数与 TX 写函数风格对齐：
 *        - for 循环按 len 尝试读取；
 *        - 每次循环快照 head/tail；
 *        - 缓冲区空（tail == head）则提前 break；
 *        - 函数返回实际读取的数量。
 */
uint16_t DRV_SCI0_RxReadBytes(uint16_t *pBuf, uint16_t len);

/**
 * @brief 向 SCI0 发送环形缓冲区写入数据（低 8 位为有效字节）。
 *
 * @param[in] pData  数据缓冲区指针（每个 Uint16 的低 8 位为一个字节）
 * @param[in] len    待写入的字节数
 *
 * @return 实际写入缓冲区的字节数（可能小于 len，表示缓冲区已满）
 *
 * @note
 * - 假设只有一个任务调用本函数写入（单生产者），中断为单消费者。
 * - 本函数为“非阻塞”写入，如果缓冲区满，会提前退出。
 * - 若实际写入长度 > 0，会打开 TX FIFO 中断，触发中断发送。
 */
uint16_t DRV_SCI0_TxWriteBytes(const uint16_t *pData, uint16_t len);

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
