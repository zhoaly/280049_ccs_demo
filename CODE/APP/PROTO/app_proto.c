/* app_proto.c */
#include "__INCLUDE.h"

/* ============================================================================
 * 全局变量
 * ========================================================================== */


/* ============================================================================
 * 静态变量
 * ========================================================================== */
static const char * TAG ="proto";
 
typedef struct
{
    uint16_t id;
    uint16_t enabled;
    APP_PROTO_Device dev;
    APP_PROTO_Ctx ctx;
} APP_PROTO_Channel;

static APP_PROTO_Channel s_protoChannels[APP_PROTO_CHANNEL_NUM] =
{
    { (uint16_t)APP_PROTO_CH0, (uint16_t)APP_PROTO_CH0_ENABLE, APP_PROTO_CH0_DEV, {0} },
    { (uint16_t)APP_PROTO_CH1, (uint16_t)APP_PROTO_CH1_ENABLE, APP_PROTO_CH1_DEV, {0} }
};

static uint16_t APP_ProtoReadSci0(uint16_t *pBuf, uint16_t len, void *pUser)
{
    (void)pUser;
    return DRV_SCI0_RxReadBytes(pBuf, len);
}

static uint16_t APP_ProtoWriteSci0(const uint16_t *pData, uint16_t len, void *pUser)
{
    (void)pUser;
    return DRV_SCI0_TxWriteBytes(pData, len);
}

static uint16_t APP_ProtoReadSpi0(uint16_t *pBuf, uint16_t len, void *pUser)
{
    (void)pUser;
    return DRV_SPI0_RxReadWords(pBuf, len);
}

static uint16_t APP_ProtoWriteSpi0(const uint16_t *pData, uint16_t len, void *pUser)
{
    (void)pUser;
    return DRV_SPI0_TxWriteWords(pData, len);
}

static uint16_t APP_ProtoReadNone(uint16_t *pBuf, uint16_t len, void *pUser)
{
    (void)pBuf;
    (void)len;
    (void)pUser;
    return 0u;
}

static uint16_t APP_ProtoWriteNone(const uint16_t *pData, uint16_t len, void *pUser)
{
    (void)pData;
    (void)len;
    (void)pUser;
    return 0u;
}

static uint16_t APP_ProtoLockSci0(void *pUser)
{
    (void)pUser;
    if (xSemaphoreTake(SCI0Tx_SemaphoreHandle, portMAX_DELAY) == pdTRUE)
    {
        return 1u;
    }
    return 0u;
}

static void APP_ProtoUnlockSci0(void *pUser)
{
    (void)pUser;
    xSemaphoreGive(SCI0Tx_SemaphoreHandle);
}

static APP_PROTO_Channel *APP_PROTO_GetChannel(uint16_t ch)
{
    if (ch >= (uint16_t)APP_PROTO_CHANNEL_NUM)
    {
        return (APP_PROTO_Channel *)0;
    }
    return &s_protoChannels[ch];
}

APP_PROTO_Ctx *APP_PROTO_GetChannelCtx(uint16_t ch)
{
    APP_PROTO_Channel *channel = APP_PROTO_GetChannel(ch);
    if ((channel == (APP_PROTO_Channel *)0) || (channel->enabled == 0u))
    {
        return (APP_PROTO_Ctx *)0;
    }
    return &channel->ctx;
}

uint16_t APP_PROTO_IsChannelEnabled(uint16_t ch)
{
    APP_PROTO_Channel *channel = APP_PROTO_GetChannel(ch);
    if (channel == (APP_PROTO_Channel *)0)
    {
        return 0u;
    }
    return channel->enabled;
}

/**
 * @brief Slave 侧帧处理回调（完整帧到达时由解析器调用）。
 */
static void PROTO_SlaveHandler(APP_PROTO_Cmd cmd,
                               const uint16_t *pPayload,
                               uint16_t len,
                               void *pUser);

/**
 * @brief Master 侧帧处理回调（完整帧到达时由解析器调用）。
 */
