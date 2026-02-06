/**
 * @file drv_spi.c
 * @brief SPI 驱动实现文件，完成 SPI 外设初始化与 DRV8316 兼容绑定。
 */

#include "__INCLUDE.h"

#define DRV_SPI_DEFAULT_BASE            (SPIA_BASE)        /**< 默认使用 SPIA 外设作为通信控制器。 */
#define DRV_SPI_DEFAULT_BITRATE_HZ      (1000000UL)        /**< 默认 SPI 波特率 1 MHz，兼顾 DRV8316 的时序要求与 EMC。 */
#define DRV_SPI_DEFAULT_DATA_WIDTH      (16U)              /**< DRV8316 寄存器宽度为 16 bit，对齐读写操作。 */

static DRV_SPI_State s_spiState =
{
    .base        = DRV_SPI_DEFAULT_BASE,
    .bitRate     = DRV_SPI_DEFAULT_BITRATE_HZ,
    .dataWidth   = DRV_SPI_DEFAULT_DATA_WIDTH,
    .csGpio      = DRV_SPI_INVALID_GPIO,
    .enableGpio  = DRV_SPI_INVALID_GPIO,
    .initialized = false
};

/* ========================================================================== */
/* 队列异步通信：使用环形缓冲实现 TX/RX 解耦                                   */
/* ========================================================================== */
static volatile uint16_t s_spi0QueueRxStorage[SPI0_RX_BUF_LEN];
static volatile uint16_t s_spi0QueueTxStorage[SPI0_TX_BUF_LEN];

static DRV_SPI_RingBuffer s_spi0QueueRx =
{
    .buffer = s_spi0QueueRxStorage,
    .length = SPI0_RX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
};

static DRV_SPI_RingBuffer s_spi0QueueTx =
{
    .buffer = s_spi0QueueTxStorage,
    .length = SPI0_TX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
};

/* ========================================================================== */
/* 会话式全双工通信：命令 + dummy 时钟 + 响应                                  */
/* ========================================================================== */
static volatile uint16_t s_spi0SessionRxStorage[SPI0_SESSION_RX_BUF_LEN];

static DRV_SPI_RingBuffer s_spi0SessionRx =
{
    .buffer = s_spi0SessionRxStorage,
    .length = SPI0_SESSION_RX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
};

/* 会话式全双工流程说明：
 * 1) TX ISR 先发送命令字；
 * 2) TX ISR 再发送 dummy 字以维持 SCLK；
 * 3) RX ISR 丢弃命令回波；
 * 4) RX ISR 采集期望响应并写入会话 RX 缓冲。
 */
typedef struct
{
    /* 会话激活标志：1 表示进入会话式 TX/RX 处理流程 */
    uint16_t active;
    /* 待发送命令/数据字数 */
    uint16_t txRemaining;
    /* 需要丢弃的回波字数（等于命令长度） */
    uint16_t rxIgnore;
    /* 需要继续发送的 dummy 字数，用于维持 SPI 时钟 */
    uint16_t dummyRemaining;
    /* 期望接收的响应字数，写入会话 RX 缓冲 */
    uint16_t rxExpect;
    /* 命令/数据发送指针 */
    const uint16_t *txPtr;
    /* dummy 填充值（典型为 0xFFFF） */
    uint16_t dummyWord;
} DRV_SPI_Session;

static volatile DRV_SPI_Session s_spi0Session = {0};
/* 从机就绪线拉高后的“待接收字数”计数，用于自动拉低 */
static volatile uint16_t s_spi0ReadyRxRemaining = 0U;

#if !DRV_SPI_USE_SYSCFG
static void DRV_SPI_enableModuleClock(void)
{
    /**
     * SPI 模块的寄存器访问依赖于外设时钟。根据选择的 SPI 基地址，
     * 使能对应的外设时钟域，确保后续的寄存器配置合法。
     */
    if(s_spiState.base == SPIA_BASE)
    {
        SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPIA);
    }
    else if(s_spiState.base == SPIB_BASE)
    {
        SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPIB);
    }
}

