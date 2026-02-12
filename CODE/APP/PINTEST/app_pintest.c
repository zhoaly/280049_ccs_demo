/**
 * @file app_pintest.c
 * @brief APP 层引脚电平互连测试（Pin-to-Pin）模块实现。
 *
 * 逻辑流程（主机）：
 * 1) 通过 PROTO 发送“设置某引脚电平”的命令（含 pinId）；
 * 2) 等待从机 ACK；
 * 3) 读取本机同名引脚电平（GPIO 或 ADC），判断是否连通；
 * 4) 可选：拉回低电平，进入下一点。
 *
 * 逻辑流程（从机）：
 * 1) 收到 WRITE 命令后，根据 pinId 拉高/拉低对应引脚；
 * 2) 由 PROTO 层发送 ACK。
 */
#include "__INCLUDE.h"

/* 模块日志标签 */
static const char *TAG = "pintest";

/**
 * @brief 测试点 IO 类型：GPIO 数字输入/ADC 模拟输入。
 */
typedef enum
{
    APP_PINTEST_IO_GPIO = 0u,
    APP_PINTEST_IO_ADC  = 1u
} APP_PINTEST_IoType;

/**
 * @brief 单个测试点描述。
 *
 * - pinId/pinConfig：GPIO 号与 pinmux 配置（GPIO_n_GPIOn）。
 * - type：读取方式（GPIO/ADC）。
 * - adcBase/adcResultBase/adcChannel：ADC 读取所需参数（仅 ADC 类型有效）。
 */
typedef struct
{
    uint16_t          pinId;
    uint32_t          pinConfig;
    APP_PINTEST_IoType type;
    uint32_t          adcBase;
    uint32_t          adcResultBase;
    ADC_Channel       adcChannel;
} APP_PINTEST_Point;

/**
 * @brief 测试点列表：由宏 APP_PINTEST_GPIO_POINT_LIST/APP_PINTEST_ADC_POINT_LIST 生成。
 */
static const APP_PINTEST_Point s_pinTestPoints[] =
{
#define X(pin, cfg) { (uint16_t)(pin), (uint32_t)(cfg), APP_PINTEST_IO_GPIO, 0u, 0u, (ADC_Channel)0 },
    APP_PINTEST_GPIO_POINT_LIST
#undef X


#define X(pin, cfg, adcBase, adcResBase, adcCh) { (uint16_t)(pin), (uint32_t)(cfg), APP_PINTEST_IO_ADC, (uint32_t)(adcBase), (uint32_t)(adcResBase), (ADC_Channel)(adcCh) },
    APP_PINTEST_ADC_POINT_LIST
#undef X
};

/**
 * @brief 测试点数量。
 */
#define APP_PINTEST_POINT_COUNT ((uint16_t)(sizeof(s_pinTestPoints) / sizeof(s_pinTestPoints[0])))

/**
 * @brief 初始化标志，确保只执行一次硬件配置。
 */
static uint16_t s_pinTestInitDone = 0u;

/**
 * @brief ADC 初始化掩码（按 ADCA/ADCB/ADCC 基址区分）。
 */
static uint16_t s_adcInitMask = 0u;

/**
 * @brief 判断 pinId 是否为可配置模拟模式的引脚（AIO/22/23）。
 */
static uint16_t APP_PINTEST_IsAnalogPin(uint16_t pinId)
{
    if (((pinId >= 224u) && (pinId <= 247u)) || (pinId == 22u) || (pinId == 23u))
    {
        return 1u;
    }
    return 0u;
}

/**
 * @brief 根据 pinId 查找测试点。
 * @return 找到返回指针，否则返回 NULL。
 */
static const APP_PINTEST_Point *APP_PINTEST_FindPoint(uint16_t pinId)
{
    uint16_t i;

    for (i = 0u; i < APP_PINTEST_POINT_COUNT; i++)
    {
        if (s_pinTestPoints[i].pinId == pinId)
        {
            return &s_pinTestPoints[i];
        }
    }

    return (const APP_PINTEST_Point *)0;
}

/**
 * @brief 判断列表中是否存在 ADC 类型测试点。
 */
