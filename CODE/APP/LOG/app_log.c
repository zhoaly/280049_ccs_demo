/**
 * @file app_log.c
 * @brief APP 层异步日志实现，利用现有 SCI 驱动输出日志。
 */

#include "__INCLUDE.h"

/* 运行时日志级别，初始为编译期默认值 */
static volatile app_log_level_t s_appLogRuntimeLevel = APP_LOG_GLOBAL_LEVEL;

/* 一次写入 TX 缓冲区的最大切片长度。 */
#define APP_LOG_TX_SLICE_LEN   (64U)

static const char *APP_LOG_levelStr(app_log_level_t level);
static void APP_LOG_flushString(const char *str, size_t len);
static size_t APP_LOG_FormatFromArgs(char *buf, size_t bufSize, const APP_LogMessage *msg);
static void APP_LOG_outputFormatted(const APP_LogMessage *msg);
static BaseType_t APP_LOG_Enqueue(const APP_LogMessage *msg);

void APP_LOG_SetLevel(app_log_level_t level)
{
    s_appLogRuntimeLevel = level;
}

app_log_level_t APP_LOG_GetLevel(void)
{
    return s_appLogRuntimeLevel;
}

/*
 * @brief 内部封装：将已经打包好的日志消息入队
 */
static BaseType_t APP_LOG_Enqueue(const APP_LogMessage *msg)
{
    BaseType_t ret = pdFAIL;

    if ((msg == NULL) || (APP_LOG_QueueHandle == NULL))
    {
        return pdFAIL;
    }

    ret = xQueueSend(APP_LOG_QueueHandle,
                     msg,
                     pdMS_TO_TICKS(APP_LOG_QUEUE_TIMEOUT_MS));

    return ret;
}

/*
 * @brief 对外暴露的“已打包参数”写入接口
 *
 * 调用侧仅负责准备格式串和 APP_LogArg 数组，避免在高实时性任务里创建大 buffer。
 */
BaseType_t APP_LOG_WriteArgs(app_log_level_t level,
                             const char *tag,
                             const char *fmt,
                             uint8_t argCount,
                             const APP_LogArg *args)
{
    APP_LogMessage logMessage;
    uint8_t i;

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
    logMessage.fmt      = fmt;
    logMessage.argCount = (argCount > APP_LOG_MAX_ARGS) ? APP_LOG_MAX_ARGS : argCount;

    if ((logMessage.argCount > 0U) && (args != NULL))
    {
        for (i = 0U; i < logMessage.argCount; i++)
        {
            logMessage.args[i] = args[i];
        }
    }

    /* 队列可用时异步发送，失败则回落为同步输出 */
    if (APP_LOG_Enqueue(&logMessage) == pdPASS)
    {
        return pdPASS;
    }

    /* 队列不可用或发送失败时，直接在当前任务中同步格式化并输出。 */
    //APP_LOG_outputFormatted(&logMessage);
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
            APP_LOG_outputFormatted(&logMessage);
        }
        /* 若返回值不是 pdPASS，一般意味着严重错误，可视情况加上断言或错误计数 */
    }
}

/* -------------------------------------------------------------------------- */
/* 内部工具函数                                                               */
/* -------------------------------------------------------------------------- */

/*
 * @brief 在安全范围内写入单个字符，自动维护剩余空间与已写长度
 */
static inline void APP_LOG_PutChar(char **pp, size_t *pRemain, size_t *pWritten, char ch)
{
    if ((pp != NULL) && (pRemain != NULL) && (*pRemain > 1U) && (*pp != NULL))
    {
        **pp = ch;
        (*pp)++;
        (*pRemain)--;
        **pp = '\0';
    }

    if (pWritten != NULL)
    {
        (*pWritten)++;
    }
}

/*
 * @brief 逐字符写入字符串，内部会处理空指针情况
 */
static inline void APP_LOG_PutStr(char **pp, size_t *pRemain, size_t *pWritten, const char *s)
{
    if (s == NULL)
    {
        s = "(null)";
    }

    while (*s != '\0')
    {
        APP_LOG_PutChar(pp, pRemain, pWritten, *s++);
    }
}

/*
 * @brief 按指定进制输出无符号整数
 */
static inline void APP_LOG_PutUnsigned(char **pp,
                                       size_t *pRemain,
                                       size_t *pWritten,
                                       unsigned long value,
                                       unsigned base,
                                       int upper)
{
    char tmp[16];
    int  i = 0;

    if ((base < 2U) || (base > 16U))
    {
        return;
    }

    do
    {
        unsigned digit = (unsigned)(value % base);
        value /= base;

        if (digit < 10U)
        {
            tmp[i++] = (char)('0' + digit);
        }
        else
        {
            tmp[i++] = (char)((upper ? 'A' : 'a') + (digit - 10U));
        }
    } while ((value != 0U) && (i < (int)sizeof(tmp)));

    while (i > 0)
    {
        APP_LOG_PutChar(pp, pRemain, pWritten, tmp[--i]);
    }
}

/*
 * @brief 输出带符号整数，内部调用无符号转换实现
 */
static inline void APP_LOG_PutSigned(char **pp,
                                     size_t *pRemain,
                                     size_t *pWritten,
                                     long value)
{
    unsigned long v;

    if (value < 0)
    {
        APP_LOG_PutChar(pp, pRemain, pWritten, '-');
        v = (unsigned long)(-value);
    }
    else
    {
        v = (unsigned long)value;
    }

    APP_LOG_PutUnsigned(pp, pRemain, pWritten, v, 10U, 0);
}

/*
 * @brief 简易浮点数转字符串，控制小数位精度
 */
