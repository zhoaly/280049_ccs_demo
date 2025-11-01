/**
 * @file app_drv8316.c
 * @brief DRV8316 应用层管理模块，实现周期性读取与配置接口。
 */

#include "app_drv8316.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "device.h"

#include "drv8316s.h"

static DRV8316_Obj     s_drvObj;
static DRV8316_Handle  s_drvHandle = NULL;
static DRV8316_VARS_t  s_drvVars;
static APP_DRV8316_Config s_runtimeConfig;

static StaticSemaphore_t s_mutexBuffer;
static SemaphoreHandle_t s_mutexHandle = NULL;

static volatile bool     s_initialized    = false;
static TickType_t        s_refreshPeriod  = 0U;

static void APP_DRV8316_lock(void)
{
    if(s_mutexHandle != NULL)
    {
        xSemaphoreTake(s_mutexHandle, portMAX_DELAY);
    }
}

static void APP_DRV8316_unlock(void)
{
    if(s_mutexHandle != NULL)
    {
        xSemaphoreGive(s_mutexHandle);
    }
}

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

bool APP_DRV8316_isReady(void)
{
    return s_initialized;
}

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

DRV8316_Handle APP_DRV8316_getHandle(void)
{
    return s_drvHandle;
}

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

        APP_DRV8316_lock();
        DRV8316_readData(s_drvHandle, &s_drvVars);
        APP_DRV8316_unlock();

        periodTicks = APP_DRV8316_getRefreshPeriodTicks();
        vTaskDelayUntil(&lastWakeTick, periodTicks);
    }
}