static void PROTO_MasterHandler(APP_PROTO_Cmd cmd,
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

/**
 * @brief 绑定通道的读写 IO 与可选锁（不同设备使用不同驱动实现）。
 *
 * @param[in,out] ch 通道结构体指针。
 *
 * 说明：
 * - 依据通道配置的设备类型选择 SCI/SPI/None；
 * - SCI/SPI 可选绑定互斥锁，保证发送原子化；
 * - 未启用或无效通道则直接返回。
 */
static void APP_PROTO_ChannelBindIO(APP_PROTO_Channel *ch)
{
    if (ch == (APP_PROTO_Channel *)0)
    {
        return;
    }

    ch->ctx.lockFn    = (APP_PROTO_LockFn)0;
    ch->ctx.unlockFn  = (APP_PROTO_UnlockFn)0;
    ch->ctx.pLockUser = (void *)0;

    switch (ch->dev)
    {
        case APP_PROTO_DEV_SCI0:
        {
            APP_PROTO_RegisterIO(&ch->ctx,
                                 APP_ProtoReadSci0,  (void *)0,
                                 APP_ProtoWriteSci0, (void *)0);
            ch->ctx.lockFn   = APP_ProtoLockSci0;
            ch->ctx.unlockFn = APP_ProtoUnlockSci0;
        } break;

        case APP_PROTO_DEV_SPI0:
        {
            APP_PROTO_RegisterIO(&ch->ctx,
                                 APP_ProtoReadSpi0,  (void *)0,
                                 APP_ProtoWriteSpi0, (void *)0);
        } break;

        case APP_PROTO_DEV_NONE:
        default:
        {
            APP_PROTO_RegisterIO(&ch->ctx,
                                 (APP_PROTO_ReadFn)0,  (void *)0,
                                 (APP_PROTO_WriteFn)0, (void *)0);
        } break;
    }
}

/**
 * @brief 初始化单个通道（上下文初始化 + IO 绑定）。
 *
 * @param[in,out] ch 通道结构体指针。
 *
 * 说明：
 * - 按 APP_PROTO_ROLE 选择 Master/Slave 回调；
 * - 仅对已使能的通道生效。
 */
static void APP_PROTO_ChannelInitOne(APP_PROTO_Channel *ch)
{
    if ((ch == (APP_PROTO_Channel *)0) || (ch->enabled == 0u))
    {
        return;
    }

    /* 依据角色选择不同的帧处理回调 */
#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
    APP_PROTO_Init(&ch->ctx, PROTO_MasterHandler, (void *)ch);
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
    APP_PROTO_Init(&ch->ctx, PROTO_SlaveHandler, (void *)ch);
#else
    APP_PROTO_Init(&ch->ctx, (APP_PROTO_FrameHandler)0, (void *)ch);
#endif
    APP_PROTO_ChannelBindIO(ch);
}

/**
 * @brief 初始化所有通道。
 */
static void APP_PROTO_ChannelInitAll(void)
{
    uint16_t i;

    for (i = 0u; i < (uint16_t)APP_PROTO_CHANNEL_NUM; i++)
    {
        APP_PROTO_ChannelInitOne(&s_protoChannels[i]);
    }
}

/**
 * @brief 轮询单个通道并解析协议数据。
 *
 * @param[in,out] ch 通道结构体指针。
 */
static void APP_PROTO_ChannelPollOne(APP_PROTO_Channel *ch)
{
    if ((ch == (APP_PROTO_Channel *)0) || (ch->enabled == 0u))
    {
        return;
    }

    APP_PROTO_Poll(&ch->ctx);
}

/**
 * @brief 轮询所有通道并解析协议数据。
 */
static void APP_PROTO_ChannelPollAll(void)
{
    uint16_t i;

    for (i = 0u; i < (uint16_t)APP_PROTO_CHANNEL_NUM; i++)
    {
        APP_PROTO_ChannelPollOne(&s_protoChannels[i]);
    }
}

/**
 * @brief 协议任务入口：初始化通道并周期轮询解析。
 *
 * @param[in] pvParameters 任务参数（未使用）。
 *
 * 说明：
 * - 该任务仅负责解析与回调，不负责创建通道；
 * - Master/Slave 由 APP_PROTO_ROLE 编译时确定。
 */
void PROTO_Task_Func(void *pvParameters)
{
    (void)pvParameters;

    /* 初始化全部通道（包含 IO 绑定与回调注册） */
    APP_PROTO_ChannelInitAll();

    while (1)
    {
        /* 按角色轮询解析（Master/Slave 由条件编译控制） */
#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)
        APP_PROTO_ChannelPollAll();
        /* Master 可在此处扩展周期性发送逻辑 */
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)
        APP_PROTO_ChannelPollAll();
#else
        APP_PROTO_ChannelPollAll();
#endif
        vTaskDelay(pdTICKS_TO_MS(10));
    }
}

void APP_PROTO_InitAll(void)
{
    /**
     * @brief 初始化所有协议通道并完成 IO 绑定。
     *
     * 说明：
     * - 依据 APP_PROTO_CHx_ENABLE / APP_PROTO_CHx_DEV 进行通道开关与设备绑定；
     * - 仅初始化协议上下文与回调，不创建/启动任务；
     * - 建议在系统启动阶段调用一次。
     */
    APP_PROTO_ChannelInitAll();
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

    pCtx->initialized  = 1u;
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
    pCtx->lockFn    = (APP_PROTO_LockFn)0;
    pCtx->unlockFn  = (APP_PROTO_UnlockFn)0;
    pCtx->pLockUser = (void *)0;


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




#if (APP_PROTO_ROLE == APP_PROTO_ROLE_MASTER)

/**
 * @brief Master 侧帧处理：处理从 Slave 返回的帧。
 *
 * 说明：
 * - 当前实现仅做日志示例，便于验证链路；
 * - 可根据协议需求扩展解析与业务处理。
 */
static void PROTO_MasterHandler(APP_PROTO_Cmd cmd,
                                const uint16_t *pPayload,
                                uint16_t len,
                                void *pUser)
{
    APP_PROTO_Channel *channel = (APP_PROTO_Channel *)pUser;

    switch (cmd)
    {
        case APP_PROTO_CMD_WRITE:
        {
            if ((pPayload != (const uint16_t *)0) && (len > 0u))
            {
                (void)channel;
                APP_LOGI0(TAG, "Master RX WRITE response\n");
            }
        } break;

        case APP_PROTO_CMD_READ:
        {
            if ((pPayload != (const uint16_t *)0) && (len > 0u))
            {
                (void)channel;
                APP_LOGI0(TAG, "Master RX READ response\n");
            }
        } break;

        case APP_PROTO_CMD_ACK:
        {
            // (void)channel;
            //收到ACK 释放信号量
            if (channel->id ==APP_PROTO_CH0) {
                xSemaphoreGive(PROTO_ACK_CH1Handle);
            }
            else if(channel->id ==APP_PROTO_CH0){
                xSemaphoreGive(PROTO_ACK_CH1Handle);    
            }

            APP_LOGI0(TAG, "Master RX READ response\n");
            
        } break;

        default:
        {
            /* 其他命令暂不处理 */
        } break;
    }
}

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
    if ((pCtx == (APP_PROTO_Ctx *)0) || (pCtx->initialized == 0u))
    {
        return 0u;
    }
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_WRITE, pPay, len);
}

