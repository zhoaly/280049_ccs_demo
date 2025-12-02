/**
 * @file app_log_vsnprintf.h
 * @brief 轻量级格式化输出函数（自实现 vsnprintf），适用于 C2000 + FreeRTOS 等嵌入式场景。
 *
 * 功能特性：
 *  - 不依赖 TI RTS 的 printf/vsnprintf 实现；
 *  - 无动态内存分配；
 *  - 栈占用远小于标准库版本；
 *  - 线程安全（无静态可变全局变量）。
 *
 * 支持的格式：
 *  - 普通字符；
 *  - %%          ：输出一个 '%'；
 *  - %d, %i      ：有符号十进制整数（int）；
 *  - %u          ：无符号十进制整数（unsigned int）；
 *  - %x, %X      ：无符号十六进制整数（unsigned int）；
 *  - %s          ：字符串（const char *，NULL 时输出 "(null)"）；
 *  - %f, %.nf    ：浮点数（float/double，n 为小数位数，0~6，缺省为 3）。
 *
 * 不支持：
 *  - 长度修饰符（h, l, ll 等）；
 *  - 复杂对齐/填充/宽度控制；
 *  - 科学计数法等高级浮点格式。
 */

#ifndef APP_VSNPRINTF_H_
#define APP_VSNPRINTF_H_

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 轻量版 vsnprintf 实现。
 *
 * @param[out] buf      输出缓冲区指针。
 * @param[in]  bufSize  缓冲区大小（字节数，包含结尾 '\0'）。
 * @param[in]  fmt      格式化字符串。
 * @param[in]  ap       va_list，可变参数列表。
 *
 * @return 返回理论输出的字符数（不包含结尾 '\0'），即使发生截断也按完整输出长度计算。
 *         若 buf 或 fmt 为 NULL，或 bufSize 为 0，则返回 0。
 */
int APP_LOG_vsnprintf(char *buf,
                      size_t bufSize,
                      const char *fmt,
                      va_list ap);

/**
 * @brief 轻量版 snprintf 封装，内部调用 APP_LOG_vsnprintf。
 *
 * @param[out] buf      输出缓冲区指针。
 * @param[in]  bufSize  缓冲区大小（字节数，包含结尾 '\0'）。
 * @param[in]  fmt      格式化字符串。
 * @param[in]  ...      可变参数。
 *
 * @return 返回理论输出的字符数（不包含结尾 '\0'），即使发生截断也按完整输出长度计算。
 */
int APP_LOG_snprintf(char *buf,
                     size_t bufSize,
                     const char *fmt,
                     ...);

#ifdef __cplusplus
}
#endif

#endif /* APP_VSNPRINTF_H_ */