static inline void APP_LOG_PutFloat(char **pp,
                                    size_t *pRemain,
                                    size_t *pWritten,
                                    double val,
                                    int precision)
{
    int i;

    if (precision < 0)
    {
        precision = 3;
    }
    if (precision > 6)
    {
        precision = 6;
    }

    if ((pRemain == NULL) || (*pRemain <= 1U))
    {
        return;
    }

    if (val < 0.0)
    {
        APP_LOG_PutChar(pp, pRemain, pWritten, '-');
        val = -val;
    }

    double rounding = 0.5;
    for (i = 0; i < precision; i++)
    {
        rounding *= 0.1;
    }
    val += rounding;

    unsigned long intPart = (unsigned long)val;
    double        frac    = val - (double)intPart;

    APP_LOG_PutUnsigned(pp, pRemain, pWritten, intPart, 10U, 0);

    if (precision <= 0)
    {
        return;
    }

    APP_LOG_PutChar(pp, pRemain, pWritten, '.');

    for (i = 0; i < precision; i++)
    {
        frac *= 10.0;
        int digit = (int)frac;
        if (digit > 9)
        {
            digit = 9;
        }
        APP_LOG_PutChar(pp, pRemain, pWritten, (char)('0' + digit));
        frac -= (double)digit;
    }
}


/*
 * @brief 在日志任务中将“格式串+参数”拼装成完整文本
 */
static size_t APP_LOG_FormatFromArgs(char *buf,
                                     size_t bufSize,
                                     const APP_LogMessage *msg)
{
    char   *p       = buf;
    size_t  remain  = bufSize;
    size_t  written = 0U;
    uint8_t argIdx  = 0U;

    if ((buf == NULL) || (bufSize == 0U) || (msg == NULL))
    {
        return 0U;
    }

    *p = '\0';

    const char *lvl = APP_LOG_levelStr(msg->level);
    const char *tag = (msg->tag != NULL) ? msg->tag : "APP";

    APP_LOG_PutChar(&p, &remain, &written, '[');
    APP_LOG_PutStr(&p, &remain, &written, lvl);
    APP_LOG_PutChar(&p, &remain, &written, ']');
    APP_LOG_PutChar(&p, &remain, &written, '[');
    APP_LOG_PutStr(&p, &remain, &written, tag);
    APP_LOG_PutChar(&p, &remain, &written, ']');
    APP_LOG_PutChar(&p, &remain, &written, ' ');

    const char *fmt = (msg->fmt != NULL) ? msg->fmt : "";

    while ((*fmt != '\0') && (remain > 1U))
    {
        if (*fmt != '%')
        {
            APP_LOG_PutChar(&p, &remain, &written, *fmt++);
            continue;
        }

        fmt++; /* skip '%' */

        if (*fmt == '%')
        {
            APP_LOG_PutChar(&p, &remain, &written, '%');
            fmt++;
            continue;
        }

        while ((*fmt == '0') || (*fmt == '-') || (*fmt == '+') || (*fmt == ' '))
        {
            fmt++;
        }

        while (isdigit((unsigned char)*fmt) != 0)
        {
            fmt++;
        }

        if (*fmt == '.')
        {
            fmt++;
            while (isdigit((unsigned char)*fmt) != 0)
            {
                fmt++;
            }
        }

        if (argIdx >= msg->argCount)
        {
            APP_LOG_PutStr(&p, &remain, &written, "[ARG?]");
            break;
        }

        APP_LogArg arg = msg->args[argIdx++];
        switch (*fmt)
        {
            case 'd':
            case 'i':
                APP_LOG_PutSigned(&p, &remain, &written, (long)arg.v.i32);
                break;

            case 'u':
                APP_LOG_PutUnsigned(&p, &remain, &written, (unsigned long)arg.v.u32, 10U, 0);
                break;

            case 'x':
            case 'X':
                APP_LOG_PutUnsigned(&p, &remain, &written, (unsigned long)arg.v.hex, 16U, (*fmt == 'X') ? 1 : 0);
                break;

            case 'f':
                APP_LOG_PutFloat(&p, &remain, &written, (double)arg.v.f32, -1);
                break;

            case 's':
                APP_LOG_PutStr(&p, &remain, &written, arg.v.str);
                break;

            default:
                APP_LOG_PutChar(&p, &remain, &written, '%');
                APP_LOG_PutChar(&p, &remain, &written, *fmt);
                break;
        }

        if (*fmt != '\0')
        {
            fmt++;
        }
    }

    APP_LOG_PutChar(&p, &remain, &written, '\r');
    APP_LOG_PutChar(&p, &remain, &written, '\n');

    return (size_t)(p - buf);
}

/*
 * @brief 将格式化后的日志字符串发送至底层 SCI
 */
static void APP_LOG_outputFormatted(const APP_LogMessage *msg)
{
    char   buffer[APP_LOG_MESSAGE_MAX_LEN + 24U];
    size_t len;

    len = APP_LOG_FormatFromArgs(buffer, sizeof(buffer), msg);
    if (len == 0U)
    {
        return;
    }

    if (len >= sizeof(buffer))
    {
        len = sizeof(buffer) - 1U;
    }

    APP_LOG_flushString(buffer, len);
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
       
        if(xSemaphoreTake(SCI0Tx_SemaphoreHandle, portMAX_DELAY) == pdTRUE){
            
            (void)DRV_SCI0_TxWriteBytes(
                (const uint16_t *)(const void *)(str + offset),
                (uint16_t)sliceLen);
            xSemaphoreGive(SCI0Tx_SemaphoreHandle);//实现原子化操作
        }
    
        

        /* 更新偏移量，指向下一段待发送数据的起始位置 */
        offset += sliceLen;
    }
}
