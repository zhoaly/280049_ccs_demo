/**
 * @file drv_spi.h
 * @brief SPI driver interface.
 */

#ifndef DRV_SPI_H
#define DRV_SPI_H

#include <stdint.h>
#include <stdbool.h>

#ifndef DRV_SPI_USE_SYSCFG
#define DRV_SPI_USE_SYSCFG   (1)
#endif

#ifndef DRV8316S_H
#include "drv8316s.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SPI0_RX_BUF_LEN   128U
#define SPI0_TX_BUF_LEN   128U
/* 会话式全双工响应缓冲区长度（默认与普通 RX 缓冲一致） */
#ifndef SPI0_SESSION_RX_BUF_LEN
#define SPI0_SESSION_RX_BUF_LEN   SPI0_RX_BUF_LEN
#endif

typedef struct
{
    volatile uint16_t *buffer;
    uint16_t length;
    volatile uint16_t head;
    volatile uint16_t tail;
} DRV_SPI_RingBuffer;

static inline uint16_t DRV_SPI_nextIndex(uint16_t idx, uint16_t len)
{
    ++idx;
    if (idx >= len)
    {
        idx = 0;
    }
    return idx;
}

#define DRV_SPI_INVALID_GPIO        (0xFFFFFFFFUL)

typedef struct
{
    uint32_t base;
    uint32_t bitRate;
    uint16_t dataWidth;
    uint32_t csGpio;
    uint32_t enableGpio;
    bool initialized;
} DRV_SPI_State;

void DRV_SPI_init(void);
void DRV_SPI_setChipSelectGPIO(uint32_t gpio);
void DRV_SPI_setEnableGPIO(uint32_t gpio);
void DRV_SPI_getState(DRV_SPI_State *state);
void DRV_SPI_attachToDRV8316(DRV8316_Handle handle, uint32_t csGpio, uint32_t enableGpio);

/* ========================================================================== */
/* 队列异步通信接口                                                           */
/* ========================================================================== */
/* 从普通 RX 队列读取数据（非会话模式使用） */
uint16_t DRV_SPI0_QueueReadWords(uint16_t *pBuf, uint16_t len);
/* 向普通 TX 队列写入数据（非会话模式使用） */
uint16_t DRV_SPI0_QueueWriteWords(const uint16_t *pData, uint16_t len);
/* 清空普通 RX 队列与硬件 FIFO */
void DRV_SPI0_QueueRxFlush(void);
/* 就绪线控制：从机拉高就绪线，并在接收 rxCount 个字后自动拉低（主机侧调用无效） */
void DRV_SPI0_ReadyLineArm(uint16_t rxCount);

/* ========================================================================== */
/* 会话式全双工通信接口                                                       */
/* ========================================================================== */
/* 启动一次“命令发送 + dummy 时钟 + 响应接收”的全双工会话
 * 返回值：成功时返回计划发送/接收的总字数（txLen + rxExpect），失败返回 0。
 */
uint16_t DRV_SPI0_SessionBegin(const uint16_t *pTx, uint16_t txLen, uint16_t rxExpect);
/* 读取会话式响应数据 */
uint16_t DRV_SPI0_SessionReadWords(uint16_t *pBuf, uint16_t len);
/* 查询会话是否仍在进行 */
uint16_t DRV_SPI0_SessionIsActive(void);

/* ========================================================================== */
/* 兼容旧接口（内部调用新接口，建议新代码使用上面的分离接口）                */
/* ========================================================================== */
uint16_t DRV_SPI0_RxReadWords(uint16_t *pBuf, uint16_t len);
uint16_t DRV_SPI0_TxWriteWords(const uint16_t *pData, uint16_t len);
void DRV_SPI0_RxFlush(void);
uint16_t DRV_SPI0_BeginSession(const uint16_t *pTx, uint16_t txLen, uint16_t rxExpect);

void spi_send_string(const char *str);

__interrupt void INT_mySPI0_RX_ISR(void);
__interrupt void INT_mySPI0_TX_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SPI_H */
