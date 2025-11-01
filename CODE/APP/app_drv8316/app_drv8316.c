/**
 * @file app_drv8316.c
 * @brief DRV8316 应用层管理模块，实现对底层驱动的统一封装与周期性维护。
 *
 * 本模块位于应用层，负责协调 SPI 通道、DRV8316 底层驱动库以及 FreeRTOS 任务，
 * 为上层业务提供线程安全的寄存器访问接口。模块通过静态互斥量保护共享状态，
 * 在指定任务中周期性触发寄存器刷新，并响应手动写入或读取请求。
 */

#include "app_drv8316.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "device.h"

#include "drv8316s.h"

/** DRV8316 底层对象实例，供驱动库初始化句柄时使用。 */
static DRV8316_Obj     s_drvObj;
/** 指向底层驱动句柄的指针，用于实际执行 SPI 读写。 */
static DRV8316_Handle  s_drvHandle = NULL;
/** 用于缓存驱动库维护的寄存器镜像及命令标志。 */
static DRV8316_VARS_t  s_drvVars;
/** 记录当前生效的应用层配置，便于任务动态调整周期。 */
static APP_DRV8316_Config s_runtimeConfig;

/** 互斥量静态存储缓冲区，避免堆分配依赖。 */
static StaticSemaphore_t s_mutexBuffer;
/** 互斥量句柄，保护模块内部共享状态。 */
static SemaphoreHandle_t s_mutexHandle = NULL;

/** 标识模块是否已经完成初始化流程。 */
static volatile bool     s_initialized    = false;
/** 当前用于任务调度的刷新周期（系统节拍数）。 */
static TickType_t        s_refreshPeriod  = 0U;

/**
 * @brief 进入模块临界区。
 *
 * 使用互斥量对共享数据进行串行化访问，确保在多任务场景下的线程安全。
 */
static void APP_DRV8316_lock(void)
{
    if(s_mutexHandle != NULL)
    {
        xSemaphoreTake(s_mutexHandle, portMAX_DELAY);
    }
}

/**
 * @brief 离开模块临界区。
 */
static void APP_DRV8316_unlock(void)
{
    if(s_mutexHandle != NULL)
    {
        xSemaphoreGive(s_mutexHandle);
    }
}

/**
 * @brief 获取当前任务刷新周期。
 *
 * 当外部配置未设定时，退回到模块默认刷新周期，以保证任务仍能按固定速率运行。
 */
static TickType_t APP_DRV8316_getRefreshPeriodTicks(void)
{
    TickType_t ticks;

    APP_DRV8316_lock();
    ticks = s_refreshPeriod;
    APP_DRV8316_unlock();

    if(ticks == 0U)
    {
        ticks = pdMS_TO_TICKS(APP_DRV8316_DEFAULT_REFRESH_MS);
    }

    return ticks;
}

/**
 * @brief 初始化应用层模块并与底层驱动建立关联。
 *
 * 步骤包括：
 *  1. 创建互斥量并重置内部状态；
 *  2. 调用底层库生成 DRV8316 句柄；
 *  3. 合并用户配置与默认参数；
 *  4. 将 SPI 资源附着至驱动并完成 SPI 配置；
 *  5. 根据需要自动拉使能脚。
 */
void APP_DRV8316_init(const APP_DRV8316_Config *config)
{
    if(s_mutexHandle == NULL)
    {
        s_mutexHandle = xSemaphoreCreateMutexStatic(&s_mutexBuffer);
    }

    APP_DRV8316_lock();

    s_initialized = false;

    memset(&s_drvObj, 0, sizeof(s_drvObj));
    memset(&s_drvVars, 0, sizeof(s_drvVars));

    s_drvHandle = DRV8316_init(&s_drvObj);

    APP_DRV8316_Config defaultConfig =
    {
        .csGpio            = DEVICE_GPIO_PIN_SPISTEA,
        .enableGpio        = DRV_SPI_INVALID_GPIO,
        .refreshPeriodTicks = pdMS_TO_TICKS(APP_DRV8316_DEFAULT_REFRESH_MS),
        .autoEnable        = false
    };

    if(config != NULL)
    {
        s_runtimeConfig = *config;

        if(s_runtimeConfig.refreshPeriodTicks == 0U)
        {
            s_runtimeConfig.refreshPeriodTicks = pdMS_TO_TICKS(APP_DRV8316_DEFAULT_REFRESH_MS);
        }
    }
    else
    {
        s_runtimeConfig = defaultConfig;
    }

    s_refreshPeriod = s_runtimeConfig.refreshPeriodTicks;

    DRV_SPI_attachToDRV8316(s_drvHandle,
                            s_runtimeConfig.csGpio,
                            s_runtimeConfig.enableGpio);

    DRV8316_setupSPI(s_drvHandle, &s_drvVars);

    if(s_runtimeConfig.autoEnable &&
       (s_runtimeConfig.enableGpio != DRV_SPI_INVALID_GPIO))
    {
        DRV8316_enable(s_drvHandle);
    }

    s_initialized = true;

    APP_DRV8316_unlock();
}

/**
 * @brief 查询模块是否已经完成初始化。
 */
