/**
 * @file app_log.c
 * @brief APP 层异步日志实现，利用现有 SCI 驱动输出日志。
 */

#include "app_log.h"
#include "drv_sci.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 运行时日志级别，初始为编译期默认值 */
static volatile app_log_level_t s_appLogRuntimeLevel = APP_LOG_GLOBAL_LEVEL;

/* 日志消息结构体：队列中传递格式化后的正文与元数据。 */
typedef struct
{
    app_log_level_t level;
    const char     *tag;
    char            message[APP_LOG_MESSAGE_MAX_LEN];
} APP_LogMessage;

/* 一次写入 TX 缓冲区的最大切片长度。 */
#define APP_LOG_TX_SLICE_LEN   (64U)

static const char *APP_LOG_levelStr(app_log_level_t level);
static void APP_LOG_flushString(const char *str, size_t len);
static void APP_LOG_outputLine(app_log_level_t level, const char *tag, const char *payload);

void APP_LOG_SetLevel(app_log_level_t level)
{
    s_appLogRuntimeLevel = level;
}

app_log_level_t APP_LOG_GetLevel(void)
{
    return s_appLogRuntimeLevel;
}

BaseType_t APP_LOG_Write(app_log_level_t level, const char *tag, const char *fmt, ...)
{
    APP_LogMessage logMessage;
    BaseType_t ret;
    va_list args;

    if (level > s_appLogRuntimeLevel)
    {
        return pdFAIL;
    }

    if (tag == NULL)
    {
        tag = "APP";
    }

    logMessage.level = level;
    logMessage.tag   = tag;

    va_start(args, fmt);
    (void)vsnprintf(logMessage.message, sizeof(logMessage.message), fmt, args);
    va_end(args);
    logMessage.message[APP_LOG_MESSAGE_MAX_LEN - 1U] = '\0';

    if (APP_LOG_QueueHandle != NULL)
    {
        ret = xQueueSend(APP_LOG_QueueHandle,
                         &logMessage,
                         pdMS_TO_TICKS(APP_LOG_QUEUE_TIMEOUT_MS));
        if (ret == pdPASS)
        {
            return ret;
        }
    }

    /* 队列不可用或发送失败时，直接同步输出，避免丢日志。 */
    APP_LOG_outputLine(logMessage.level, logMessage.tag, logMessage.message);
    return pdPASS;
}

void APP_LOG_Task(void *pvParameters)
{
    (void)pvParameters;
    APP_LogMessage logMessage;

    for (;;)
    {
        if ((APP_LOG_QueueHandle != NULL) &&
            (xQueueReceive(APP_LOG_QueueHandle, &logMessage, portMAX_DELAY) == pdPASS))
        {
            APP_LOG_outputLine(logMessage.level, logMessage.tag, logMessage.message);
        }
        else
        {
            /* 队列尚未就绪时保持低功耗等待。 */
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

static const char *APP_LOG_levelStr(app_log_level_t level)
{
    switch (level)
    {
        case APP_LOG_ERROR:   return "E";
        case APP_LOG_WARN:    return "W";
        case APP_LOG_INFO:    return "I";
        case APP_LOG_DEBUG:   return "D";
        case APP_LOG_VERBOSE: return "V";
        default:              return "?";
    }
}

static void APP_LOG_flushString(const char *str, size_t len)
{
    uint16_t txBuf[APP_LOG_TX_SLICE_LEN];
    size_t offset = 0U;

    while (offset < len)
    {
        size_t sliceLen = len - offset;
        size_t i;

        if (sliceLen > APP_LOG_TX_SLICE_LEN)
        {
            sliceLen = APP_LOG_TX_SLICE_LEN;
        }

        for (i = 0U; i < sliceLen; i++)
        {
            txBuf[i] = (uint16_t)str[offset + i];
        }

        (void)DRV_SCI0_TxWriteBytes(txBuf, (uint16_t)sliceLen);
        offset += sliceLen;
    }
}

static void APP_LOG_outputLine(app_log_level_t level, const char *tag, const char *payload)
{
    char buffer[APP_LOG_MESSAGE_MAX_LEN + 24U];
    int written;
    size_t len;

    if (tag == NULL)
    {
        tag = "APP";
    }

    written = snprintf(buffer,
                       sizeof(buffer),
                       "[%s][%s] %s\r\n",
                       APP_LOG_levelStr(level),
                       tag,
                       payload);

    if (written < 0)
    {
        return;
    }

    len = (size_t)written;
    if (len >= sizeof(buffer))
    {
        len = sizeof(buffer) - 1U;
        buffer[len] = '\0';
    }

    APP_LOG_flushString(buffer, len);
}
