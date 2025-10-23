//! \file   libraries/drvic/drv8316/source/drv8316s.c
//! \brief  包含与 DRV8316 对象相关的各类函数
//!

// **************************************************************************
// 引用的头文件

#include <math.h>

// **************************************************************************
// 驱动
#include "drv8316s.h"

// **************************************************************************
// 模块

// **************************************************************************
// 平台

// **************************************************************************
// 宏定义

// **************************************************************************
// 全局变量

// **************************************************************************
// 函数原型

DRV8316_Handle DRV8316_init(void *pMemory)
{
    DRV8316_Handle handle;

    // 赋值句柄
    handle = (DRV8316_Handle)pMemory;

    DRV8316_resetRxTimeout(handle);
    DRV8316_resetEnableTimeout(handle);

    return(handle);
} // DRV8316_init() 函数结束

void DRV8316_enable(DRV8316_Handle handle)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;
    volatile uint16_t enableWaitTimeOut;
    uint16_t n = 0;

    // 使能 DRV8316
    GPIO_writePin(obj->gpioNumber_EN, 0);
    GPIO_writePin(obj->gpioNumber_EN, 0);

    // 等待 DRV8316 完成启动流程
    for(n = 0; n < 0xffff; n++)
    {
        __asm(" NOP");
    }

    enableWaitTimeOut = 0;

    // 确保启动期间 FAULT 位未置位
    while(((DRV8316_readSPI(handle, DRV8316_ADDRESS_STATUS_0) &
            DRV8316_STAT00_FAULT_BITS) != 0) && (enableWaitTimeOut < 1000))
    {
        if(++enableWaitTimeOut > 999)
        {
            obj->enableTimeOut = true;
        }
    }

    // 等待 DRV8316 完成启动流程
    for(n = 0; n < 0xffff; n++)
    {
        __asm(" NOP");
    }

    // 向该寄存器写入 011b 以解锁全部寄存器
    DRV8316_writeSPI(handle,  DRV8316_ADDRESS_CONTROL_1, 0x03);

    // 清除故障，转换斜率为 200 V/μs
    DRV8316_writeSPI(handle,  DRV8316_ADDRESS_CONTROL_2, 0x19);

    return;
} // DRV8316_enable() 函数结束

void DRV8316_setSPIHandle(DRV8316_Handle handle, uint32_t spiHandle)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;

    // 初始化串行外设接口对象
    obj->spiHandle = spiHandle;

    return;
} // DRV8316_setSPIHandle() 函数结束

void DRV8316_setGPIOCSNumber(DRV8316_Handle handle, uint32_t gpioNumber)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;

    // 初始化 GPIO 接口对象
    obj->gpioNumber_CS = gpioNumber;

    return;
} // DRV8316_setGPIOCSNumber() 函数结束

void DRV8316_setGPIOENNumber(DRV8316_Handle handle, uint32_t gpioNumber)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;

    // 初始化 GPIO 接口对象
    obj->gpioNumber_EN = gpioNumber;

    return;
} // DRV8316_setGPIOENNumber() 函数结束

void DRV8316_setupSPI(DRV8316_Handle handle,
                      DRV8316_VARS_t *drv8316Vars)
{
    DRV8316_Address_e drvRegAddr;
    uint16_t drvDataNew;

    // 设置默认值
    // 手动读写
    drv8316Vars->manReadAddr  = 0;
    drv8316Vars->manReadData  = 0;
    drv8316Vars->manReadCmd = false;
    drv8316Vars->manWriteAddr = 0;
    drv8316Vars->manWriteData = 0;
    drv8316Vars->manWriteCmd = false;

    // 读写控制
    drv8316Vars->readCmd  = false;
    drv8316Vars->writeCmd = false;

    // 读取寄存器以获得默认值
    // 读取状态寄存器 0
    drvRegAddr = DRV8316_ADDRESS_STATUS_0;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->statReg00.all = drvDataNew;

    // 读取状态寄存器 1
    drvRegAddr = DRV8316_ADDRESS_STATUS_1;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->statReg01.all = drvDataNew;

    // 读取状态寄存器 2
    drvRegAddr = DRV8316_ADDRESS_STATUS_2;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->statReg02.all = drvDataNew;

      // 读取控制寄存器 1
    drvRegAddr = DRV8316_ADDRESS_CONTROL_1;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg01.all = drvDataNew;

    // 读取控制寄存器 2
    drvRegAddr = DRV8316_ADDRESS_CONTROL_2;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg02.all = drvDataNew;

    // 读取控制寄存器 3
    drvRegAddr = DRV8316_ADDRESS_CONTROL_3;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg02.all = drvDataNew;

    // 读取控制寄存器 4
    drvRegAddr = DRV8316_ADDRESS_CONTROL_4;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg04.all = drvDataNew;

    // 读取控制寄存器 5
    drvRegAddr = DRV8316_ADDRESS_CONTROL_5;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg05.all = drvDataNew;

    // 读取控制寄存器 6
    drvRegAddr = DRV8316_ADDRESS_CONTROL_6;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg06.all = drvDataNew;

    // 读取控制寄存器 10
    drvRegAddr = DRV8316_ADDRESS_CONTROL_10;
    drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
    drv8316Vars->ctrlReg10.all = drvDataNew;

    return;
} // DRV8316_setupSPI() 函数结束

