/**
 * @file app_log.c
 * @brief APP 层异步日志实现，利用现有 SCI 驱动输出日志。
 */

#include "app_log.h"
#include "drv_sci.h"
#include "app_log_vsnprintf.h"  

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
    //vsnprintf 会根据 fmt 和 args，把格式化后的字符串写入 logMessage.message
    (void)vsnprintf(logMessage.message, sizeof(logMessage.message), fmt, args);//
    // (void)APP_LOG_vsnprintf(logMessage.message, sizeof(logMessage.message), fmt, args);//
    va_end(args);
    logMessage.message[APP_LOG_MESSAGE_MAX_LEN - 1U] = '\0';

    if (APP_LOG_QueueHandle != NULL)
    {
        // ret = pdPASS;
        ret = xQueueSend(APP_LOG_QueueHandle,
                         &logMessage,
                         pdMS_TO_TICKS(APP_LOG_QUEUE_TIMEOUT_MS));
        if (ret == pdPASS)
        {
            return ret;//队列可用时,直接在这里返回
        }
    }

    /* 队列不可用或发送失败时，直接同步输出，避免丢日志。 */
    APP_LOG_outputLine(logMessage.level, logMessage.tag, logMessage.message);
    return pdPASS;
}

void LOG_Task_Func(void *pvParameters)
{
    (void)pvParameters;
    APP_LogMessage logMessage;

    while (APP_LOG_QueueHandle == NULL)
    {
        /* 队列还没创建好，适当让出 CPU（比如 1ms） */
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    QueueHandle_t queue = (QueueHandle_t)APP_LOG_QueueHandle;

    for (;;)
    {
        /* 阻塞等待一条日志消息，不需要额外延时 */
        if (xQueueReceive(queue, &logMessage, portMAX_DELAY) == pdPASS)
        {
            APP_LOG_outputLine(logMessage.level,
                               logMessage.tag,
                               logMessage.message);
        }
        /* 若返回值不是 pdPASS，一般意味着严重错误，可视情况加上断言或错误计数 */
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

/**
 * @brief 通过 SCI 发送一整段字符串（按固定片长切片发送）。
 *
 * 该函数不直接逐字节写硬件，而是：
 * 1. 将输入字符串按 APP_LOG_TX_SLICE_LEN 为最大长度分片；
 * 2. 将每一片拷贝到临时 uint16_t 缓冲区（低 8 位为有效字符）；
 * 3. 调用 DRV_SCI0_TxWriteBytes 写入 SCI 发送环形缓冲区，
 *    由底层 SCI TX 中断机制完成最终发送。
 *
 * @param[in] str 待发送的字符串指针
 * @param[in] len 字符串长度（不含 '\0'）
 */
static void APP_LOG_flushString(const char *str, size_t len)
{
    size_t offset = 0U;                    /**< 已经发送/处理的字符串偏移量 */

    /* 只要还有未处理的数据，就持续循环发送 */
    while (offset < len)
    {
        size_t sliceLen = len - offset;    /**< 本次准备发送的数据长度（先按剩余长度计算） */


        /* 若剩余长度大于单次发送上限，则本次仅发送 APP_LOG_TX_SLICE_LEN 个字符 */
        if (sliceLen > APP_LOG_TX_SLICE_LEN)
        {
            sliceLen = APP_LOG_TX_SLICE_LEN;
        }

       /* 直接把 char 缓冲区视为 uint16_t 缓冲区使用，避免逐字节拷贝 */
        (void)DRV_SCI0_TxWriteBytes(
            (const uint16_t *)(const void *)(str + offset),
            (uint16_t)sliceLen
        );

        /* 更新偏移量，指向下一段待发送数据的起始位置 */
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
    // 统一报告格式:"[<level_str>][<tag>] <payload>\r\n"
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