static uint16_t APP_PINTEST_HasAdcPoints(void)
{
    uint16_t i;

    for (i = 0u; i < APP_PINTEST_POINT_COUNT; i++)
    {
        if (s_pinTestPoints[i].type == APP_PINTEST_IO_ADC)
        {
            return 1u;
        }
    }

    return 0u;
}

/**
 * @brief 将 ADC 基址映射为初始化掩码。
 */
static uint16_t APP_PINTEST_AdcMaskFromBase(uint32_t base)
{
    if (base == ADCA_BASE)
    {
        return 0x0001u;
    }
    if (base == ADCB_BASE)
    {
        return 0x0002u;
    }
    if (base == ADCC_BASE)
    {
        return 0x0004u;
    }

    return 0u;
}

/**
 * @brief 初始化指定 ADC 模块（去重）。
 *
 * 说明：
 * - 仅在主机侧使用，用于 ADC 类型测试点采样；
 * - 若某 ADC 已初始化则跳过；
 * - 简单配置 12bit、单端、End-of-Conv 中断脉冲。
 */
static void APP_PINTEST_InitAdcBase(uint32_t base)
{
    uint16_t mask = APP_PINTEST_AdcMaskFromBase(base);

    if (mask == 0u)
    {
        return;
    }

    if ((s_adcInitMask & mask) != 0u)
    {
        return;
    }

    ADC_setPrescaler(base, ADC_CLK_DIV_4_0);
    // ADC_setMode(base, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    ADC_setInterruptPulseMode(base, ADC_PULSE_END_OF_CONV);
    ADC_enableConverter(base);
    DEVICE_DELAY_US(1000u);

    s_adcInitMask |= mask;
}

/**
 * @brief 初始化所有 ADC 类型测试点涉及的 ADC 模块。
 */
static void APP_PINTEST_InitAdcPoints(void)
{
    uint16_t i;

    for (i = 0u; i < APP_PINTEST_POINT_COUNT; i++)
    {
        if (s_pinTestPoints[i].type == APP_PINTEST_IO_ADC)
        {
            APP_PINTEST_InitAdcBase(s_pinTestPoints[i].adcBase);
        }
    }
}

/**
 * @brief 读取指定测试点的 ADC 数值（阻塞式）。
 *
 * 流程：
 * - 配置 SOC0 采样通道与采样窗口；
 * - 触发软件采样并等待 ADCINT1；
 * - 读取结果寄存器。
 */
static uint16_t APP_PINTEST_ReadAdc(const APP_PINTEST_Point *point)
{
    if ((point == (const APP_PINTEST_Point *)0) || (point->type != APP_PINTEST_IO_ADC))
    {
        return 0u;
    }

    ADC_setupSOC(point->adcBase,
                 ADC_SOC_NUMBER0,
                 ADC_TRIGGER_SW_ONLY,
                 point->adcChannel,
                 (uint16_t)APP_PINTEST_ADC_ACQPS);

    ADC_setInterruptSource(point->adcBase, ADC_INT_NUMBER1, ADC_SOC_NUMBER0);
    ADC_clearInterruptStatus(point->adcBase, ADC_INT_NUMBER1);
    ADC_enableInterrupt(point->adcBase, ADC_INT_NUMBER1);

    ADC_forceSOC(point->adcBase, ADC_SOC_NUMBER0);

    while (ADC_getInterruptStatus(point->adcBase, ADC_INT_NUMBER1) == false)
    {
    }

    ADC_clearInterruptStatus(point->adcBase, ADC_INT_NUMBER1);

    return ADC_readResult(point->adcResultBase, ADC_SOC_NUMBER0);
}

/**
 * @brief PINTEST 模块一次性初始化。
 *
 * 根据角色进行 GPIO 配置：
 * - 主机侧：GPIO 输入/ADC 模拟输入；
 * - 从机侧：GPIO 输出，默认拉低。
 */
