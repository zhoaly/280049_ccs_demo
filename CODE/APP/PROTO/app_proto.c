/* app_proto.c */
#include "app_proto.h"
#include "device.h"
#include "board.h"

/* ============================================================================
 * 全局变量
 * ========================================================================== */


/* ============================================================================
 * 静态变量
 * ========================================================================== */
static APP_PROTO_Ctx s_protoCtx;
/* ============================================================================
 * 内部静态函数
 * ========================================================================== */

//两个回调函数
/* 从 RX 环形缓冲区读取：每个 uint16_t 的低 8 位为有效字节 */
static uint16_t APP_ProtoRead(uint16_t *pBuf, uint16_t len, void *pUser)
{
    (void)pUser;
    return DRV_SCI0_RxReadBytes(pBuf, len);
}

/* 向 TX 环形缓冲区写入：每个 uint16_t 的低 8 位为有效字节 */
static uint16_t APP_ProtoWrite(const uint16_t *pData, uint16_t len, void *pUser)
{
    (void)pUser;
    return DRV_SCI0_TxWriteBytes(pData, len);//TODO:当前存在bug,会和LOG模块冲突,应实现原子化操作(似乎应在任务中自行实现)
}

static void PROTO_Handler(APP_PROTO_Cmd cmd,
                          const uint16_t *pPayload,
                          uint16_t len,
                          void *pUser);


/**
 * @brief 判断命令字是否合法
 *
 * @param[in] cmd 命令字（uint16_t，低8位有效）
 *
 * @return 1：合法；0：非法
 *
 * 说明：
 * - 仅允许协议定义的命令字（WRITE/READ/预留）。
 * - 非法命令会触发解析错误并进行丢帧重同步。
 */
static inline uint16_t APP_PROTO_IsCmdValid(APP_PROTO_Cmd cmd)
{
    uint16_t cmdByte = (uint16_t)(((uint16_t)cmd) & 0x00FFu);

    return (uint16_t)((cmdByte == (uint16_t)APP_PROTO_CMD_WRITE) ||
                      (cmdByte == (uint16_t)APP_PROTO_CMD_READ));
}

/**
 * @brief 将协议解析器状态机复位到等待 SOF 状态
 *
 * @param[in,out] pCtx 协议上下文
 *
 * 说明：
 * - 复位 state/cmd/len/idx，用于启动解析或错误恢复。
 * - 复位后会重新寻找 SOF（0xA5）。
 */
static void APP_PROTO_Reset(APP_PROTO_Ctx *pCtx)
{
    pCtx->state = APP_PROTO_ST_WAIT_SOF;
    pCtx->cmd   = (APP_PROTO_Cmd)0u;
    pCtx->len   = 0u;
    pCtx->idx   = 0u;
}

/**
 * @brief 协议解析错误处理与重同步
 *
 * @param[in,out] pCtx    协议上下文
 * @param[in]     err     错误类型（CMD/LEN/EOF）
 * @param[in]     curByte 当前触发错误的字节（uint16_t，低8位有效）
 *
 * 说明：
 * - 记录错误统计计数。
 * - 执行丢帧并重同步：
 *   - 若 curByte 本身就是 SOF（0xA5），直接进入 WAIT_CMD（加速重同步）。
 *   - 否则复位到 WAIT_SOF，继续丢弃直到找到 SOF。
 */
static void APP_PROTO_OnError(APP_PROTO_Ctx *pCtx, APP_PROTO_Error err, uint16_t curByte)
{
    pCtx->cntErrFrames++;

    switch (err)
    {
        case APP_PROTO_ERR_CMD: pCtx->cntErrCmd++; break;
        case APP_PROTO_ERR_LEN: pCtx->cntErrLen++; break;
        case APP_PROTO_ERR_EOF: pCtx->cntErrEof++; break;
        default: break;
    }

    /* 重同步加速：若当前字节就是 SOF，则直接进入 WAIT_CMD（把它当作新帧头） */
    if ( (uint16_t)(curByte & 0x00FFu) == (uint16_t)(APP_PROTO_SOF & 0x00FFu) )
    {
        pCtx->state = APP_PROTO_ST_WAIT_CMD;
        pCtx->cmd   = (APP_PROTO_Cmd)0u;
        pCtx->len   = 0u;
        pCtx->idx   = 0u;
    }
    else
    {
        APP_PROTO_Reset(pCtx);
    }
}