static void DRV_SPI_configureFunctionalPins(void)
{
    /**
     * 将 SPI 功能信号映射到器件预定义的引脚，并配置其输入同步方式
     * 与上拉属性，确保高速 SPI 信号的完整性与抗干扰能力。
     */
    // GPIO_setPinConfig(DEVICE_GPIO_CFG_SPICLKA);
    // GPIO_setPinConfig(DEVICE_GPIO_CFG_SPISIMOA);
    // GPIO_setPinConfig(DEVICE_GPIO_CFG_SPISOMIA);

    // GPIO_setQualificationMode(DEVICE_GPIO_PIN_SPICLKA, GPIO_QUAL_ASYNC);
    // GPIO_setQualificationMode(DEVICE_GPIO_PIN_SPISIMOA, GPIO_QUAL_ASYNC);
    // GPIO_setQualificationMode(DEVICE_GPIO_PIN_SPISOMIA, GPIO_QUAL_ASYNC);

//     GPIO_setPadConfig(DEVICE_GPIO_PIN_SPICLKA, GPIO_PIN_TYPE_PULLUP);
//     GPIO_setPadConfig(DEVICE_GPIO_PIN_SPISIMOA, GPIO_PIN_TYPE_PULLUP);
//     GPIO_setPadConfig(DEVICE_GPIO_PIN_SPISOMIA, GPIO_PIN_TYPE_PULLUP);
}
#endif

