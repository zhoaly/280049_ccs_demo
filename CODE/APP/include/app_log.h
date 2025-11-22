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
#include "queue.h"

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
 * @brief 日志写入接口（异步，将消息投递到日志队列）。
 */
BaseType_t APP_LOG_Write(app_log_level_t level, const char *tag, const char *fmt, ...);

/**
 * @brief 日志任务入口函数，使用 SysCfg 配置的任务创建。
 */
void LOG_Task_Func(void *pvParameters);

/* 一组便捷宏，模仿 ESP_LOGX(TAG, ...) 用法 */
#define APP_LOGE(TAG, fmt, ...)  \
    do { \
        if (APP_LOG_GLOBAL_LEVEL >= APP_LOG_ERROR) { \
            APP_LOG_Write(APP_LOG_ERROR, TAG, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGW(TAG, fmt, ...)  \
    do { \
        if (APP_LOG_GLOBAL_LEVEL >= APP_LOG_WARN) { \
            APP_LOG_Write(APP_LOG_WARN, TAG, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGI(TAG, fmt, ...)  \
    do { \
        if (APP_LOG_GLOBAL_LEVEL >= APP_LOG_INFO) { \
            APP_LOG_Write(APP_LOG_INFO, TAG, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGD(TAG, fmt, ...)  \
    do { \
        if (APP_LOG_GLOBAL_LEVEL >= APP_LOG_DEBUG) { \
            APP_LOG_Write(APP_LOG_DEBUG, TAG, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#define APP_LOGV(TAG, fmt, ...)  \
    do { \
        if (APP_LOG_GLOBAL_LEVEL >= APP_LOG_VERBOSE) { \
            APP_LOG_Write(APP_LOG_VERBOSE, TAG, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* APP_LOG_H */
