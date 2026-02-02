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




uint16_t DRV_SPI0_RxReadWords(uint16_t *pBuf, uint16_t len);
uint16_t DRV_SPI0_TxWriteWords(const uint16_t *pData, uint16_t len);

void spi_send_string(const char *str);

__interrupt void INT_mySPI0_RX_ISR(void);
__interrupt void INT_mySPI0_TX_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SPI_H */