static void DRV_SPI_configureChipSelectPin(uint32_t gpio)
{
    /**
     * 片选引脚通常由软件控制，驱动层需提供通用的 GPIO 配置逻辑。
     * 当传入无效引脚时直接返回，避免错误操作其它 IO。
     */
    if(gpio == DRV_SPI_INVALID_GPIO)
    {
        return;
    }

    /* 输出方向 + 同步采样，满足 SPI 片选的时序要求。 */
    GPIO_setDirectionMode(gpio, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(gpio, GPIO_QUAL_SYNC);
    GPIO_setPadConfig(gpio, GPIO_PIN_TYPE_STD);
    /* 片选默认拉高，保持从设备不被选中。 */
    GPIO_writePin(gpio, 1U);
}

static void DRV_SPI_configureEnablePin(uint32_t gpio)
{
    /**
     * 使能引脚用于控制功率驱动芯片的上电时序，与片选配置一致，
     * 需要在初始化阶段将引脚置为输出并拉高，防止误触发。
     */
    if(gpio == DRV_SPI_INVALID_GPIO)
    {
        return;
    }

    GPIO_setDirectionMode(gpio, GPIO_DIR_MODE_OUT);
    GPIO_setQualificationMode(gpio, GPIO_QUAL_SYNC);
    GPIO_setPadConfig(gpio, GPIO_PIN_TYPE_STD);
    /* 默认置 1，确保驱动芯片处于禁止状态，由上层决定开启时机。 */
    GPIO_writePin(gpio, 1U);
}

void DRV_SPI_init(void)
{
    /**
     * 初始化过程包含：
     * 1. 打开外设时钟；
     * 2. 配置 SPI 功能引脚；
     * 3. 设置 SPI 控制寄存器（极性、相位、位宽、波特率等）；
     * 4. 使能 FIFO 并清除历史中断状态；
     * 5. 初始化软件片选及使能引脚。
     * 初始化完成后会将状态标记为已初始化，避免重复执行。
     */
    if(s_spiState.initialized)
    {
        return;
    }
#if DRV_SPI_USE_SYSCFG
#if defined(mySPI0_BASE)
    s_spiState.base = mySPI0_BASE;
#endif
#if defined(mySPI0_BITRATE)
    s_spiState.bitRate = mySPI0_BITRATE;
#endif
#if defined(mySPI0_DATAWIDTH)
    s_spiState.dataWidth = mySPI0_DATAWIDTH;
#endif
#if defined(mySPI0_BASE)
    SPI_clearInterruptStatus(s_spiState.base, SPI_INT_RXFF | SPI_INT_RXFF_OVERFLOW | SPI_INT_TXFF);
    SPI_disableInterrupt(s_spiState.base, SPI_INT_TXFF);
#endif
#else
    DRV_SPI_enableModuleClock();

    DRV_SPI_configureFunctionalPins();

    SPI_disableModule(s_spiState.base);
    /* Configure SPI as controller, CPOL=0, CPHA=1, using LSPCLK. */
    SPI_setConfig(s_spiState.base,
                  DEVICE_LSPCLK_FREQ,
                  SPI_PROT_POL0PHA1,
                  SPI_MODE_CONTROLLER,
                  s_spiState.bitRate,
                  s_spiState.dataWidth);
    SPI_setEmulationMode(s_spiState.base, SPI_EMULATION_FREE_RUN);

    SPI_enableModule(s_spiState.base);

    SPI_enableFIFO(s_spiState.base);
    /* Transmit delay 0 so data moves to shift register immediately. */
    SPI_setTxFifoTransmitDelay(s_spiState.base, 0U);
    /* FIFO interrupt level. */
    SPI_setFIFOInterruptLevel(s_spiState.base, SPI_FIFO_TX0, SPI_FIFO_RX1);
    /* Clear pending interrupt flags. */
    SPI_clearInterruptStatus(s_spiState.base,
                             SPI_INT_RX_DATA_TX_EMPTY |
                             SPI_INT_RX_OVERRUN |
                             SPI_INT_RXFF |
                             SPI_INT_RXFF_OVERFLOW |
                             SPI_INT_TXFF);
#endif


    DRV_SPI_configureChipSelectPin(s_spiState.csGpio);
    DRV_SPI_configureEnablePin(s_spiState.enableGpio);

    s_spiState.initialized = true;
}

void DRV_SPI_setChipSelectGPIO(uint32_t gpio)
{
    /**
     * 运行期允许动态调整片选引脚，满足不同板级设计的复用需求。
     * 若驱动已经初始化，则立即对新引脚执行 GPIO 配置。
     */
    s_spiState.csGpio = gpio;

    if(s_spiState.initialized)
    {
        DRV_SPI_configureChipSelectPin(gpio);
    }
}

void DRV_SPI_setEnableGPIO(uint32_t gpio)
{
    /**
     * 使能引脚与片选配置逻辑一致，支持运行中重新映射，
     * 以适配双驱动或硬件版本差异。
     */
    s_spiState.enableGpio = gpio;

    if(s_spiState.initialized)
    {
        DRV_SPI_configureEnablePin(gpio);
    }
}

void DRV_SPI_getState(DRV_SPI_State *state)
{
    /**
     * 提供线程安全的状态读取：结构体通过值复制返回，
     * 调用者可据此判断初始化标志或检查当前配置。
     */
    if(state != NULL)
    {
        *state = s_spiState;
    }
}

void DRV_SPI_attachToDRV8316(DRV8316_Handle handle, uint32_t csGpio, uint32_t enableGpio)
{
    /**
     * 为保持与现有 DRV8316 软件栈兼容，此接口封装了 SPI 初始化
     * 及 GPIO 绑定流程，并将 SPI 基地址交给 DRV8316 底层驱动使用。
     */
    if(handle == NULL)
    {
        return;
    }

    DRV_SPI_init();

    if(csGpio != DRV_SPI_INVALID_GPIO)
    {
        DRV_SPI_setChipSelectGPIO(csGpio);
        DRV8316_setGPIOCSNumber(handle, csGpio);
    }

    if(enableGpio != DRV_SPI_INVALID_GPIO)
    {
        DRV_SPI_setEnableGPIO(enableGpio);
        DRV8316_setGPIOENNumber(handle, enableGpio);
    }

    DRV8316_setSPIHandle(handle, s_spiState.base);
}

/* ========================================================================== */
/* 内部辅助函数                                                              */
/* ========================================================================== */
static void DRV_SPI0_ClearRxFifo(void)
{
    uint16_t dummy;

    while (SPI_getRxFIFOStatus(s_spiState.base) != SPI_FIFO_RXEMPTY)
    {
        dummy = SPI_readDataNonBlocking(s_spiState.base);
        (void)dummy;
    }

    SPI_clearInterruptStatus(s_spiState.base,
                             SPI_INT_RXFF | SPI_INT_RXFF_OVERFLOW | SPI_INT_RX_OVERRUN);
}

static void DRV_SPI0_RxHandleQueue(uint16_t data)
{
    uint16_t nextHead;

    nextHead = DRV_SPI_nextIndex(s_spi0QueueRx.head, s_spi0QueueRx.length);
    if (nextHead != s_spi0QueueRx.tail)
    {
        s_spi0QueueRx.buffer[s_spi0QueueRx.head] = data;
        s_spi0QueueRx.head = nextHead;
    }
}

static void DRV_SPI0_RxHandleSession(uint16_t data)
{
    uint16_t nextHead;

    /* 先丢弃命令回波，再接收响应数据 */
    if (s_spi0Session.rxIgnore > 0U)
    {
        s_spi0Session.rxIgnore--;
        return;
    }

    if (s_spi0Session.rxExpect > 0U)
    {
        nextHead = DRV_SPI_nextIndex(s_spi0SessionRx.head, s_spi0SessionRx.length);
        if (nextHead != s_spi0SessionRx.tail)
        {
            s_spi0SessionRx.buffer[s_spi0SessionRx.head] = data;
            s_spi0SessionRx.head = nextHead;
        }
        s_spi0Session.rxExpect--;
    }
    else
    {
        /* 超出期望长度的多余数据直接丢弃 */
    }
}

static void DRV_SPI0_TxHandleSession(void)
{
    uint16_t fifoStatus;
    uint16_t data;

    for (;;)
    {
        fifoStatus = SPI_getTxFIFOStatus(s_spiState.base);
        if (fifoStatus == SPI_FIFO_TXFULL)
        {
            break;
        }

        if (s_spi0Session.txRemaining > 0U)
        {
            /* 发送命令/数据字，同时产生回波 */
            data = *s_spi0Session.txPtr++;
            s_spi0Session.txRemaining--;
            SPI_writeDataNonBlocking(s_spiState.base, data);
            continue;
        }

        if (s_spi0Session.dummyRemaining > 0U)
        {
            /* 发送 dummy 字以维持时钟，驱动从机回传 */
            SPI_writeDataNonBlocking(s_spiState.base, s_spi0Session.dummyWord);
            s_spi0Session.dummyRemaining--;
            continue;
        }

        /* 会话发送完成，关闭 TX 中断 */
        SPI_disableInterrupt(s_spiState.base, SPI_INT_TXFF);
        break;
    }
}

static void DRV_SPI0_TxHandleQueue(void)
{
    uint16_t head = s_spi0QueueTx.head;
    uint16_t tail = s_spi0QueueTx.tail;
    uint16_t fifoStatus;
    uint16_t data;

    while (tail != head)
    {
        fifoStatus = SPI_getTxFIFOStatus(s_spiState.base);
        if (fifoStatus == SPI_FIFO_TXFULL)
        {
            break;
        }

        data = s_spi0QueueTx.buffer[tail];
        tail = DRV_SPI_nextIndex(tail, s_spi0QueueTx.length);

        SPI_writeDataNonBlocking(s_spiState.base, data);
    }

    s_spi0QueueTx.tail = tail;

    if (tail == s_spi0QueueTx.head)
    {
        SPI_disableInterrupt(s_spiState.base, SPI_INT_TXFF);
    }
}

/* ========================================================================== */
/* 队列异步通信接口                                                          */
/* ========================================================================== */
uint16_t DRV_SPI0_QueueReadWords(uint16_t *pBuf, uint16_t len)
{
    uint16_t i;
    uint16_t head;
    uint16_t tail;

    if ((pBuf == NULL) || (len == 0U))
    {
        return 0U;
    }

    /* 会话进行中时不允许读取普通队列 */
    if (s_spi0Session.active != 0U)
    {
        return 0U;
    }

    for (i = 0U; i < len; i++)
    {
        head = s_spi0QueueRx.head;
        tail = s_spi0QueueRx.tail;

        if (tail == head)
        {
            break;
        }

        pBuf[i] = s_spi0QueueRx.buffer[tail];
        s_spi0QueueRx.tail = DRV_SPI_nextIndex(tail, s_spi0QueueRx.length);
    }

    return i;
}

uint16_t DRV_SPI0_QueueWriteWords(const uint16_t *pData, uint16_t len)
{
    uint16_t head;
    uint16_t tail;
    uint16_t nextHead;
    uint16_t i = 0U;

    if ((pData == NULL) || (len == 0U))
    {
        return 0U;
    }

    /* 会话进行中时禁止向普通 TX 队列写入 */
    if (s_spi0Session.active != 0U)
    {
        return 0U;
    }

    for (i = 0U; i < len; i++)
    {
        head = s_spi0QueueTx.head;
        tail = s_spi0QueueTx.tail;

        nextHead = DRV_SPI_nextIndex(head, s_spi0QueueTx.length);

        if (nextHead == tail)
        {
            break;
        }

        s_spi0QueueTx.buffer[head] = pData[i];
        s_spi0QueueTx.head         = nextHead;
    }

    if (i > 0U)
    {
        SPI_enableInterrupt(s_spiState.base, SPI_INT_TXFF);
    }

    return i;
}

void DRV_SPI0_QueueRxFlush(void)
{
    /* 会话进行中时不允许清空普通 RX 队列 */
    if (s_spi0Session.active != 0U)
    {
        return;
    }

    s_spi0QueueRx.head = s_spi0QueueRx.tail;
    DRV_SPI0_ClearRxFifo();
}

void DRV_SPI0_ReadyLineArm(uint16_t rxCount)
{
#if (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
    if (rxCount == 0U)
    {
        return;
    }

    /* 拉高就绪线，等待主机读出指定字数后自动拉低 */
    s_spi0ReadyRxRemaining = rxCount;
    GPIO_writePin(APP_PROTO_SPI_READY_GPIO, 1U);
#else
    (void)rxCount;
#endif
}

/* ========================================================================== */
/* 会话式全双工通信接口                                                      */
/* ========================================================================== */
uint16_t DRV_SPI0_SessionReadWords(uint16_t *pBuf, uint16_t len)
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
        head = s_spi0SessionRx.head;
        tail = s_spi0SessionRx.tail;

        if (tail == head)
        {
            break;
        }

        pBuf[i] = s_spi0SessionRx.buffer[tail];
        s_spi0SessionRx.tail = DRV_SPI_nextIndex(tail, s_spi0SessionRx.length);
    }

    return i;
}

