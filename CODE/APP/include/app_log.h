/**
 * @file app_log.h
 * @brief 基于 SCI 的异步日志模块，提供 ESP_LOG 风格的接口。
 */

#ifndef APP_LOG_H
#define APP_LOG_H

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "c2000_freertos.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 日志级别定义（数值越大，信息越详细）。
 */
typedef enum
{
    APP_LOG_NONE    = 0,
    APP_LOG_ERROR   = 1,
    APP_LOG_WARN    = 2,
    APP_LOG_INFO    = 3,
    APP_LOG_DEBUG   = 4,
    APP_LOG_VERBOSE = 5,
} app_log_level_t;

/**
 * @brief 编译期全局日志级别控制，可通过编译宏覆盖。
 */
#ifndef APP_LOG_GLOBAL_LEVEL
#define APP_LOG_GLOBAL_LEVEL   APP_LOG_DEBUG
#endif

/**
 * @brief 单条日志允许的最大正文长度（不含格式化前缀与结尾换行）。
 */
#ifndef APP_LOG_MESSAGE_MAX_LEN
#define APP_LOG_MESSAGE_MAX_LEN    (96U)
#endif

/**
 * @brief 日志队列默认发送等待时间（毫秒）。
 */
#ifndef APP_LOG_QUEUE_TIMEOUT_MS
#define APP_LOG_QUEUE_TIMEOUT_MS   (5U)
#endif

/**
 * @brief 每条日志允许的最大参数个数。
 */
#ifndef APP_LOG_MAX_ARGS
#define APP_LOG_MAX_ARGS   (2U)
#endif

/**
 * @brief 日志参数类型。
 */
typedef enum
{
    APP_LOG_ARG_INT = 0,
    APP_LOG_ARG_UINT,
    APP_LOG_ARG_HEX,
    APP_LOG_ARG_FLOAT,
    APP_LOG_ARG_STR,
} APP_LogArgType;

/**
 * @brief 日志参数打包结构。
 */
typedef struct
{
    APP_LogArgType type;
    union
    {
        int32_t      i32;
        uint32_t     u32;
        uint32_t     hex;
        float        f32;
        const char  *str;
    } v;
} APP_LogArg;

/**
 * @brief 队列传递的日志消息体：保存原始格式串与参数列表。
 */
typedef struct
{
    app_log_level_t level;
    const char     *tag;
    const char     *fmt;
    uint8_t         argCount;
    APP_LogArg      args[APP_LOG_MAX_ARGS];
} APP_LogMessage;

/**
 * @brief 使用 SysCfg 生成的日志队列句柄（在其他文件中定义）。
 */
extern QueueHandle_t APP_LOG_QueueHandle;

/**
 * @brief 设置运行时全局日志级别。
 */
void APP_LOG_SetLevel(app_log_level_t level);

/**
 * @brief 获取当前运行时日志级别。
 */
app_log_level_t APP_LOG_GetLevel(void);

/**
 * @brief 已打包参数的日志写入接口（异步，将消息投递到日志队列）。
 */
BaseType_t APP_LOG_WriteArgs(app_log_level_t level,
                             const char *tag,
                             const char *fmt,
                             uint8_t argCount,
                             const APP_LogArg *args);

/**
 * @brief 日志任务入口函数，使用 SysCfg 配置的任务创建。
 */
void LOG_Task_Func(void *pvParameters);

//暂时实现以下四个宏,后续使用时可以自行排列组合
#define APP_LOGX0(LEVEL, TAG, fmt)                                      \
    do {                                                                \
        if (APP_LOG_GLOBAL_LEVEL >= (LEVEL)) {                          \
            APP_LOG_WriteArgs((LEVEL), TAG, fmt, 0U, NULL);             \
        }                                                               \
    } while (0)

#define APP_LOGX1D(LEVEL, TAG, fmt, a1)                                  \
    do {                                                                \
        if (APP_LOG_GLOBAL_LEVEL >= (LEVEL)) {                          \
            APP_LogArg _args[1];                                        \
            _args[0].type  = APP_LOG_ARG_INT;                           \
            _args[0].v.i32 = (int32_t)(a1);                             \
            APP_LOG_WriteArgs((LEVEL), TAG, fmt, 1U, _args);            \
        }                                                               \
    } while (0)

