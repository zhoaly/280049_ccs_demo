/* app_proto.h */
#ifndef APP_PROTO_H
#define APP_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "drv_sci.h"
#include "FreeRTOS.h"
#include "task.h"
/* ============================================================================
 * 协议配置
 * ========================================================================== */
#define APP_PROTO_SOF                (0x00A5u)
#define APP_PROTO_EOF                (0x005Au)

#define APP_PROTO_CMD_WRITE          (0x0001u)
#define APP_PROTO_CMD_READ           (0x0002u)
#define APP_PROTO_CMD_RSVD3          (0x0003u)
#define APP_PROTO_CMD_RSVD4          (0x0004u)

/* Payload 最大长度（单位：字节；本实现用 uint16_t 数组承载，每元素低8位有效） */
#ifndef APP_PROTO_MAX_PAYLOAD
#define APP_PROTO_MAX_PAYLOAD        (64u)
#endif

/* 每次 Poll 从环形缓冲区读取的字数（uint16_t数组元素个数，低8位有效） */
#ifndef APP_PROTO_POLL_READ_CHUNK
#define APP_PROTO_POLL_READ_CHUNK    (32u)
#endif

/* 每次 Poll 最多读取多少轮（避免任务占用过久） */
#ifndef APP_PROTO_POLL_MAX_ROUNDS
#define APP_PROTO_POLL_MAX_ROUNDS    (8u)
#endif

/* ============================================================================
 * 类型定义
 * ========================================================================== */
/* 解耦：从“唤醒缓冲区/环形缓冲区”读取字节（每个uint16低8位有效），返回实际读取个数 */
typedef uint16_t (*APP_PROTO_ReadFn)(uint16_t *pBuf, uint16_t len, void *pUser);

/* 解耦：向“发送环形缓冲区”写入字节（每个uint16低8位有效），返回实际写入个数 */
typedef uint16_t (*APP_PROTO_WriteFn)(const uint16_t *pData, uint16_t len, void *pUser);

typedef enum
{
    APP_PROTO_ST_WAIT_SOF = 0,
    APP_PROTO_ST_WAIT_CMD,
    APP_PROTO_ST_WAIT_LEN,
    APP_PROTO_ST_WAIT_PAYLOAD,
    APP_PROTO_ST_WAIT_EOF
} APP_PROTO_State;

typedef enum
{
    APP_PROTO_ERR_NONE = 0,
    APP_PROTO_ERR_CMD,
    APP_PROTO_ERR_LEN,
    APP_PROTO_ERR_EOF
} APP_PROTO_Error;

/* 完整帧回调：cmd + payload + len
 * 注意：payload 每个 uint16_t 低8位有效；len 为“字节数/元素数”
 */
typedef void (*APP_PROTO_FrameHandler)(uint16_t cmd,
                                       const uint16_t *pPayload,
                                       uint16_t len,
                                       void *pUser);

/* 协议上下文 */
typedef struct
{
    APP_PROTO_State  state;
    uint16_t         cmd;                              /* 低8位有效 */
    uint16_t         len;                              /* payload长度（字节数） */
    uint16_t         idx;                              /* payload索引 */
    uint16_t         payload[APP_PROTO_MAX_PAYLOAD];   /* 每元素低8位有效 */

    /* 统计信息 */
    uint32_t         cntDropBytes;     /* WAIT_SOF 状态丢弃的无效字节 */
    uint32_t         cntOkFrames;      /* 成功解析的帧数 */
    uint32_t         cntErrFrames;     /* 解析错误帧数 */
    uint32_t         cntErrCmd;
    uint32_t         cntErrLen;
    uint32_t         cntErrEof;

    APP_PROTO_FrameHandler handler;
    void                  *pHandlerUser;

    /* IO 回调（注册后由 Poll/SendFrame 直接使用） */
    APP_PROTO_ReadFn       readFn;
    void                  *pReadUser;
    APP_PROTO_WriteFn      writeFn;
    void                  *pWriteUser;

} APP_PROTO_Ctx;

/* ============================================================================
 * API
 * ========================================================================== */
void APP_PROTO_Init(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser);

/**
 * @brief 注册协议 IO（读/写）回调函数
 *
 * @param[in,out] pCtx        协议上下文
 * @param[in]     readFn      读回调：从环形缓冲区读取（每个uint16低8位有效）
 * @param[in]     pReadUser   读回调用户参数
 * @param[in]     writeFn     写回调：向发送缓冲区写入（每个uint16低8位有效）
 * @param[in]     pWriteUser  写回调用户参数
 *
 * 说明：
 * - 协议层不直接依赖 SCI/SPI/USB 等底层；通过注册回调实现解耦。
 * - 可单独注册读或写（另一个可传 NULL）。
 */
void APP_PROTO_RegisterIO(APP_PROTO_Ctx *pCtx,
                          APP_PROTO_ReadFn readFn,
                          void *pReadUser,
                          APP_PROTO_WriteFn writeFn,
                          void *pWriteUser);

/**
 * @brief 协议轮询解析：使用已注册 readFn 从环形缓冲区读取并按帧解析
 *
 * @param[in,out] pCtx 协议上下文
 *
 * 说明：
 * - 必须先调用 APP_PROTO_RegisterIO() 注册 readFn，否则本函数直接返回。
 * - 每次 Poll 会循环读取多批数据，直到读不到数据或达到 APP_PROTO_POLL_MAX_ROUNDS。
 * - 解析器仅依赖已注册 readFn，不直接依赖底层通信方式。
 */
void APP_PROTO_Poll(APP_PROTO_Ctx *pCtx);

/**
 * @brief 组帧：SOF + CMD + LEN + PAYLOAD + EOF
 *
 * @param[in]  cmd     命令字（低8位有效）
 * @param[in]  pPay    payload 指针（len=0 时可为 NULL；每元素低8位有效）
 * @param[in]  len     payload 长度（单位：字节/元素个数，<= APP_PROTO_MAX_PAYLOAD）
 * @param[out] pOutU16 输出缓冲（uint16 数组，每个元素低 8 位为有效字节）
 * @param[in]  outCap  输出缓冲容量（单位：uint16 元素个数）
 *
 * @return 实际生成的帧长度（单位：字节/uint16 元素个数）；0 表示失败
 *
 * 失败条件：
 * - pOutU16 为空或 outCap 为 0
 * - len 超过最大 payload
 * - cmd 非法
 * - outCap 不足以容纳完整帧
 */
uint16_t APP_PROTO_BuildFrame(uint16_t cmd,
                              const uint16_t *pPay,
                              uint16_t len,
                              uint16_t *pOutU16,
                              uint16_t outCap);

/**
 * @brief 发送帧：先组帧再使用已注册 writeFn 写入发送缓冲区
 *
 * @param[in] pCtx 协议上下文
 * @param[in] cmd  命令字（低8位有效）
 * @param[in] pPay payload 指针（len=0 时可为 NULL；每元素低8位有效）
 * @param[in] len  payload 长度（<= APP_PROTO_MAX_PAYLOAD）
 *
 * @return 实际写入发送缓冲区的字节数（uint16 元素个数）；0 表示失败
 *
 * 说明：
 * - 必须先调用 APP_PROTO_RegisterIO() 注册 writeFn，否则直接返回 0。
 * - writeFn 返回值可能小于帧长度（例如发送缓冲区满），上层可据此做重发/补发策略。
 */
uint16_t APP_PROTO_SendFrame(APP_PROTO_Ctx *pCtx,
                             uint16_t cmd,
                             const uint16_t *pPay,
                             uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* APP_PROTO_H */