/**
 * @brief 向解析器喂入 1 个字节，推进协议状态机
 *
 * @param[in,out] pCtx 协议上下文
 * @param[in]     byte 输入字节（uint16_t，低8位有效）
 *
 * 状态机流程：
 * - WAIT_SOF     : 等待 SOF(0xA5)，否则丢弃（握手失败丢弃）
 * - WAIT_CMD     : 读取命令字并校验合法性
 * - WAIT_LEN     : 读取 payload 长度并校验（<= MAX_PAYLOAD）
 * - WAIT_PAYLOAD : 按长度收集 payload
 * - WAIT_EOF     : 校验 EOF(0x5A)，成功则回调上层 handler
 *
 * 错误处理：
 * - CMD/LEN/EOF 任一错误：计数 + 丢帧 + 重同步（可利用 SOF 加速重同步）
 */
static void APP_PROTO_FeedByte(APP_PROTO_Ctx *pCtx, uint16_t byte)
{
    byte = (uint16_t)(byte & 0x00FFu);

    switch (pCtx->state)
    {
        case APP_PROTO_ST_WAIT_SOF:
        {
            if (byte == (uint16_t)(APP_PROTO_SOF & 0x00FFu))
            {
                pCtx->state = APP_PROTO_ST_WAIT_CMD;
            }
            else
            {
                /* 握手失败：丢弃无效字节 */
                pCtx->cntDropBytes++;
            }
        } break;

        case APP_PROTO_ST_WAIT_CMD:
        {
            if (APP_PROTO_IsCmdValid((APP_PROTO_Cmd)byte) == 0u)
            {
                APP_PROTO_OnError(pCtx, APP_PROTO_ERR_CMD, byte);
                break;
            }

            pCtx->cmd   = (APP_PROTO_Cmd)byte;
            pCtx->state = APP_PROTO_ST_WAIT_LEN;
        } break;

        case APP_PROTO_ST_WAIT_LEN:
        {
            pCtx->len = byte;

            if (pCtx->len > (uint16_t)APP_PROTO_MAX_PAYLOAD)
            {
                APP_PROTO_OnError(pCtx, APP_PROTO_ERR_LEN, byte);
                break;
            }

            pCtx->idx = 0u;

            if (pCtx->len == 0u)
            {
                pCtx->state = APP_PROTO_ST_WAIT_EOF;
            }
            else
            {
                pCtx->state = APP_PROTO_ST_WAIT_PAYLOAD;
            }
        } break;

        case APP_PROTO_ST_WAIT_PAYLOAD:
        {
            pCtx->payload[pCtx->idx] = byte; /* 低8位有效 */
            pCtx->idx++;

            if (pCtx->idx >= pCtx->len)
            {
                pCtx->state = APP_PROTO_ST_WAIT_EOF;
            }
        } break;

        case APP_PROTO_ST_WAIT_EOF:
        {
            if (byte != (uint16_t)(APP_PROTO_EOF & 0x00FFu))
            {
                APP_PROTO_OnError(pCtx, APP_PROTO_ERR_EOF, byte);
                break;
            }

            /* 完整帧成功 调用hand */
            pCtx->cntOkFrames++;

            if (pCtx->handler != (APP_PROTO_FrameHandler)0)
            {
                pCtx->handler(pCtx->cmd,
                              pCtx->payload,
                              pCtx->len,
                              pCtx->pHandlerUser);
            }

            /* 准备下一帧 */
            APP_PROTO_Reset(pCtx);
        } break;

        default:
        {
            APP_PROTO_Reset(pCtx);
        } break;
    }
}

/* ============================================================================
 * 对外 API
 * ========================================================================== */

/**
 * @brief 协议任务函数（示例占位）
 *
 * @param[in] pvParameters 任务参数
 *
 * 说明：
 * - 可在此函数中初始化 APP_PROTO，并周期调用 APP_PROTO_Poll() 进行解析。
 * - 建议结合事件/信号量，在有新数据时唤醒任务以降低 CPU 占用。
 */
void PROTO_Task_Func(void *pvParameters)
{
    (void)pvParameters;
    // APP_PROTO_Init(&s_protoCtx, PROTO_Handler, (void *)0);
    
    // APP_PROTO_RegisterIO(&s_protoCtx,
    //                      APP_ProtoRead,  (void *)0,
    //                      APP_ProtoWrite, (void *)0);
    while (1)
    {
        // APP_PROTO_Poll(&s_protoCtx);
        vTaskDelay(pdTICKS_TO_MS(10));
        
    }
}

