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
#include "board.h"
#include "FreeRTOS.h"
#include "c2000_freertos.h"
/* ============================================================================
 * 协议配置
 * ========================================================================== */
#define APP_PROTO_SOF                (0x00A5u)
#define APP_PROTO_EOF                (0x005Au)

#define APP_PROTO_ROLE_MASTER        (1u)
#define APP_PROTO_ROLE_SLAVE         (2u)

#ifndef APP_PROTO_ROLE
#define APP_PROTO_ROLE               (APP_PROTO_ROLE_MASTER)
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
/* SPI 主机发送后期望从机返回的字数（用于 dummy 时钟补发与 ACK 读取） */
#ifndef APP_PROTO_SPI_ACK_WORDS
#define APP_PROTO_SPI_ACK_WORDS      (4u)
#endif
/* SPI 从机就绪线 GPIO：主机读“就绪=高”后再补发 dummy 读取 ACK */
#ifndef APP_PROTO_SPI_READY_GPIO
#define APP_PROTO_SPI_READY_GPIO     SPI_SLAVE_SYS
#endif
#ifndef APP_PROTO_SPI_READY_GPIO_PIN_CONFIG
/* SysCfg 生成的 pinmux 配置宏 */
#define APP_PROTO_SPI_READY_GPIO_PIN_CONFIG SPI_SLAVE_SYS_GPIO_PIN_CONFIG
#endif
/* 主机等待就绪线的超时时间（ms），需覆盖从机最长处理耗时 */
#ifndef APP_PROTO_SPI_READY_TIMEOUT_MS
#define APP_PROTO_SPI_READY_TIMEOUT_MS  20u
#endif
/* 主机轮询就绪线的间隔（ms），过小会增加 CPU 占用 */
#ifndef APP_PROTO_SPI_READY_POLL_MS
#define APP_PROTO_SPI_READY_POLL_MS     1u
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
 * Multi-channel config
 * ========================================================================== */
typedef enum
{
    APP_PROTO_DEV_NONE = 0,
    APP_PROTO_DEV_SCI0,
    APP_PROTO_DEV_SPI0
} APP_PROTO_Device;

typedef enum
{
    APP_PROTO_CH0 = 0u,
    APP_PROTO_CH1 = 1u
} APP_PROTO_ChannelId;

#ifndef APP_PROTO_CHANNEL_NUM
#define APP_PROTO_CHANNEL_NUM        (2u)
#endif

#if (APP_PROTO_CHANNEL_NUM != 2u)
#error "APP_PROTO_CHANNEL_NUM currently supports only 2 channels"
#endif

#ifndef APP_PROTO_CH0_ENABLE
#define APP_PROTO_CH0_ENABLE         (1u)
#endif

#ifndef APP_PROTO_CH1_ENABLE
#define APP_PROTO_CH1_ENABLE         (1u)
#endif

#ifndef APP_PROTO_CH0_DEV
#define APP_PROTO_CH0_DEV            (APP_PROTO_DEV_SCI0)
#endif

#ifndef APP_PROTO_CH1_DEV
#define APP_PROTO_CH1_DEV            (APP_PROTO_DEV_SPI0)
#endif


/* ============================================================================
 * 类型定义
 * ========================================================================== */
/* 解耦：从“环形缓冲区”读取字节（每个uint16低8位有效），返回实际读取个数 */
typedef uint16_t (*APP_PROTO_ReadFn)(uint16_t *pBuf, uint16_t len, void *pUser);

/* 解耦：向“发送环形缓冲区”写入字节（每个uint16低8位有效），返回实际写入个数 */
typedef uint16_t (*APP_PROTO_WriteFn)(const uint16_t *pData, uint16_t len, void *pUser);

/* Optional lock/unlock around polling (non-zero return means locked) */
typedef uint16_t (*APP_PROTO_LockFn)(void *pUser);
typedef void (*APP_PROTO_UnlockFn)(void *pUser);


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
    uint16_t         initialized;                     /* 是否已完成初始化（非0表示已初始化） */
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
    APP_PROTO_LockFn        lockFn;
    APP_PROTO_UnlockFn      unlockFn;
    void                   *pLockUser;


} APP_PROTO_Ctx;

/* ============================================================================
 * API
 * ========================================================================== */
/**
 * @brief 获取通道上下文指针（仅通道使能时有效）。
 *
 * @param[in] ch 通道号（APP_PROTO_CH0/APP_PROTO_CH1）。
 *
 * @return 通道上下文指针；若通道未使能或无效则返回 NULL。
 */
APP_PROTO_Ctx *APP_PROTO_GetChannelCtx(uint16_t ch);

/**
 * @brief 查询通道是否使能。
 *
 * @param[in] ch 通道号（APP_PROTO_CH0/APP_PROTO_CH1）。
 *
 * @return 1：使能；0：未使能或无效通道。
 */