uint16_t DRV_SPI0_SessionBegin(const uint16_t *pTx, uint16_t txLen, uint16_t rxExpect)
{
    /* 参数检查：至少要发送或接收 */
    if ((txLen == 0U) && (rxExpect == 0U))
    {
        return 0U;
    }

    /* txLen 非 0 时，发送指针必须有效 */
    if ((pTx == NULL) && (txLen != 0U))
    {
        return 0U;
    }

    /* 会话互斥：一次仅允许一个会话 */
    if (s_spi0Session.active != 0U)
    {
        return 0U;
    }

    /* 确保普通 TX 队列为空，避免混用 */
    if (s_spi0QueueTx.head != s_spi0QueueTx.tail)
    {
        return 0U;
    }

    /* 清空会话 RX 缓冲并刷新硬件 FIFO */
    s_spi0SessionRx.head = s_spi0SessionRx.tail;
    DRV_SPI0_ClearRxFifo();

    /* 初始化会话状态 */
    s_spi0Session.txPtr          = pTx;
    s_spi0Session.txRemaining    = txLen;
    s_spi0Session.rxIgnore       = txLen;
    s_spi0Session.dummyRemaining = rxExpect;
    s_spi0Session.rxExpect       = rxExpect;
    s_spi0Session.dummyWord      = 0xFFFFu;
    s_spi0Session.active         = 1U;

    /* 使能 TX FIFO 中断以启动发送 */
    SPI_enableInterrupt(s_spiState.base, SPI_INT_TXFF);

    return txLen;
}