/**
 * @brief 初始化协议解析器上下文
 *
 * @param[in,out] pCtx    协议上下文指针
 * @param[in]     handler 完整帧回调函数指针（可为 NULL）
 * @param[in]     pUser   回调用户参数（透传给 handler）
 *
 * 说明：
 * - 清零统计计数。
 * - 复位状态机到 WAIT_SOF。
 * - IO 回调默认置空，需通过 APP_PROTO_RegisterIO() 注册。
 * - 解析成功后将调用 handler(cmd, payload, len, pUser)。
 */
static void APP_PROTO_CoreInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser)
{
    if (pCtx == (APP_PROTO_Ctx *)0)
    {
        return;
    }

    pCtx->handler      = handler;
    pCtx->pHandlerUser = pUser;

    pCtx->cntDropBytes = 0u;
    pCtx->cntOkFrames  = 0u;
    pCtx->cntErrFrames = 0u;
    pCtx->cntErrCmd    = 0u;
    pCtx->cntErrLen    = 0u;
    pCtx->cntErrEof    = 0u;

    /* IO 回调默认置空，由 RegisterIO 绑定 */
    pCtx->readFn     = (APP_PROTO_ReadFn)0;
    pCtx->pReadUser  = (void *)0;
    pCtx->writeFn    = (APP_PROTO_WriteFn)0;
    pCtx->pWriteUser = (void *)0;

    APP_PROTO_Reset(pCtx);
}

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
static void APP_PROTO_CoreRegisterIO(APP_PROTO_Ctx *pCtx,
                                     APP_PROTO_ReadFn readFn,
                                     void *pReadUser,
                                     APP_PROTO_WriteFn writeFn,
                                     void *pWriteUser)
{
    if (pCtx == (APP_PROTO_Ctx *)0)
    {
        return;
    }

    pCtx->readFn     = readFn;
    pCtx->pReadUser  = pReadUser;
    pCtx->writeFn    = writeFn;
    pCtx->pWriteUser = pWriteUser;
}

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
static void APP_PROTO_CorePoll(APP_PROTO_Ctx *pCtx)
{
    uint16_t rxBuf[APP_PROTO_POLL_READ_CHUNK];
    uint16_t n;
    uint16_t i;
    uint16_t round = 0u;

    if (pCtx == (APP_PROTO_Ctx *)0)
    {
        return;
    }

    if (pCtx->readFn == (APP_PROTO_ReadFn)0)
    {
        return;
    }

    /* 轮询读取：直到缓冲区没数据或达到最大轮数 */
    do
    {
        n = pCtx->readFn(rxBuf,
                         (uint16_t)(sizeof(rxBuf) / sizeof(rxBuf[0])),
                         pCtx->pReadUser);
        if (n == 0u)
        {
            break;
        }

        for (i = 0u; i < n; i++)
        {
            /* 每个 uint16_t 仅低 8 位有效 */
            APP_PROTO_FeedByte(pCtx, (uint16_t)(rxBuf[i] & 0x00FFu));
        }

        round++;
    } while (round < (uint16_t)APP_PROTO_POLL_MAX_ROUNDS);
}

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
static uint16_t APP_PROTO_CoreBuildFrame(APP_PROTO_Cmd cmd,
                                         const uint16_t *pPay,
                                         uint16_t len,
                                         uint16_t *pOutU16,
                                         uint16_t outCap)
{
    uint16_t need;
    uint16_t pos = 0u;
    uint16_t k;
    uint16_t cmdByte;

    if ((pOutU16 == (uint16_t *)0) || (outCap == 0u))
    {
        return 0u;
    }

    cmdByte = (uint16_t)(((uint16_t)cmd) & 0x00FFu);

    if ((len > (uint16_t)APP_PROTO_MAX_PAYLOAD) || (APP_PROTO_IsCmdValid(cmd) == 0u))
    {
        return 0u;
    }

    /* SOF(1) + CMD(1) + LEN(1) + PAY(len) + EOF(1) */
    need = (uint16_t)(1u + 1u + 1u + len + 1u);
    if (outCap < need)
    {
        return 0u;
    }

    pOutU16[pos++] = (uint16_t)(APP_PROTO_SOF & 0x00FFu);
    pOutU16[pos++] = cmdByte;

    /* LEN 字段语义按 1Byte 使用（低8位有效） */
    pOutU16[pos++] = (uint16_t)(len & 0x00FFu);

    for (k = 0u; k < len; k++)
    {
        uint16_t b = (pPay == (const uint16_t *)0) ? 0u : (uint16_t)(pPay[k] & 0x00FFu);
        pOutU16[pos++] = b;
    }

    pOutU16[pos++] = (uint16_t)(APP_PROTO_EOF & 0x00FFu);

    return pos;
}

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
static uint16_t APP_PROTO_CoreSendFrame(APP_PROTO_Ctx *pCtx,
                                        APP_PROTO_Cmd cmd,
                                        const uint16_t *pPay,
                                        uint16_t len)
{
    uint16_t frameU16[1u + 1u + 1u + APP_PROTO_MAX_PAYLOAD + 1u];
    uint16_t frameLen;

    if (pCtx == (APP_PROTO_Ctx *)0)
    {
        return 0u;
    }

    if (pCtx->writeFn == (APP_PROTO_WriteFn)0)
    {
        return 0u;
    }

    frameLen = APP_PROTO_CoreBuildFrame(cmd, pPay, len,
                                    frameU16,
                                    (uint16_t)(sizeof(frameU16) / sizeof(frameU16[0])));
    if (frameLen == 0u)
    {
        return 0u;
    }

    return pCtx->writeFn(frameU16, frameLen, pCtx->pWriteUser);
}

