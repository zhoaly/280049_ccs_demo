/* app_pintest.c */
#include "__INCLUDE.h"

static const char *TAG = "pintest";

static uint16_t s_pinTestInitDone = 0u;

// SemaphoreHandle_t PROTO_ACK_CH0Handle = NULL;
// SemaphoreHandle_t PROTO_ACK_CH1Handle = NULL;

// static StaticSemaphore_t s_protoAckCh0Buffer;
// static StaticSemaphore_t s_protoAckCh1Buffer;

static void APP_PINTEST_InitOnce(void)
{
    if (s_pinTestInitDone != 0u)
    {
        return;
    }

    if (PROTO_ACK_CH0Handle == NULL)
    {
        PROTO_ACK_CH0Handle = xSemaphoreCreateBinaryStatic(&s_protoAckCh0Buffer);
    }

    if (PROTO_ACK_CH1Handle == NULL)
    {
        PROTO_ACK_CH1Handle = xSemaphoreCreateBinaryStatic(&s_protoAckCh1Buffer);
    }

    EALLOW;
    GPIO_setPinConfig(APP_PINTEST_GPIO_PIN_CONFIG);

    uint32_t padConfig = GPIO_PIN_TYPE_STD;
#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
#if (APP_PINTEST_INPUT_PULLUP != 0u)
    padConfig |= GPIO_PIN_TYPE_PULLUP;
#endif
#endif
    GPIO_setPadConfig(APP_PINTEST_GPIO_PIN, padConfig);
    GPIO_setQualificationMode(APP_PINTEST_GPIO_PIN, GPIO_QUAL_SYNC);
    GPIO_setControllerCore(APP_PINTEST_GPIO_PIN, GPIO_CORE_CPU1);

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
    GPIO_setDirectionMode(APP_PINTEST_GPIO_PIN, GPIO_DIR_MODE_IN);
#else
    GPIO_setDirectionMode(APP_PINTEST_GPIO_PIN, GPIO_DIR_MODE_OUT);
#endif
    EDIS;

#if (APP_PROTO_ROLE != APP_PROTO_ROLE_MASTER)
    GPIO_writePin(APP_PINTEST_GPIO_PIN, 0u);
#endif

    s_pinTestInitDone = 1u;
}

void APP_PINTEST_Init(void)
{
    APP_PINTEST_InitOnce();
}

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

    if (pinId != (uint16_t)APP_PINTEST_PIN_ID)
    {
        APP_LOGW2D(TAG, "PinTest pinId %u != %u\n", pinId, APP_PINTEST_PIN_ID);
        return 0u;
    }

    uint16_t gpioLevel = (level != 0u) ? 1u : 0u;
    GPIO_writePin(APP_PINTEST_GPIO_PIN, gpioLevel);
    APP_LOGI2D(TAG, "Slave set pin %u level %u\n", APP_PINTEST_GPIO_PIN, gpioLevel);

    return 1u;
#else
    (void)channel;
    (void)payload;
    (void)len;
    return 0u;
#endif
}

void APP_PINTEST_Task_Func(void *pvParameters)
{
    (void)pvParameters;

    APP_PINTEST_InitOnce();

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
    APP_LOGI0(TAG, "PinTest master task start\n");

    for (;;)
    {
        APP_PROTO_Ctx *ctx = APP_PROTO_GetChannelCtx(APP_PINTEST_CHANNEL);

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

        uint16_t payload[APP_PINTEST_PAYLOAD_LEN];
        payload[0] = (uint16_t)APP_PINTEST_CMD_SET_LEVEL;
        payload[1] = (uint16_t)APP_PINTEST_PIN_ID;
        payload[2] = (uint16_t)APP_PINTEST_LEVEL_HIGH;

        SemaphoreHandle_t ackSem = (APP_PINTEST_CHANNEL == APP_PROTO_CH0) ?
                                   PROTO_ACK_CH0Handle : PROTO_ACK_CH1Handle;

        if (ackSem != NULL)
        {
            (void)xSemaphoreTake(ackSem, 0);
        }

        APP_LOGI1D(TAG, "Send pin set HIGH on ch %u\n", APP_PINTEST_CHANNEL);

        if (APP_PROTO_SendWrite(ctx, payload, (uint16_t)APP_PINTEST_PAYLOAD_LEN) == 0u)
        {
            APP_LOGE0(TAG, "PinTest send failed\n");
        }
        else
        {
            if ((ackSem != NULL) &&
                (xSemaphoreTake(ackSem, pdMS_TO_TICKS(APP_PINTEST_ACK_TIMEOUT_MS)) == pdTRUE))
            {
                vTaskDelay(pdMS_TO_TICKS(APP_PINTEST_SETTLE_DELAY_MS));

                uint16_t level = (GPIO_readPin(APP_PINTEST_GPIO_PIN) != 0u) ? 1u : 0u;
                if (level != 0u)
                {
                    APP_LOGI2D(TAG, "Master read pin %u level %u (PASS)\n",
                               APP_PINTEST_GPIO_PIN, level);
                }
                else
                {
                    APP_LOGE2D(TAG, "Master read pin %u level %u (FAIL)\n",
                               APP_PINTEST_GPIO_PIN, level);
                }
            }
            else
            {
                APP_LOGE0(TAG, "PinTest ACK timeout\n");
            }
        }

#if (APP_PINTEST_RESET_AFTER != 0u)
        payload[0] = (uint16_t)APP_PINTEST_CMD_SET_LEVEL;
        payload[1] = (uint16_t)APP_PINTEST_PIN_ID;
        payload[2] = (uint16_t)APP_PINTEST_LEVEL_LOW;

        if (ackSem != NULL)
        {
            (void)xSemaphoreTake(ackSem, 0);
        }

        (void)APP_PROTO_SendWrite(ctx, payload, (uint16_t)APP_PINTEST_PAYLOAD_LEN);
        if (ackSem != NULL)
        {
            (void)xSemaphoreTake(ackSem, pdMS_TO_TICKS(APP_PINTEST_ACK_TIMEOUT_MS));
        }
#endif

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