// 1) 输出 1 个字符串参数
#define APP_LOGX1S(LEVEL, TAG, fmt, s1)                                  \
    do {                                                                 \
        if (APP_LOG_GLOBAL_LEVEL >= (LEVEL)) {                           \
            APP_LogArg _args[1];                                         \
            _args[0].type  = APP_LOG_ARG_STR;                            \
            _args[0].v.str = (const char *)(s1);                         \
            APP_LOG_WriteArgs((LEVEL), TAG, fmt, 1U, _args);             \
        }                                                                \
    } while (0)

// 2) 输出 1 个 float32 参数
#define APP_LOGX1F(LEVEL, TAG, fmt, f1)                                  \
    do {                                                                 \
        if (APP_LOG_GLOBAL_LEVEL >= (LEVEL)) {                           \
            APP_LogArg _args[1];                                         \
            _args[0].type  = APP_LOG_ARG_F32;                            \
            _args[0].v.f32 = (float)(f1);                                \
            APP_LOG_WriteArgs((LEVEL), TAG, fmt, 1U, _args);             \
        }                                                                \
    } while (0)


#define APP_LOGX2D(LEVEL, TAG, fmt, a1, a2)                              \
    do {                                                                \
        if (APP_LOG_GLOBAL_LEVEL >= (LEVEL)) {                          \
            APP_LogArg _args[2] ;                                       \
            _args[0].type  = APP_LOG_ARG_INT;                           \
            _args[0].v.i32 = (int32_t)(a1);                             \
            _args[1].type  = APP_LOG_ARG_INT;                           \
            _args[1].v.i32 = (int32_t)(a2);                             \
            APP_LOG_WriteArgs((LEVEL), TAG, fmt, 2U, _args);            \
        }                                                               \
    } while (0)

/* 按级别派生的便捷宏 */
#define APP_LOGE0(TAG, fmt)                 APP_LOGX0(APP_LOG_ERROR,   TAG, fmt)
#define APP_LOGE1D(TAG, fmt, a1)             APP_LOGX1D(APP_LOG_ERROR,   TAG, fmt, a1)
#define APP_LOGE2D(TAG, fmt, a1, a2)         APP_LOGX2D(APP_LOG_ERROR,   TAG, fmt, a1, a2)

#define APP_LOGW0(TAG, fmt)                 APP_LOGX0(APP_LOG_WARN,    TAG, fmt)
#define APP_LOGW1D(TAG, fmt, a1)             APP_LOGX1D(APP_LOG_WARN,    TAG, fmt, a1)
#define APP_LOGW2D(TAG, fmt, a1, a2)         APP_LOGX2D(APP_LOG_WARN,    TAG, fmt, a1, a2)

#define APP_LOGI0(TAG, fmt)                 APP_LOGX0(APP_LOG_INFO,    TAG, fmt)
#define APP_LOGI1D(TAG, fmt, a1)             APP_LOGX1D(APP_LOG_INFO,    TAG, fmt, a1)
#define APP_LOGI2D(TAG, fmt, a1, a2)         APP_LOGX2D(APP_LOG_INFO,    TAG, fmt, a1, a2)
#define APP_LOGI1S(TAG, fmt, s1)             APP_LOGX1S(APP_LOG_INFO,    TAG, fmt, s1)
#define APP_LOGI1F(TAG, fmt, f1)             APP_LOGX1F(APP_LOG_INFO,    TAG, fmt, f1)

#define APP_LOGD0(TAG, fmt)                 APP_LOGX0(APP_LOG_DEBUG,   TAG, fmt)
#define APP_LOGD1D(TAG, fmt, a1)             APP_LOGX1D(APP_LOG_DEBUG,   TAG, fmt, a1)
#define APP_LOGD2D(TAG, fmt, a1, a2)         APP_LOGX2D(APP_LOG_DEBUG,   TAG, fmt, a1, a2)

#define APP_LOGV0(TAG, fmt)                 APP_LOGX0(APP_LOG_VERBOSE, TAG, fmt)
#define APP_LOGV1D(TAG, fmt, a1)             APP_LOGX1D(APP_LOG_VERBOSE, TAG, fmt, a1)
#define APP_LOGV2D(TAG, fmt, a1, a2)         APP_LOGX2D(APP_LOG_VERBOSE, TAG, fmt, a1, a2)

#ifdef __cplusplus
}
#endif

#endif /* APP_LOG_H */
