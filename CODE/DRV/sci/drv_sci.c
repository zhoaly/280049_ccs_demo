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

static volatile uint16_t s_sci0RxStorage[SCI0_RX_BUF_LEN];
static volatile uint16_t s_sci0TxStorage[SCI0_TX_BUF_LEN];

static DRV_SCI_RingBuffer s_sci0RxQueue =
{
    .buffer = s_sci0RxStorage,
    .length = SCI0_RX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
};

static DRV_SCI_RingBuffer s_sci0TxQueue =
{
    .buffer = s_sci0TxStorage,
    .length = SCI0_TX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
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
uint16_t DRV_SCI0_RxReadBytes(uint16_t *pBuf, uint16_t len)
{
    uint16_t i;
    uint16_t head;
    uint16_t tail;

    if ((pBuf == NULL) || (len == 0U))
    {
        return 0U;
    }
    
    for (i = 0U; i < len; i++)
    {
        /* 快照当前指针 */
        head = s_sci0RxQueue.head;
        tail = s_sci0RxQueue.tail;

        /* 缓冲区为空：head == tail，提前退出 */
        if (tail == head)
        {
            break;
        }

        /* 取出 1 个字节（低 8 位有效） */
        pBuf[i] = s_sci0RxQueue.buffer[tail] & 0x00FFU;

        /* 推进读指针 */
        s_sci0RxQueue.tail = nextIndex(tail, s_sci0RxQueue.length);
    }

    return i;
}



/**
 * @brief 向 SCI0 发送环形缓冲区写入数据（低 8 位为有效字节）。
 *
 * @param[in] pData  数据缓冲区指针（每个 Uint16 的低 8 位为一个字节）
 * @param[in] len    待写入的字节数
 *
 * @return 实际写入缓冲区的字节数（可能小于 len，表示缓冲区已满）
 *
 * 说明：
 * - 假设只有一个任务调用本函数写入（单生产者），中断为单消费者。
 * - 本函数为“非阻塞”写入，如果缓冲区满，会提前退出。
 * - 若实际写入长度 > 0，会打开 TX FIFO 中断，触发中断发送。
 */
uint16_t DRV_SCI0_TxWriteBytes(const uint16_t *pData, uint16_t len)
{
    uint16_t head;
    uint16_t tail;
    uint16_t nextHead;
    uint16_t i = 0U;

    if ((pData == NULL) || (len == 0U))
    {
        return 0U;
    }


    
    for (i = 0U; i < len; i++)
    {
        head = s_sci0TxQueue.head;
        tail = s_sci0TxQueue.tail;

        nextHead = nextIndex(head, s_sci0TxQueue.length);

        // 缓冲区已满：下一个 head 等于 tail(这里留了一个位置作为哨兵空位)
        if (nextHead == tail)
        {
            break;  // 已经写满，退出循环
        }

        // 写入 1 字节，低 8 位有效
        s_sci0TxQueue.buffer[head] = pData[i] & 0x00FFU;
        s_sci0TxQueue.head         = nextHead;
    }

    // 如果写入了至少 1 个字节，则打开 TX FIFO 中断，由中断继续发送
    // 保护性判断,当且仅当写入时缓冲区满时,不进入此分支
    if (i > 0U)
    {
        SCI_enableInterrupt(mySCI0_BASE, SCI_INT_TXFF);
    }

    return i;
}


__interrupt void INT_mySCI0_RX_ISR(void)
{
    uint16_t head;
    uint16_t fifoStatus;//fifo状态临时变量
    uint16_t data;//临时变量

    if(s_sci.state.onRx != NULL)
    {
        s_sci.state.onRx();
    }

//以下是接收逻辑

    // 把 FIFO 里当前所有字节都读出来，放进环形缓冲区
    do
    {
        fifoStatus = SCI_getRxFIFOStatus(mySCI0_BASE);
        if (fifoStatus != SCI_FIFO_RX0)      // FIFO 非空
        {
            data = SCI_readCharBlockingFIFO(mySCI0_BASE);
            head = nextIndex(s_sci0RxQueue.head, s_sci0RxQueue.length);

            s_sci0RxQueue.buffer[s_sci0RxQueue.head] = data;
            s_sci0RxQueue.head = head;
        }
    } while (fifoStatus != SCI_FIFO_RX0);


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
    uint16_t head;
    uint16_t tail;
    uint16_t fifoStatus;
    uint16_t data;

    if(s_sci.state.onTx != NULL)
    {
        s_sci.state.onTx();
    }

//以下是发送逻辑
    // 快照当前 head / tail
    head = s_sci0TxQueue.head;
    tail = s_sci0TxQueue.tail;

    // 只要 FIFO 未满 且 缓冲区中还有数据，就不断填充 FIFO
    while (tail != head)
    {
        fifoStatus = SCI_getTxFIFOStatus(mySCI0_BASE);

        // FIFO 已满，无法再写，提前退出
        if (fifoStatus == SCI_FIFO_TX16)
        {
            break;
        }

        // 取出一个待发送字节（低 8 位有效）
        data = s_sci0TxQueue.buffer[tail] & 0x00FFU;
        tail = nextIndex(tail, s_sci0TxQueue.length);

        // 写入 TX FIFO（非阻塞）
        SCI_writeCharNonBlocking(mySCI0_BASE, data);
    }

    // 更新全局 tail 指针
    s_sci0TxQueue.tail = tail;

    // 如果缓冲区已经空了，则关掉 TX FIFO 中断，防止 FIFO 空时产生持续中断
    if (tail == s_sci0TxQueue.head)
    {
        SCI_disableInterrupt(mySCI0_BASE, SCI_INT_TXFF);
    }

#if DRV_SCI_USE_SYSCFG
    SCI_clearInterruptStatus(mySCI0_BASE, SCI_INT_TXFF);
#else
    SCI_clearInterruptStatus(s_sci.state.base, SCI_INT_TXFF);
#endif

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