static void PROTO_Handler(APP_PROTO_Cmd cmd,
                          const uint16_t *pPayload,
                          uint16_t len,
                          void *pUser)
{
    // uint16_t cmdOk = 0u;

    (void)pUser;

    switch (cmd)
    {
        case APP_PROTO_CMD_WRITE:
        {
            if ((pPayload != (const uint16_t *)0) && (len > 0u))
            {
                // cmdOk = 1u;
                GPIO_togglePin(myLED2_GPIO);
            }
        } break;

        case APP_PROTO_CMD_READ:
        {

        } break;

        default:
        {
            // cmdOk = 0u;
        } break;
    }
}

#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
void APP_PROTO_MasterInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser)
{
    APP_PROTO_CoreInit(pCtx, handler, pUser);
}

void APP_PROTO_MasterRegisterIO(APP_PROTO_Ctx *pCtx,
                                APP_PROTO_ReadFn readFn,
                                void *pReadUser,
                                APP_PROTO_WriteFn writeFn,
                                void *pWriteUser)
{
    APP_PROTO_CoreRegisterIO(pCtx, readFn, pReadUser, writeFn, pWriteUser);
}

void APP_PROTO_MasterPoll(APP_PROTO_Ctx *pCtx)
{
    APP_PROTO_CorePoll(pCtx);
}

uint16_t APP_PROTO_MasterWrite(APP_PROTO_Ctx *pCtx,
                               const uint16_t *pPay,
                               uint16_t len)
{
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_WRITE, pPay, len);
}

uint16_t APP_PROTO_MasterRead(APP_PROTO_Ctx *pCtx,
                              const uint16_t *pPay,
                              uint16_t len)
{
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_READ, pPay, len);
}
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
void APP_PROTO_SlaveInit(APP_PROTO_Ctx *pCtx, APP_PROTO_FrameHandler handler, void *pUser)
{
    APP_PROTO_CoreInit(pCtx, handler, pUser);
}

void APP_PROTO_SlaveRegisterIO(APP_PROTO_Ctx *pCtx,
                               APP_PROTO_ReadFn readFn,
                               void *pReadUser,
                               APP_PROTO_WriteFn writeFn,
                               void *pWriteUser)
{
    APP_PROTO_CoreRegisterIO(pCtx, readFn, pReadUser, writeFn, pWriteUser);
}

void APP_PROTO_SlavePoll(APP_PROTO_Ctx *pCtx)
{
    APP_PROTO_CorePoll(pCtx);
}

uint16_t APP_PROTO_SlaveWrite(APP_PROTO_Ctx *pCtx,
                              const uint16_t *pPay,
                              uint16_t len)
{
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_WRITE, pPay, len);
}

uint16_t APP_PROTO_SlaveRead(APP_PROTO_Ctx *pCtx,
                             const uint16_t *pPay,
                             uint16_t len)
{
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_READ, pPay, len);
}
#endif
