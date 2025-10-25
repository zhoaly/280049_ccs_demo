// sys include********************************************/
#include "driverlib.h"
#include "device.h"
#include "FreeRTOS.h"
#include "board.h"
#include "c2000_freertos.h"
// user include********************************************/
#include "drv_epwm.h"


// user d********************************************/
#define RED         0xDEADBEAF
#define BLUE        0xBAADF00D

//
// 函数原型
//
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationMallocFailedHook( void );

//
// Timer1 中断服务程序
//
__interrupt void timer1_ISR(void);

//
// 主函数
//
void main(void)
{
    // 初始化器件时钟和外设
    Device_init();


    // 初始化 PIE 并清除 PIE 寄存器，禁用 CPU 中断。
    Interrupt_initModule();

    //初始化GPIO
    Device_initGPIO();

    //
    // 禁用所有 CPU 中断并清除所有 CPU 中断标志。
    //
    DINT;
    IER = 0x0000;
    IFR = 0x0000;

    //
    // 使用指向默认中断服务程序 (ISR) 的指针初始化 PIE 向量表。
    //
    Interrupt_initVectorTable();

    //以下代码由syscfg生成**************************************/
    // 配置 CPUTimer1 和 LED。
    Board_init();
    // 配置 FreeRTOS
    FreeRTOS_init();

    //以下为业务代码********************************************/
    //驱动初始化************************************************/
    DRV_EPWM_init();

    while(1)
    {    // 正常情况下永远不会执行。
    }
}

//
// Timer1 中断服务程序 - 为红色任务释放信号量
//
//可以作为时基?
__interrupt void timer1_ISR( void )
{

}


//
// vApplicationStackOverflowHook - 检查运行时堆栈溢出
//
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* 当 configCHECK_FOR_STACK_OVERFLOW 定义为 1 或 2 时执行运行时堆栈溢出检查。
    如果检测到堆栈溢出，将调用此钩子函数。 */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

//
// vApplicationMallocFailedHook - 捕获 pvPortMalloc() 失败的钩子函数
//
void vApplicationMallocFailedHook( void )
{
    /* 只有当 FreeRTOSConfig.h 中的 configUSE_MALLOC_FAILED_HOOK 设为 1 时，
    才会调用 vApplicationMallocFailedHook()。该钩子函数会在调用
    pvPortMalloc() 失败时被触发。内核在创建任务、队列、定时器或信号量时都会
    内部调用 pvPortMalloc()，演示程序的各个部分也会调用它。如果使用 heap_1.c
    或 heap_2.c，pvPortMalloc() 可用的堆大小由 FreeRTOSConfig.h 中的
    configTOTAL_HEAP_SIZE 定义，可以使用 xPortGetFreeHeapSize() API 函数查询
    剩余的堆空间大小（但该函数无法提供剩余堆空间碎片情况的信息）。 */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}