uint16_t DRV_SPI0_SessionIsActive(void)
{
    return s_spi0Session.active;
}

/* ========================================================================== */
/* 兼容旧接口（内部调用新接口）                                              */
/* ========================================================================== */
uint16_t DRV_SPI0_RxReadWords(uint16_t *pBuf, uint16_t len)
{
    return DRV_SPI0_QueueReadWords(pBuf, len);
}

uint16_t DRV_SPI0_TxWriteWords(const uint16_t *pData, uint16_t len)
{
    return DRV_SPI0_QueueWriteWords(pData, len);
}

void DRV_SPI0_RxFlush(void)
{
    DRV_SPI0_QueueRxFlush();
}

uint16_t DRV_SPI0_BeginSession(const uint16_t *pTx, uint16_t txLen, uint16_t rxExpect)
{
    return DRV_SPI0_SessionBegin(pTx, txLen, rxExpect);
}

/* ========================================================================== */
/* 调试/辅助接口                                                             */
/* ========================================================================== */
void spi_send_string(const char *str)
{
    uint16_t i = 0;
    uint16_t spi_tx_buf[64];

    while (str[i] != '\0' && i < (sizeof(spi_tx_buf) / sizeof(spi_tx_buf[0])))
    {
        spi_tx_buf[i] = (uint16_t)(str[i] & 0x00FFu);
        i++;
    }

    /* 通过普通队列发送字符串 */
    DRV_SPI0_QueueWriteWords(spi_tx_buf, i);

    /* 可选：读取并丢弃回波数据 */
    {
        uint16_t dummy[64];
        DRV_SPI0_QueueReadWords(dummy, i);
    }
}