static void APP_PINTEST_InitOnce(void)
{
    uint16_t i;

    if (s_pinTestInitDone != 0u)
    {
        return;
    }

    /* PROTO_ACK_CHxHandle 由 SysCfg 创建，这里不再初始化。 */

    if (APP_PINTEST_POINT_COUNT == 0u)
    {
        s_pinTestInitDone = 1u;
        return;
    }

    ASysCtl_disableDCDC();//关闭22/23引脚的片上DCDC功能

    EALLOW;

    for (i = 0u; i < APP_PINTEST_POINT_COUNT; i++)
    {
        const APP_PINTEST_Point *point = &s_pinTestPoints[i];

        GPIO_setPinConfig(point->pinConfig);
        GPIO_setControllerCore(point->pinId, GPIO_CORE_CPU1);

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
        if (point->type == APP_PINTEST_IO_ADC)
        {
            if (APP_PINTEST_IsAnalogPin(point->pinId) != 0u)
            {
                GPIO_setAnalogMode(point->pinId, GPIO_ANALOG_ENABLED);
            }
            GPIO_setDirectionMode(point->pinId, GPIO_DIR_MODE_IN);
        }
        else
        {
            uint32_t padConfig = GPIO_PIN_TYPE_STD;
#if (APP_PINTEST_INPUT_PULLUP != 0u)
            padConfig |= GPIO_PIN_TYPE_PULLUP;
#endif
            if (APP_PINTEST_IsAnalogPin(point->pinId) != 0u)
            {
                GPIO_setAnalogMode(point->pinId, GPIO_ANALOG_DISABLED);
            }
            GPIO_setPadConfig(point->pinId, padConfig);
            GPIO_setQualificationMode(point->pinId, GPIO_QUAL_SYNC);
            GPIO_setDirectionMode(point->pinId, GPIO_DIR_MODE_IN);
        }
#else
        if (APP_PINTEST_IsAnalogPin(point->pinId) != 0u)
        {
            GPIO_setAnalogMode(point->pinId, GPIO_ANALOG_DISABLED);
        }
        GPIO_setPadConfig(point->pinId, GPIO_PIN_TYPE_STD);
        GPIO_setQualificationMode(point->pinId, GPIO_QUAL_SYNC);
        GPIO_setDirectionMode(point->pinId, GPIO_DIR_MODE_OUT);
#endif
    }

    /* Force GPIO22/23 to digital GPIO for PinTest. */
    GPIO_setAnalogMode(22u, GPIO_ANALOG_DISABLED);
    GPIO_setAnalogMode(23u, GPIO_ANALOG_DISABLED);
#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
    GPIO_setPadConfig(22u, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(23u, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(22u, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(23u, GPIO_QUAL_SYNC);
    GPIO_setDirectionMode(22u, GPIO_DIR_MODE_IN);
    GPIO_setDirectionMode(23u, GPIO_DIR_MODE_IN);
#else
    GPIO_setPadConfig(22u, GPIO_PIN_TYPE_STD);
    GPIO_setPadConfig(23u, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(22u, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(23u, GPIO_QUAL_SYNC);
    GPIO_setDirectionMode(22u, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(23u, GPIO_DIR_MODE_OUT);
#endif

    EDIS;

#if (APP_PROTO_ROLE != APP_PROTO_ROLE_MASTER)
    /* 从机侧全部拉低，避免上电误触发。 */
    for (i = 0u; i < APP_PINTEST_POINT_COUNT; i++)
    {
        GPIO_writePin(s_pinTestPoints[i].pinId, 0u);
    }
#endif

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
    if (APP_PINTEST_HasAdcPoints() != 0u)
    {
        APP_PINTEST_InitAdcPoints();
    }
#endif

    s_pinTestInitDone = 1u;
}

/**
 * @brief 对外初始化封装。
 */
void APP_PINTEST_Init(void)
{
    APP_PINTEST_InitOnce();
}

/**
 * @brief PROTO ACK 回调（主机侧）。
 *
 * 根据通道释放对应 ACK 信号量，供测试任务同步使用。
 */
void APP_PINTEST_OnProtoAck(uint16_t channel)
{
    APP_PINTEST_InitOnce();

    if ((channel == (uint16_t)APP_PROTO_CH0) && (PROTO_ACK_CH0Handle != NULL))
    {
        xSemaphoreGive(PROTO_ACK_CH0Handle);
    }
    else if ((channel == (uint16_t)APP_PROTO_CH1) && (PROTO_ACK_CH1Handle != NULL))
    {
        xSemaphoreGive(PROTO_ACK_CH1Handle);
    }
    else
    {
        /* ignore */
    }
}

/**
 * @brief PROTO WRITE 回调（从机侧）。
 *
 * - 校验 payload；
 * - 解析 pinId 和 level；
 * - 拉高/拉低对应引脚；
 * - ACK 在 PROTO 层发送。
 */
uint16_t APP_PINTEST_OnProtoWrite(uint16_t channel,
                                  const uint16_t *payload,
                                  uint16_t len)
{
    APP_PINTEST_InitOnce();

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
    if (channel != (uint16_t)APP_PINTEST_CHANNEL)
    {
        return 0u;
    }

    if ((payload == (const uint16_t *)0) || (len < (uint16_t)APP_PINTEST_PAYLOAD_LEN))
    {
        APP_LOGW0(TAG, "PinTest payload invalid\n");
        return 0u;
    }

    uint16_t cmd   = (uint16_t)(payload[0] & 0x00FFu);
    uint16_t pinId = (uint16_t)(payload[1] & 0x00FFu);
    uint16_t level = (uint16_t)(payload[2] & 0x00FFu);

    if (cmd != (uint16_t)APP_PINTEST_CMD_SET_LEVEL)
    {
        APP_LOGW1D(TAG, "PinTest cmd %u ignored\n", cmd);
        return 0u;
    }

    const APP_PINTEST_Point *point = APP_PINTEST_FindPoint(pinId);
    if (point == (const APP_PINTEST_Point *)0)
    {
        APP_LOGW1D(TAG, "PinTest unknown pin %u\n", pinId);
        return 0u;
    }

    uint16_t gpioLevel = (level != 0u) ? 1u : 0u;
    GPIO_writePin(point->pinId, gpioLevel);
    APP_LOGI2D(TAG, "Slave set pin %u level %u\n", point->pinId, gpioLevel);

    return 1u;
#else
    (void)channel;
    (void)payload;
    (void)len;
    return 0u;
#endif
}

/**
 * @brief PINTEST 任务入口。
 *
 * 主机侧：按测试点列表逐点发送命令、等待 ACK、读取电平并上报。
 * 从机侧：仅保持就绪（由 PROTO 回调处理命令）。
 */
void PINTEST_Task_Func(void *pvParameters)
{
    (void)pvParameters;

    APP_PINTEST_InitOnce(); //初始化

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
    uint32_t round = 0u;

    APP_LOGI0(TAG, "PinTest master task start\n");

    for (;;)
    {
        APP_PROTO_Ctx *ctx = APP_PROTO_GetChannelCtx(APP_PINTEST_CHANNEL);
        uint16_t i;
        uint16_t failedPins[APP_PINTEST_POINT_COUNT];
        uint16_t failedCount = 0u;

        if ((ctx == (APP_PROTO_Ctx *)0) || (ctx->initialized == 0u))
        {
            APP_LOGW0(TAG, "PROTO not ready\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (APP_PROTO_IsChannelEnabled(APP_PINTEST_CHANNEL) == 0u)
        {
            APP_LOGW1D(TAG, "Channel %u disabled\n", APP_PINTEST_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (APP_PINTEST_POINT_COUNT == 0u)
        {
            APP_LOGW0(TAG, "No PinTest points\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        round += 1u;

        for (i = 0u; i < APP_PINTEST_POINT_COUNT; i++)
        {
            const APP_PINTEST_Point *point = &s_pinTestPoints[i];
            uint16_t payload[APP_PINTEST_PAYLOAD_LEN];
            SemaphoreHandle_t ackSem = (APP_PINTEST_CHANNEL == APP_PROTO_CH0) ?
                                       PROTO_ACK_CH0Handle : PROTO_ACK_CH1Handle;
            uint16_t pointFailed = 0u;

            /* 步骤1：清空可能残留的 ACK 信号，避免将旧 ACK 误认为本次响应 */
            if (ackSem != NULL)
            {
                (void)xSemaphoreTake(ackSem, 0);
            }

            /* 步骤2：组帧 payload（CMD + PIN_ID + LEVEL） */
            payload[0] = (uint16_t)APP_PINTEST_CMD_SET_LEVEL; /* CMD：设置电平 */
            payload[1] = (uint16_t)point->pinId;             /* PIN_ID：目标引脚编号 */
            payload[2] = (uint16_t)APP_PINTEST_LEVEL_HIGH;   /* LEVEL：拉高 */

            APP_LOGI1D(TAG, "Send pin %u HIGH\n", point->pinId);

            /* 步骤3：通过 PROTO 发送 WRITE 帧给从机 */
            if (APP_PROTO_SendWrite(ctx, payload, (uint16_t)APP_PINTEST_PAYLOAD_LEN) == 0u)
            {
                APP_LOGE1D(TAG, "PinTest send fail pin %u\n", point->pinId);
                if (failedCount < APP_PINTEST_POINT_COUNT)
                {
                    failedPins[failedCount] = point->pinId;
                    failedCount++;
                }
                continue;
            }

            /* 步骤4：等待从机 ACK，确认命令已执行 */
            if ((ackSem != NULL) &&
                (xSemaphoreTake(ackSem, pdMS_TO_TICKS(APP_PINTEST_ACK_TIMEOUT_MS)) == pdTRUE))
            {
                /* 步骤5：ACK 到达后延时等待电平稳定 */
                vTaskDelay(pdMS_TO_TICKS(APP_PINTEST_SETTLE_DELAY_MS));

                if (point->type == APP_PINTEST_IO_ADC)
                {
                    /* 步骤6A：ADC 测点—读取模拟值并判定高电平 */
                    uint16_t adcValue = APP_PINTEST_ReadAdc(point);
                    uint16_t pass = (adcValue >= (uint16_t)APP_PINTEST_ADC_HIGH_THRESHOLD) ? 1u : 0u;

                    if (pass != 0u)
                    {
                        APP_LOGI2D(TAG, "ADC pin %u val %u PASS\n", point->pinId, adcValue);
                    }
                    else
                    {
                        APP_LOGE2D(TAG, "ADC pin %u val %u FAIL\n", point->pinId, adcValue);
                        pointFailed = 1u;
                    }
                }
                else
                {
                    /* 步骤6B：GPIO 测点—直接读取数字电平 */
                    uint16_t level = (GPIO_readPin(point->pinId) != 0u) ? 1u : 0u;
                    if (level != 0u)
                    {
                        APP_LOGI2D(TAG, "GPIO pin %u level %u PASS\n", point->pinId, level);
                    }
                    else
                    {
                        APP_LOGE2D(TAG, "GPIO pin %u level %u FAIL\n", point->pinId, level);
                        pointFailed = 1u;
                    }
                }
            }
            else
            {
                APP_LOGE1D(TAG, "PinTest ACK timeout pin %u\n", point->pinId);
                pointFailed = 1u;
            }

            if (pointFailed != 0u)//记录
            {
                if (failedCount < APP_PINTEST_POINT_COUNT)
                {
                    failedPins[failedCount] = point->pinId;
                    failedCount++;
                }
            }

#if (APP_PINTEST_RESET_AFTER != 0u)
            /* 步骤7（可选）：拉回低电平，便于下一轮测试 */
            payload[0] = (uint16_t)APP_PINTEST_CMD_SET_LEVEL;
            payload[1] = (uint16_t)point->pinId;
            payload[2] = (uint16_t)APP_PINTEST_LEVEL_LOW;

            if (ackSem != NULL)
            {
                /* 发送低电平命令前同样清理 ACK */
                (void)xSemaphoreTake(ackSem, 0);
            }

            /* 发送低电平命令并等待 ACK */
            (void)APP_PROTO_SendWrite(ctx, payload, (uint16_t)APP_PINTEST_PAYLOAD_LEN);
            if (ackSem != NULL)
            {
                (void)xSemaphoreTake(ackSem, pdMS_TO_TICKS(APP_PINTEST_ACK_TIMEOUT_MS));
            }
#endif
        }

        if (failedCount == 0u)
        {
            APP_LOGI1D(TAG, "PinTest round %u PASS\n", (uint32_t)round);
        }
        else
        {
            APP_LOGW2D(TAG, "PinTest round %u FAIL count %u\n", (uint32_t)round, (uint32_t)failedCount);
            for (i = 0u; i < failedCount; i++)
            {
                APP_LOGW2D(TAG, "PinTest round %u fail pin %u\n", (uint32_t)round, failedPins[i]);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(APP_PINTEST_PERIOD_MS));
    }
#else
    APP_LOGI0(TAG, "PinTest slave ready\n");

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#endif
}