uint16_t APP_PROTO_IsChannelEnabled(uint16_t ch);

/**
 * @brief 初始化所有通道（按 APP_PROTO_CHx_ENABLE/DEV 配置进行绑定）。
 *
 * @note 该函数仅完成协议上下文与 IO 绑定，不会创建任务。
 */
void APP_PROTO_InitAll(void);

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
/**
 * @brief 初始化 Master 侧协议上下文。
 *
 * @param[in,out] pCtx    协议上下文指针。
 * @param[in]     handler 完整帧回调（可为 NULL）。
 * @param[in]     pUser   回调用户参数。
 */
void APP_PROTO_MasterInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser);

/**
 * @brief 注册 Master 侧 IO 回调（读/写）。
 *
 * @param[in,out] pCtx      协议上下文。
 * @param[in]     readFn    读回调（可为 NULL）。
 * @param[in]     pReadUser 读回调用户参数。
 * @param[in]     writeFn   写回调（可为 NULL）。
 * @param[in]     pWriteUser写回调用户参数。
 */
void APP_PROTO_MasterRegisterIO(APP_PROTO_Ctx *pCtx,
                                APP_PROTO_ReadFn readFn,
                                void *pReadUser,
                                APP_PROTO_WriteFn writeFn,
                                void *pWriteUser);

/**
 * @brief Master 轮询解析（从已注册的读回调获取数据并解析）。
 *
 * @param[in,out] pCtx 协议上下文。
 */
void APP_PROTO_MasterPoll(APP_PROTO_Ctx *pCtx);

/**
 * @brief Master 发送 WRITE 帧。
 *
 * @param[in] pCtx  协议上下文（需已初始化）。
 * @param[in] pPay  payload 指针（len=0 可为 NULL）。
 * @param[in] len   payload 长度（<= APP_PROTO_MAX_PAYLOAD）。
 *
 * @return 实际写入发送缓冲区的 word 数；0 表示失败。
 */
uint16_t APP_PROTO_MasterWrite(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);

/**
 * @brief Master 发送 READ 帧。
 *
 * @param[in] pCtx  协议上下文（需已初始化）。
 * @param[in] pPay  payload 指针（len=0 可为 NULL）。
 * @param[in] len   payload 长度（<= APP_PROTO_MAX_PAYLOAD）。
 *
 * @return 实际写入发送缓冲区的 word 数；0 表示失败。
 */
uint16_t APP_PROTO_MasterRead(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
/**
 * @brief 初始化 Slave 侧协议上下文。
 *
 * @param[in,out] pCtx    协议上下文指针。
 * @param[in]     handler 完整帧回调（可为 NULL）。
 * @param[in]     pUser   回调用户参数。
 */
void APP_PROTO_SlaveInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser);

/**
 * @brief 注册 Slave 侧 IO 回调（读/写）。
 *
 * @param[in,out] pCtx      协议上下文。
 * @param[in]     readFn    读回调（可为 NULL）。
 * @param[in]     pReadUser 读回调用户参数。
 * @param[in]     writeFn   写回调（可为 NULL）。
 * @param[in]     pWriteUser写回调用户参数。
 */
void APP_PROTO_SlaveRegisterIO(APP_PROTO_Ctx *pCtx,
                            APP_PROTO_ReadFn readFn,
                            void *pReadUser,
                            APP_PROTO_WriteFn writeFn,
                            void *pWriteUser);

/**
 * @brief Slave 轮询解析（可带锁保护）。
 *
 * @param[in,out] pCtx 协议上下文。
 */
void APP_PROTO_SlavePoll(APP_PROTO_Ctx *pCtx);

/**
 * @brief Slave 发送 ACK 帧。
 *
 * @param[in] pCtx  协议上下文（需已初始化）。
 *
 * @return 实际写入发送缓冲区的 word 数；0 表示失败。
 */

uint16_t APP_PROTO_SlaveACK(APP_PROTO_Ctx *pCtx);
                              
/**
 * @brief Slave 发送 WRITE 帧。
 *
 * @param[in] pCtx  协议上下文（需已初始化）。
 * @param[in] pPay  payload 指针（len=0 可为 NULL）。
 * @param[in] len   payload 长度（<= APP_PROTO_MAX_PAYLOAD）。
 *
 * @return 实际写入发送缓冲区的 word 数；0 表示失败。
 */
uint16_t APP_PROTO_SlaveWrite(APP_PROTO_Ctx *pCtx,
                            const uint16_t *pPay,
                            uint16_t len);

/**
 * @brief Slave 发送 READ 帧。
 *
 * @param[in] pCtx  协议上下文（需已初始化）。
 * @param[in] pPay  payload 指针（len=0 可为 NULL）。
 * @param[in] len   payload 长度（<= APP_PROTO_MAX_PAYLOAD）。
 *
 * @return 实际写入发送缓冲区的 word 数；0 表示失败。
 */
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