bool APP_DRV8316_isReady(void)
{
    return s_initialized;
}

/**
 * @brief 获取最新的寄存器镜像。
 *
 * 通过互斥量拷贝底层缓存的寄存器结构，供上层进行状态诊断或 UI 显示。
 */
bool APP_DRV8316_getStatusSnapshot(DRV8316_VARS_t *outVars)
{
    if((outVars == NULL) || !s_initialized)
    {
        return false;
    }

    APP_DRV8316_lock();
    *outVars = s_drvVars;
    APP_DRV8316_unlock();

    return true;
}

/**
 * @brief 申请一次控制寄存器更新。
 *
 * 将待写入的控制寄存器值复制到本地缓存，并设置写入标志，由轮询任务在下一周期
 * 内实际完成 SPI 写操作。
 */
bool APP_DRV8316_scheduleControlUpdate(const DRV8316_VARS_t *ctrlRegs)
{
    if((ctrlRegs == NULL) || !s_initialized)
    {
        return false;
    }

    APP_DRV8316_lock();

    s_drvVars.ctrlReg01 = ctrlRegs->ctrlReg01;
    s_drvVars.ctrlReg02 = ctrlRegs->ctrlReg02;
    s_drvVars.ctrlReg03 = ctrlRegs->ctrlReg03;
    s_drvVars.ctrlReg04 = ctrlRegs->ctrlReg04;
    s_drvVars.ctrlReg05 = ctrlRegs->ctrlReg05;
    s_drvVars.ctrlReg06 = ctrlRegs->ctrlReg06;
    s_drvVars.ctrlReg10 = ctrlRegs->ctrlReg10;
    s_drvVars.writeCmd  = true;

    APP_DRV8316_unlock();

    return true;
}

/**
 * @brief 申请手动读取指定地址寄存器。
 *
 * 若当前已有手动读取在执行，则返回失败；否则记录目标地址并设置读取标志，由任务
 * 在下一轮刷新的时候触发底层读取流程。
 */
bool APP_DRV8316_requestManualRead(uint16_t address)
{
    if(!s_initialized || (address > 0x1FU))
    {
        return false;
    }

    APP_DRV8316_lock();

    if(s_drvVars.manReadCmd)
    {
        APP_DRV8316_unlock();
        return false;
    }

    s_drvVars.manReadAddr = address & 0x1FU;
    s_drvVars.manReadCmd  = true;

    APP_DRV8316_unlock();

    return true;
}

/**
 * @brief 读取最近一次手动寄存器查询的结果。
 *
 * 当底层读取尚未完成时，函数返回 false 并提示上层稍后重试。
 */
bool APP_DRV8316_getManualReadResult(uint16_t *data)
{
    if((data == NULL) || !s_initialized)
    {
        return false;
    }

    APP_DRV8316_lock();

    bool busy = s_drvVars.manReadCmd;
    uint16_t value = s_drvVars.manReadData;

    APP_DRV8316_unlock();

    if(busy)
    {
        return false;
    }

    *data = value;
    return true;
}

/**
 * @brief 返回底层驱动句柄，供特殊场景直接调用底层接口。
 */
DRV8316_Handle APP_DRV8316_getHandle(void)
{
    return s_drvHandle;
}

/**
 * @brief DRV8316 周期性维护任务。
 *
 * 任务逻辑：
 *  - 等待模块初始化完成；
 *  - 周期性检查是否有待写入/读取命令；
 *  - 依据驱动库提供的 API 执行写入和读取操作；
 *  - 按照当前配置的刷新周期休眠，确保寄存器数据保持最新。
 */
void APP_DRV8316_TASK(void *pvParameters)
{
    (void)pvParameters;

    TickType_t periodTicks = APP_DRV8316_getRefreshPeriodTicks();

    while(!APP_DRV8316_isReady())
    {
        vTaskDelay(pdMS_TO_TICKS(APP_DRV8316_DEFAULT_REFRESH_MS));
    }

    periodTicks = APP_DRV8316_getRefreshPeriodTicks();
    TickType_t lastWakeTick = xTaskGetTickCount();

    for(;;)
    {
        bool needWrite;

        /*
         * 预读取互斥量中缓存的命令标志。s_drvVars 结构由底层驱动维护，
         * writeCmd/manWriteCmd 标记存在写指令，readCmd 置位后将触发常规刷新。
         */
        APP_DRV8316_lock();
        needWrite = (s_drvVars.writeCmd || s_drvVars.manWriteCmd);
        s_drvVars.readCmd = true;
        APP_DRV8316_unlock();

        if(needWrite)
        {
            APP_DRV8316_lock();
            DRV8316_writeData(s_drvHandle, &s_drvVars);
            APP_DRV8316_unlock();
        }

        /* 执行常规寄存器读取，更新状态镜像和手动读回数据。 */
        APP_DRV8316_lock();
        DRV8316_readData(s_drvHandle, &s_drvVars);
        APP_DRV8316_unlock();

        periodTicks = APP_DRV8316_getRefreshPeriodTicks();
        vTaskDelayUntil(&lastWakeTick, periodTicks);
    }
}