uint16_t DRV8316_readSPI(DRV8316_Handle handle,
                         const DRV8316_Address_e regAddr)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;
    uint16_t ctrlWord;
    uint16_t n;
    const uint16_t data = 0;
    volatile uint16_t readWord;
    volatile uint16_t WaitTimeOut = 0;

    volatile SPI_RxFIFOLevel RxFifoCnt = SPI_FIFO_RXEMPTY;

    // build the control word
    ctrlWord = (uint16_t)DRV8316_buildCtrlWord(DRV8316_CTRLMODE_READ, regAddr, data);

#ifdef DRV_CS_GPIO
    GPIO_writePin(obj->gpioNumber_CS, 0);
    GPIO_writePin(obj->gpioNumber_CS, 0);
#endif  // DRV_CS_GPIO

    // wait for registers to update
    for(n = 0; n < 0x08; n++)
    {
        __asm(" NOP");
    }

    // reset the Rx fifo pointer to zero
    SPI_resetRxFIFO(obj->spiHandle);
    SPI_enableFIFO(obj->spiHandle);

    // wait for registers to update
    for(n = 0; n < 0x20; n++)
    {
        __asm(" NOP");
    }

    // write the command
    SPI_writeDataBlockingNonFIFO(obj->spiHandle, ctrlWord);

    // wait for two words to populate the RX fifo, or a wait timeout will occur
    while(RxFifoCnt < SPI_FIFO_RX1)
    {
        RxFifoCnt = SPI_getRxFIFOStatus(obj->spiHandle);

        if(++WaitTimeOut > 0xfffe)
        {
            obj->rxTimeOut = true;
        }
    }

    WaitTimeOut = 0xffff;

    // wait for registers to update
    for(n = 0; n < 0x100; n++)
    {
        __asm(" NOP");
    }

#ifdef DRV_CS_GPIO
    GPIO_writePin(obj->gpioNumber_CS, 1);
    GPIO_writePin(obj->gpioNumber_CS, 1);
#endif  // DRV_CS_GPIO

    // Read the word
    readWord = SPI_readDataNonBlocking(obj->spiHandle);

    return(readWord & DRV8316_DATA_MASK);
} // DRV8316_readSPI() 函数结束


void DRV8316_writeSPI(DRV8316_Handle handle, const DRV8316_Address_e regAddr,
                      const uint16_t data)
{
    DRV8316_Obj *obj = (DRV8316_Obj *)handle;
    uint16_t ctrlWord;
    uint16_t n;

    // build the control word
    ctrlWord = (uint16_t)DRV8316_buildCtrlWord(DRV8316_CTRLMODE_WRITE, regAddr, data);

#ifdef DRV_CS_GPIO
    GPIO_writePin(obj->gpioNumber_CS, 0);
    GPIO_writePin(obj->gpioNumber_CS, 0);
#endif  // DRV_CS_GPIO

    // wait for GPIO
    for(n = 0; n < 0x08; n++)
    {
        __asm(" NOP");
    }

    // reset the Rx fifo pointer to zero
    SPI_resetRxFIFO(obj->spiHandle);
    SPI_enableFIFO(obj->spiHandle);

    // wait for registers to update
    for(n = 0; n < 0x40; n++)
    {
        __asm(" NOP");
    }

    // write the command
    SPI_writeDataBlockingNonFIFO(obj->spiHandle, ctrlWord);

    // wait for registers to update
    for(n = 0; n < 0x100; n++)
    {
        __asm(" NOP");
    }

#ifdef DRV_CS_GPIO
    GPIO_writePin(obj->gpioNumber_CS, 1);
    GPIO_writePin(obj->gpioNumber_CS, 1);
#endif  // DRV_CS_GPIO

    return;
}  // DRV8316_writeSPI() 函数结束


