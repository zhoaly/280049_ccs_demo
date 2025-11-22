//分支说明********************************************/
//当前为大功率驱动版本分支
//使用BTN7971驱动方案,不使用DRV8316驱动,
// sys include********************************************/
#include "driverlib.h"
#include "device.h"
#include "FreeRTOS.h"
#include "board.h"
#include "c2000_freertos.h"
// user include********************************************/
#include "drv_epwm.h"
#include "drv_eqep.h"
#include "drv_spi.h"

DRV_EPWM_State epwmstate0 = {};
DRV_EQEP_State eqepstate0 = {};

//
// 函数原型
//
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationMallocFailedHook( void );
void ePWMConfigurationTemplate(uint32_t base);


void myTask0_func(void * pvParameters);
void FOC_Task_Func(void * pvParameters);
//
// Timer1 中断服务程序
//
__interrupt void timer1_ISR( void );
__interrupt void timer0_ISR( void );
__interrupt void INT_mySCI0_RX_ISR( void );
__interrupt void INT_mySCI0_TX_ISR( void );
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

    EALLOW;//外设配置必须在rtosinit前??
    //DRV_SPI_init();
    DRV_EPWM_init();
    DRV_EQEP_init();
    //ePWMConfigurationTemplate(EPWM1_BASE);
    //GPIO_writePin(myLED1_GPIO,1);

    EDIS;


    // 配置 FreeRTOS
    FreeRTOS_init();

    //以下为业务代码********************************************/
    //驱动初始化************************************************/


    
    EINT;
    ERTM;


    
    while(1)
    {    // 正常情况下永远不会执行。
    }
}



void myTask0_func(void * pvParameters){
    (void) pvParameters;

    while (1) {
        
        //  if(xSemaphoreTake(TimeBase_SemaphoreHandle, portMAX_DELAY) == pdTRUE)//1ms
        // {
        //     // 收到通知后执行相应处理
        //     GPIO_togglePin(myLED2_GPIO);//计时器运行正常标志
        // }
        
        vTaskDelay(pdTICKS_TO_MS(1000));

        //GPIO_togglePin(myLED1_GPIO);//rtos运行正常标志
        // DRV_EQEP_update(0.01f); // 默认仅刷新角度信息，速度可在传入采样周期后获取
        // DRV_EQEP_getState(&eqepstate0);
    }
}

//
// Timer0 中断服务程序
//
//可以作为时基?
__interrupt void timer0_ISR( void )//100ms触发
{

    GPIO_togglePin(myLED1_GPIO);//计时器运行正常标志
    //DRV_EPWM_getState(&epwmstate0);

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);

}

//
// Timer1 中断服务程序
//
//可以作为时基?
__interrupt void timer1_ISR( void )//1ms触发
{

    //GPIO_togglePin(myLED2_GPIO);//计时器运行正常标志
    
    xSemaphoreGive(TimeBase_SemaphoreHandle);//1ms信号量
    //DRV_EPWM_getState(&epwmstate0);


}

__interrupt void INT_mySCI0_RX_ISR( void )
{
    SCI_clearInterruptStatus(mySCI0_BASE,
                            SCI_INT_RXFF | SCI_INT_FE | SCI_INT_OE |
                            SCI_INT_PE   | SCI_INT_RXERR);

    // 清 PIE Group9 的 ACK（SCIA 的中断在 Group9）
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}
__interrupt void INT_mySCI0_TX_ISR( void )
{
    // 清除 TX FIFO 中断标志
    SCI_clearInterruptStatus(mySCI0_BASE, SCI_INT_TXFF);

    // 清 PIE Group9 ACK
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
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

    GPIO_writePin(myLED1_GPIO,1);
    GPIO_writePin(myLED2_GPIO,1);//栈溢出标志 两灯常亮
    
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