/* ========================================================================== */
/* SPI 中断服务函数                                                          */
/* ========================================================================== */
__interrupt void INT_mySPI0_RX_ISR(void){

    uint16_t data;
    uint16_t fifoStatus;

    /* 读取 RX FIFO，并根据模式分发到会话或队列 */
    do
    {
        fifoStatus = SPI_getRxFIFOStatus(s_spiState.base);
        if (fifoStatus != SPI_FIFO_RXEMPTY)
        {
            data = SPI_readDataNonBlocking(s_spiState.base);

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
            /* 从机侧：统计主机读出的字数，到达后拉低就绪线 */
            if (s_spi0ReadyRxRemaining > 0U)
            {
                s_spi0ReadyRxRemaining--;
                if (s_spi0ReadyRxRemaining == 0U)
                {
                    GPIO_writePin(APP_PROTO_SPI_READY_GPIO, 0U);
                }
            }
#endif

            if (s_spi0Session.active != 0U)
            {
                DRV_SPI0_RxHandleSession(data);
            }
            else
            {
                DRV_SPI0_RxHandleQueue(data);
            }
        }
    } while (fifoStatus != SPI_FIFO_RXEMPTY);

    /* 命令与 dummy 发送完成后结束会话 */
    if ((s_spi0Session.active != 0U) &&
        (s_spi0Session.txRemaining == 0U) &&
        (s_spi0Session.dummyRemaining == 0U))
    {
        s_spi0Session.active   = 0U;
        s_spi0Session.rxExpect = 0U;
        s_spi0Session.rxIgnore = 0U;
        s_spi0Session.txPtr    = (const uint16_t *)0;
    }

    SPI_clearInterruptStatus(s_spiState.base, SPI_INT_RXFF | SPI_INT_RXFF_OVERFLOW | SPI_INT_RX_OVERRUN);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
}

__interrupt void INT_mySPI0_TX_ISR(void){

    /* 会话模式优先，否则走普通队列 */
    if (s_spi0Session.active != 0U)
    {
        DRV_SPI0_TxHandleSession();
        SPI_clearInterruptStatus(s_spiState.base, SPI_INT_TXFF);
        Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
        return;
    }

    DRV_SPI0_TxHandleQueue();

    SPI_clearInterruptStatus(s_spiState.base, SPI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
}