void DRV8316_writeData(DRV8316_Handle handle, DRV8316_VARS_t *drv8316Vars)
{
    DRV8316_Address_e drvRegAddr;
    uint16_t drvDataNew;

    if(drv8316Vars->writeCmd)
    {
        // Write Control Register 1
        drvRegAddr = DRV8316_ADDRESS_CONTROL_1;
        drvDataNew = drv8316Vars->ctrlReg01.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        // Write Control Register 2
        drvRegAddr = DRV8316_ADDRESS_CONTROL_2;
        drvDataNew = drv8316Vars->ctrlReg02.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        // Write Control Register 3
        drvRegAddr = DRV8316_ADDRESS_CONTROL_3;
        drvDataNew = drv8316Vars->ctrlReg03.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        // Write Control Register 4
        drvRegAddr = DRV8316_ADDRESS_CONTROL_4;
        drvDataNew = drv8316Vars->ctrlReg04.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        // Write Control Register 5
        drvRegAddr = DRV8316_ADDRESS_CONTROL_5;
        drvDataNew = drv8316Vars->ctrlReg05.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        // Write Control Register 6
        drvRegAddr = DRV8316_ADDRESS_CONTROL_6;
        drvDataNew = drv8316Vars->ctrlReg06.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        // Write Control Register 10
        drvRegAddr = DRV8316_ADDRESS_CONTROL_10;
        drvDataNew = drv8316Vars->ctrlReg10.all & DRV8316_DATA_MASK;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        drv8316Vars->writeCmd = false;
    }

    // Manual write to the DRV8316
    if(drv8316Vars->manWriteCmd)
    {
        // Custom Write
        drvRegAddr = (DRV8316_Address_e)(drv8316Vars->manWriteAddr << 11);
        drvDataNew = drv8316Vars->manWriteData;
        DRV8316_writeSPI(handle, drvRegAddr, drvDataNew);

        drv8316Vars->manWriteCmd = false;
    }

    return;
}  // DRV8316_writeData() 函数结束

void DRV8316_readData(DRV8316_Handle handle, DRV8316_VARS_t *drv8316Vars)
{
    DRV8316_Address_e drvRegAddr;
    uint16_t drvDataNew;

    if(drv8316Vars->readCmd)
    {
        // 读取寄存器以获得默认值
        // 读取状态寄存器 0
        drvRegAddr = DRV8316_ADDRESS_STATUS_0;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->statReg00.all  = drvDataNew;

        // 读取状态寄存器 1
        drvRegAddr = DRV8316_ADDRESS_STATUS_1;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->statReg01.all  = drvDataNew;

        // 读取状态寄存器 2
        drvRegAddr = DRV8316_ADDRESS_STATUS_2;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->statReg02.all  = drvDataNew;

        // 读取控制寄存器 1
        drvRegAddr = DRV8316_ADDRESS_CONTROL_1;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg01.all  = drvDataNew;

        // 读取控制寄存器 2
        drvRegAddr = DRV8316_ADDRESS_CONTROL_2;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg02.all  = drvDataNew;

        // 读取控制寄存器 3
        drvRegAddr = DRV8316_ADDRESS_CONTROL_3;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg03.all  = drvDataNew;

        // 读取控制寄存器 4
        drvRegAddr = DRV8316_ADDRESS_CONTROL_4;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg04.all  = drvDataNew;

        // 读取控制寄存器 5
        drvRegAddr = DRV8316_ADDRESS_CONTROL_5;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg05.all  = drvDataNew;

        // 读取控制寄存器 6
        drvRegAddr = DRV8316_ADDRESS_CONTROL_6;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg06.all  = drvDataNew;

        // 读取控制寄存器 10
        drvRegAddr = DRV8316_ADDRESS_CONTROL_10;
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->ctrlReg10.all  = drvDataNew;

        drv8316Vars->readCmd = false;
    }

    // Manual read from the DRV8316
    if(drv8316Vars->manReadCmd)
    {
        // Custom Read
        drvRegAddr = (DRV8316_Address_e)(drv8316Vars->manReadAddr << 11);
        drvDataNew = DRV8316_readSPI(handle, drvRegAddr);
        drv8316Vars->manReadData = drvDataNew;

        drv8316Vars->manReadCmd = false;
    }

    return;
}  // DRV8316_readData() 函数结束

// end of file
