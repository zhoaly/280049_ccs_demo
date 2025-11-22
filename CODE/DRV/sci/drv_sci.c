/**
 * @file drv_sci.c
 * @brief SCI 驱动实现，兼容 SysCfg 配置与手动初始化两种模式。
 */

#include "drv_sci.h"

#include "device.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sci.h"
#include "driverlib/sysctl.h"

#if DRV_SCI_USE_SYSCFG
#include "board.h"
#endif

#define DRV_SCI_DEFAULT_BASE         (SCIA_BASE)
#define DRV_SCI_DEFAULT_BAUDRATE     (115200UL)
#define DRV_SCI_DEFAULT_CONFIG       (SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE)

#if !DRV_SCI_USE_SYSCFG
#define DRV_SCI_GPIO_TX              (GPIO16)
#define DRV_SCI_GPIO_RX              (GPIO17)
#endif

typedef struct
{
    DRV_SCI_State state;
} DRV_SCI_Internal;

static DRV_SCI_Internal s_sci =
{
    .state =
    {
        .base        = DRV_SCI_DEFAULT_BASE,
        .baudRate    = DRV_SCI_DEFAULT_BAUDRATE,
        .initialized = false,
        .onRx        = NULL,
        .onTx        = NULL
    }
};

#if !DRV_SCI_USE_SYSCFG
static void DRV_SCI_enableModuleClock(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SCIA);
    SysCtl_resetPeripheral(SYSCTL_PERIPH_RES_SCIA);
}

static void DRV_SCI_configureGPIO(void)
{
    GPIO_setPinConfig(GPIO_16_SCIA_TX);
    GPIO_setPinConfig(GPIO_17_SCIA_RX);

    GPIO_setDirectionMode(DRV_SCI_GPIO_TX, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(DRV_SCI_GPIO_RX, GPIO_DIR_MODE_IN);

    GPIO_setPadConfig(DRV_SCI_GPIO_TX, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(DRV_SCI_GPIO_RX, GPIO_PIN_TYPE_STD);

    GPIO_setQualificationMode(DRV_SCI_GPIO_RX, GPIO_QUAL_ASYNC);
}

static void DRV_SCI_configureModule(void)
{
    uint32_t base = s_sci.state.base;

    SCI_disableModule(base);
    SCI_setConfig(base,
                  DEVICE_LSPCLK_FREQ,
                  s_sci.state.baudRate,
                  DRV_SCI_DEFAULT_CONFIG);
    SCI_resetChannels(base);
    SCI_enableFIFO(base);
    SCI_resetRxFIFO(base);
    SCI_resetTxFIFO(base);
    SCI_enableModule(base);

    SCI_enableInterrupt(base,
                        SCI_INT_RXFF |
                        SCI_INT_TXFF |
                        SCI_INT_FE   |
                        SCI_INT_OE   |
                        SCI_INT_PE   |
                        SCI_INT_RXERR);
    SCI_setFIFOInterruptLevel(base, SCI_FIFO_TX2, SCI_FIFO_RX2);
    SCI_performSoftwareReset(base);
}
#endif

void DRV_SCI_init(void)
{
    if(s_sci.state.initialized)
    {
        return;
    }

#if DRV_SCI_USE_SYSCFG
    s_sci.state.base = mySCI0_BASE;
#else
    s_sci.state.base = DRV_SCI_DEFAULT_BASE;
    DRV_SCI_enableModuleClock();
    DRV_SCI_configureGPIO();
    DRV_SCI_configureModule();
#endif

    s_sci.state.initialized = true;
}

void DRV_SCI_registerCallbacks(DRV_SCI_Callback rx, DRV_SCI_Callback tx)
{
    s_sci.state.onRx = rx;
    s_sci.state.onTx = tx;
}

bool DRV_SCI_readChar(uint16_t *data)
{
    if((data == NULL) || (!s_sci.state.initialized))
    {
        return false;
    }

    if(SCI_getRxFIFOStatus(s_sci.state.base) == SCI_FIFO_RX0)
    {
        return false;
    }

    *data = SCI_readCharNonBlocking(s_sci.state.base);
    return true;
}

bool DRV_SCI_writeChar(uint16_t data)
{
    if(!s_sci.state.initialized)
    {
        return false;
    }

    if(SCI_getTxFIFOStatus(s_sci.state.base) >= SCI_FIFO_TX15)
    {
        return false;
    }

    SCI_writeCharNonBlocking(s_sci.state.base, data);
    return true;
}

void DRV_SCI_getState(DRV_SCI_State *state)
{
    if(state == NULL)
    {
        return;
    }

    *state = s_sci.state;
}

__interrupt void INT_mySCI0_RX_ISR(void)
{
    if(s_sci.state.onRx != NULL)
    {
        s_sci.state.onRx();
    }

#if DRV_SCI_USE_SYSCFG
    SCI_clearInterruptStatus(mySCI0_BASE,
                            SCI_INT_RXFF | SCI_INT_FE | SCI_INT_OE |
                            SCI_INT_PE   | SCI_INT_RXERR);
#else
    SCI_clearInterruptStatus(s_sci.state.base,
                             SCI_INT_RXFF | SCI_INT_FE | SCI_INT_OE |
                             SCI_INT_PE   | SCI_INT_RXERR);
#endif

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

__interrupt void INT_mySCI0_TX_ISR(void)
{
    if(s_sci.state.onTx != NULL)
    {
        s_sci.state.onTx();
    }

#if DRV_SCI_USE_SYSCFG
    SCI_clearInterruptStatus(mySCI0_BASE, SCI_INT_TXFF);
#else
    SCI_clearInterruptStatus(s_sci.state.base, SCI_INT_TXFF);
#endif

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