uint16_t APP_PROTO_MasterRead(APP_PROTO_Ctx *pCtx,
                              const uint16_t *pPay,
                              uint16_t len)
{
    if ((pCtx == (APP_PROTO_Ctx *)0) || (pCtx->initialized == 0u))
    {
        return 0u;
    }
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_READ, pPay, len);
}
#elif (APP_PROTO_ROLE == APP_PROTO_ROLE_SLAVE)

/**
 * @brief Slave 侧帧处理：根据命令执行对应逻辑。
 *
 * 说明：
 * - WRITE：打印日志并回写 payload（示例：回环响应）。
 * - READ ：当前仅打印日志，可扩展读取逻辑。
 */
static void PROTO_SlaveHandler(APP_PROTO_Cmd cmd,
                               const uint16_t *pPayload,
                               uint16_t len,
                               void *pUser)
{
    // uint16_t cmdOk = 0u;
    APP_PROTO_Channel *channel = (APP_PROTO_Channel *)pUser;
    APP_PROTO_Ctx *ctx = (APP_PROTO_Ctx *)0;

    if (channel != (APP_PROTO_Channel *)0)
    {
        ctx = &channel->ctx;
    }

    switch (cmd)
    {
        case APP_PROTO_CMD_WRITE:
        {
            if ((pPayload != (const uint16_t *)0) && (len > 0u))
            {
                if (channel->id == APP_PROTO_CH0) {
                    APP_LOGI0(TAG, "Receive WRITE CMD CH0\n");
                }
                else if (channel->id == APP_PROTO_CH1) {

                    // //发送ACK
                    // APP_PROTO_SlaveACK(ctx);
                    APP_LOGI0(TAG, "Receive WRITE CMD CH1\n");
                }
               
                //发送ACK
                if (ctx != (APP_PROTO_Ctx *)0)
                {
                    APP_PROTO_SlaveACK(ctx);
                }
            }
        } break;

        case APP_PROTO_CMD_READ:
        {
            if ((pPayload != (const uint16_t *)0) && (len > 0u))
            {
                APP_LOGI0(TAG, "Receive READ CMD \n");
            }

        } break;
        

        default:
        {
            // cmdOk = 0u;
        } break;
    }
}


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
    uint16_t locked = 1u;

    if (pCtx == (APP_PROTO_Ctx *)0)
    {
        return;
    }

    if (pCtx->lockFn != (APP_PROTO_LockFn)0)
    {
        locked = pCtx->lockFn(pCtx->pLockUser);
    }

    if (locked != 0u)
    {
        APP_PROTO_CorePoll(pCtx);
        if (pCtx->unlockFn != (APP_PROTO_UnlockFn)0)
        {
            pCtx->unlockFn(pCtx->pLockUser);
        }
    }
}


uint16_t APP_PROTO_SlaveACK(APP_PROTO_Ctx *pCtx)

{
    if ((pCtx == (APP_PROTO_Ctx *)0) || (pCtx->initialized == 0u))
    {
        return 0u;
    }
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_ACK, NULL, 0);
}


uint16_t APP_PROTO_SlaveWrite(APP_PROTO_Ctx *pCtx,
                              const uint16_t *pPay,
                              uint16_t len)
{
    if ((pCtx == (APP_PROTO_Ctx *)0) || (pCtx->initialized == 0u))
    {
        return 0u;
    }
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_WRITE, pPay, len);
}

uint16_t APP_PROTO_SlaveRead(APP_PROTO_Ctx *pCtx,
                             const uint16_t *pPay,
                             uint16_t len)
{
    if ((pCtx == (APP_PROTO_Ctx *)0) || (pCtx->initialized == 0u))
    {
        return 0u;
    }
    return APP_PROTO_CoreSendFrame(pCtx, APP_PROTO_CMD_READ, pPay, len);
}
#endif
