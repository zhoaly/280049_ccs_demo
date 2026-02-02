/**
 * @file drv_sci.h
 * @brief SCI driver interface.
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

#define SCI0_RX_BUF_LEN   128U
#define SCI0_TX_BUF_LEN   128U

typedef struct
{
    volatile uint16_t *buffer;
    uint16_t length;
    volatile uint16_t head;
    volatile uint16_t tail;
} DRV_SCI_RingBuffer;

static inline uint16_t nextIndex(uint16_t idx, uint16_t len)
{
    ++idx;
    if (idx >= len)
    {
        idx = 0;
    }
    return idx;
}

typedef void (*DRV_SCI_Callback)(void);

typedef struct
{
    uint32_t base;
    uint32_t baudRate;
    bool initialized;
    DRV_SCI_Callback onRx;
    DRV_SCI_Callback onTx;
} DRV_SCI_State;

void DRV_SCI_init(void);
void DRV_SCI_registerCallbacks(DRV_SCI_Callback rx, DRV_SCI_Callback tx);
bool DRV_SCI_readChar(uint16_t *data);
bool DRV_SCI_writeChar(uint16_t data);
void DRV_SCI_getState(DRV_SCI_State *state);

uint16_t DRV_SCI0_RxReadBytes(uint16_t *pBuf, uint16_t len);
uint16_t DRV_SCI0_TxWriteBytes(const uint16_t *pData, uint16_t len);

__interrupt void INT_mySCI0_RX_ISR(void);
__interrupt void INT_mySCI0_TX_ISR(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_SCI_H */
