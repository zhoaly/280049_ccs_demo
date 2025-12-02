/**
 * @file app_vsnprintf.c
 * @brief 轻量级 vsnprintf/snprintf 实现，适用于 C2000 等资源受限场景。
 */

#include "app_log_vsnprintf.h"

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* 内部工具函数                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief 向缓冲区追加一个字符，并维护已写长度（即使发生截断，也会增加计数）。
 */
static inline void APP_PutChar(char **pp, size_t *pRemain, int *pWritten, char ch)
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

/**
 * @brief 向缓冲区追加字符串。
 */
static inline void APP_PutStr(char **pp, size_t *pRemain, int *pWritten, const char *s)
{
    if (s == NULL)
    {
        s = "(null)";
    }

    while (*s != '\0')
    {
        APP_PutChar(pp, pRemain, pWritten, *s++);
    }
}

/**
 * @brief 输出无符号整数（支持任意 2~16 进制）。
 */
static inline void APP_PutUnsigned(char **pp,
                            size_t *pRemain,
                            int *pWritten,
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
        APP_PutChar(pp, pRemain, pWritten, tmp[--i]);
    }
}

/**
 * @brief 输出有符号十进制整数。
 */
static inline void APP_PutSigned(char **pp,
                          size_t *pRemain,
                          int *pWritten,
                          long value)
{
    unsigned long v;

    if (value < 0)
    {
        APP_PutChar(pp, pRemain, pWritten, '-');
        v = (unsigned long)(-value);
    }
    else
    {
        v = (unsigned long)value;
    }

    APP_PutUnsigned(pp, pRemain, pWritten, v, 10U, 0);
}

/**
 * @brief 输出浮点数（简单实现，仅用于日志，非高精度数值计算）。
 *
 * @param precision 小数位数，<0 表示使用默认（3），最大限制为 6。
 */
static inline void APP_PutFloat(char **pp,
                         size_t *pRemain,
                         int *pWritten,
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
        APP_PutChar(pp, pRemain, pWritten, '-');
        val = -val;
    }

    /* 简单四舍五入 */
    double rounding = 0.5;
    for (i = 0; i < precision; i++)
    {
        rounding *= 0.1;
    }
    val += rounding;

    /* 整数部分 */
    unsigned long intPart = (unsigned long)val;
    double        frac    = val - (double)intPart;

    APP_PutUnsigned(pp, pRemain, pWritten, intPart, 10U, 0);

    if (precision <= 0)
    {
        return;
    }

    APP_PutChar(pp, pRemain, pWritten, '.');

    /* 小数部分 */
    for (i = 0; i < precision; i++)
    {
        frac *= 10.0;
        int digit = (int)frac;
        if (digit > 9)
        {
            digit = 9;
        }
        APP_PutChar(pp, pRemain, pWritten, (char)('0' + digit));
        frac -= (double)digit;
    }
}

/* -------------------------------------------------------------------------- */
/* 核心实现：APP_LOG_vsnprintf                                                */
/* -------------------------------------------------------------------------- */

int APP_LOG_vsnprintf(char *buf,
                      size_t bufSize,
                      const char *fmt,
                      va_list ap)
{
    char   *out    = buf;
    size_t  remain = bufSize;
    int     written = 0;   /* 理论写入字符数（不含 '\0'） */

    if ((buf == NULL) || (fmt == NULL) || (bufSize == 0U))
    {
        return 0;
    }

    *out = '\0';

    const char *p = fmt;

    while (*p != '\0')
    {
        if (*p != '%')
        {
            /* 普通字符 */
            APP_PutChar(&out, &remain, &written, *p++);
            continue;
        }

        /* 遇到 '%' */
        p++;  /* skip '%' */

        if (*p == '%')
        {
            APP_PutChar(&out, &remain, &written, '%');
            p++;
            continue;
        }

        /* 解析简单的 flag / width / precision（只真正使用 precision） */
        int  precision = -1;
        int  havePrec  = 0;

        /* 跳过 flag 和 width（这里简化处理） */
        while (*p == '0' || *p == '-' || *p == '+' || *p == ' ' ||
               (*p >= '0' && *p <= '9'))
        {
            p++;
        }

        if (*p == '.')
        {
            p++;
            havePrec  = 1;
            precision = 0;
            while (*p >= '0' && *p <= '9')
            {
                precision = precision * 10 + (*p - '0');
                p++;
            }
        }

        /* 不处理长度修饰符（h, l 等），直接按 int/unsigned/double 读取 */

        char spec = *p;
        if (spec == '\0')
        {
            break;
        }

        switch (spec)
        {
            case 'd':
            case 'i':
            {
                int v = va_arg(ap, int);
                long lv = (long)v; /* C28x 上 int 一般为 16bit，可视需要改为 long */
                APP_PutSigned(&out, &remain, &written, lv);
                break;
            }

            case 'u':
            {
                unsigned int v = va_arg(ap, unsigned int);
                APP_PutUnsigned(&out, &remain, &written, (unsigned long)v, 10U, 0);
                break;
            }

            case 'x':
            case 'X':
            {
                unsigned int v = va_arg(ap, unsigned int);
                APP_PutUnsigned(&out, &remain, &written, (unsigned long)v, 16U,
                                (spec == 'X') ? 1 : 0);
                break;
            }

            case 's':
            {
                const char *s = va_arg(ap, const char *);
                APP_PutStr(&out, &remain, &written, s);
                break;
            }

            case 'c':
            {
                int ch = va_arg(ap, int);
                APP_PutChar(&out, &remain, &written, (char)ch);
                break;
            }

            case 'f':
            {
                /* 注意：变参中 float 被提升为 double，必须按 double 读取 */
                double fv = va_arg(ap, double);
                int prec = havePrec ? precision : -1;
                APP_PutFloat(&out, &remain, &written, fv, prec);
                break;
            }

            default:
                /* 未支持的格式：原样输出 '%<spec>'，便于调试 */
                APP_PutChar(&out, &remain, &written, '%');
                APP_PutChar(&out, &remain, &written, spec);
                break;
        }

        if (*p != '\0')
        {
            p++;
        }
    }

    /* 始终保证以 '\0' 结尾 */
    if (bufSize > 0U)
    {
        buf[bufSize - 1U] = '\0';
    }

    return written;
}

/* -------------------------------------------------------------------------- */
/* 封装版 snprintf                                                            */
/* -------------------------------------------------------------------------- */

int APP_LOG_snprintf(char *buf,
                     size_t bufSize,
                     const char *fmt,
                     ...)
{
    int     ret;
    va_list args;

    va_start(args, fmt);
    ret = APP_LOG_vsnprintf(buf, bufSize, fmt, args);
    va_end(args);

    return ret;
}
