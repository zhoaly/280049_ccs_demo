/* app_proto.h */
#ifndef APP_PROTO_H
#define APP_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "drv_sci.h"
#include "app_log.h"
#include "FreeRTOS.h"
#include "task.h"
/* ============================================================================
 * 协议配置
 * ========================================================================== */
#define APP_PROTO_SOF                (0x00A5u)
#define APP_PROTO_EOF                (0x005Au)

#define APP_PROTO_ROLE_MASTER        (1u)
#define APP_PROTO_ROLE_SLAVE         (2u)

#ifndef APP_PROTO_ROLE
#define APP_PROTO_ROLE               (APP_PROTO_ROLE_SLAVE)
#endif

#if (APP_PROTO_ROLE != APP_PROTO_ROLE_MASTER) && (APP_PROTO_ROLE != APP_PROTO_ROLE_SLAVE)
#error "APP_PROTO_ROLE must be APP_PROTO_ROLE_MASTER or APP_PROTO_ROLE_SLAVE"
#endif

typedef enum
{
    APP_PROTO_CMD_WRITE = 0x01u,  /* Master -> Slave write */
    APP_PROTO_CMD_READ  = 0x02u,  /* Master <- Slave read  */
    APP_PROTO_CMD_ACK  = 0x03u    /* Master <- Slave read  */
} APP_PROTO_Cmd;

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
typedef void (*APP_PROTO_FrameHandler)(APP_PROTO_Cmd cmd,//TODO 当前未实现handel
                                       const uint16_t *pPayload,
                                       uint16_t len,
                                       void *pUser);

/* 协议上下文 */
typedef struct
{
    APP_PROTO_State  state;
    APP_PROTO_Cmd    cmd;                              /* 低8位有效 */
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
#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
void APP_PROTO_MasterInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser);
void APP_PROTO_MasterRegisterIO(APP_PROTO_Ctx *pCtx,
                                APP_PROTO_ReadFn readFn,
                                void *pReadUser,
                                APP_PROTO_WriteFn writeFn,
                                void *pWriteUser);
void APP_PROTO_MasterPoll(APP_PROTO_Ctx *pCtx);
uint16_t APP_PROTO_MasterWrite(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);
uint16_t APP_PROTO_MasterRead(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
void APP_PROTO_SlaveInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser);
void APP_PROTO_SlaveRegisterIO(APP_PROTO_Ctx *pCtx,
                            APP_PROTO_ReadFn readFn,
                            void *pReadUser,
                            APP_PROTO_WriteFn writeFn,
                            void *pWriteUser);
void APP_PROTO_SlavePoll(APP_PROTO_Ctx *pCtx);
uint16_t APP_PROTO_SlaveWrite(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);
uint16_t APP_PROTO_SlaveRead(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);
#endif

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
#define APP_PROTO_Init        APP_PROTO_MasterInit
#define APP_PROTO_RegisterIO  APP_PROTO_MasterRegisterIO
#define APP_PROTO_Poll        APP_PROTO_MasterPoll
#define APP_PROTO_SendWrite   APP_PROTO_MasterWrite
#define APP_PROTO_SendRead    APP_PROTO_MasterRead
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
#define APP_PROTO_Init        APP_PROTO_SlaveInit
#define APP_PROTO_RegisterIO  APP_PROTO_SlaveRegisterIO
#define APP_PROTO_Poll        APP_PROTO_SlavePoll
#define APP_PROTO_SendWrite   APP_PROTO_SlaveWrite
#define APP_PROTO_SendRead    APP_PROTO_SlaveRead
#endif
#ifdef __cplusplus
}
#endif

#endif /* APP_PROTO_H */
