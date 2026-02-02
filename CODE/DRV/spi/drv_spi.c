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

static volatile uint16_t s_spi0RxStorage[SPI0_RX_BUF_LEN];
static volatile uint16_t s_spi0TxStorage[SPI0_TX_BUF_LEN];

static DRV_SPI_RingBuffer s_spi0RxQueue =
{
    .buffer = s_spi0RxStorage,
    .length = SPI0_RX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
};

static DRV_SPI_RingBuffer s_spi0TxQueue =
{
    .buffer = s_spi0TxStorage,
    .length = SPI0_TX_BUF_LEN,
    .head   = 0U,
    .tail   = 0U,
};

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

uint16_t DRV_SPI0_RxReadWords(uint16_t *pBuf, uint16_t len)
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
        head = s_spi0RxQueue.head;
        tail = s_spi0RxQueue.tail;

        if (tail == head)
        {
            break;
        }

        pBuf[i] = s_spi0RxQueue.buffer[tail];
        s_spi0RxQueue.tail = DRV_SPI_nextIndex(tail, s_spi0RxQueue.length);
    }

    return i;
}

uint16_t DRV_SPI0_TxWriteWords(const uint16_t *pData, uint16_t len)
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
        head = s_spi0TxQueue.head;
        tail = s_spi0TxQueue.tail;

        nextHead = DRV_SPI_nextIndex(head, s_spi0TxQueue.length);

        if (nextHead == tail)
        {
            break;
        }

        s_spi0TxQueue.buffer[head] = pData[i];
        s_spi0TxQueue.head         = nextHead;
    }

    if (i > 0U)
    {
        SPI_enableInterrupt(s_spiState.base, SPI_INT_TXFF);
    }

    return i;
}


void spi_send_string(const char *str)
{
    uint16_t i = 0;
    uint16_t spi_tx_buf[64];
    
    while (str[i] != '\0' && i < (sizeof(spi_tx_buf)/sizeof(spi_tx_buf[0])))
    {
        spi_tx_buf[i] = (uint16_t)(str[i] & 0x00FFu);
        i++;
    }

    // 片选（如果你是手动片选）
    // GPIO_writePin(CS_GPIO, 0);

    // 写入发送队列
    DRV_SPI0_TxWriteWords(spi_tx_buf, i);

    // 可选：清掉 RX（避免 RX 堆积）
    uint16_t dummy[64];
    DRV_SPI0_RxReadWords(dummy, i);

}


__interrupt void INT_mySPI0_RX_ISR(void){

    uint16_t data;
    uint16_t nextHead;
    uint16_t fifoStatus;

    do
    {
        fifoStatus = SPI_getRxFIFOStatus(s_spiState.base);
        if (fifoStatus != SPI_FIFO_RXEMPTY)
        {
            data = SPI_readDataNonBlocking(s_spiState.base);
            nextHead = DRV_SPI_nextIndex(s_spi0RxQueue.head, s_spi0RxQueue.length);

            if (nextHead != s_spi0RxQueue.tail)
            {
                s_spi0RxQueue.buffer[s_spi0RxQueue.head] = data;
                s_spi0RxQueue.head = nextHead;
            }
        }
    } while (fifoStatus != SPI_FIFO_RXEMPTY);

    SPI_clearInterruptStatus(s_spiState.base, SPI_INT_RXFF | SPI_INT_RXFF_OVERFLOW | SPI_INT_RX_OVERRUN);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
}


__interrupt void INT_mySPI0_TX_ISR(void){

    uint16_t head;
    uint16_t tail;
    uint16_t fifoStatus;
    uint16_t data;

    head = s_spi0TxQueue.head;
    tail = s_spi0TxQueue.tail;

    while (tail != head)
    {
        fifoStatus = SPI_getTxFIFOStatus(s_spiState.base);
        if (fifoStatus == SPI_FIFO_TXFULL)
        {
            break;
        }

        data = s_spi0TxQueue.buffer[tail];
        tail = DRV_SPI_nextIndex(tail, s_spi0TxQueue.length);

        SPI_writeDataNonBlocking(s_spiState.base, data);
    }

    s_spi0TxQueue.tail = tail;

    if (tail == s_spi0TxQueue.head)
    {
        SPI_disableInterrupt(s_spiState.base, SPI_INT_TXFF);
    }

    SPI_clearInterruptStatus(s_spiState.base, SPI_INT_TXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP6);
}
