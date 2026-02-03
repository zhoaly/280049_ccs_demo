// Branch notes ********************************************/
// Protocol development test branch
// sys include ********************************************/
#include "__INCLUDE.h"


static const char * TAG ="main";


DRV_EPWM_State epwmstate0 = {};
DRV_EQEP_State eqepstate0 = {};

// Function prototypes
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);
void vApplicationMallocFailedHook( void );
void ePWMConfigurationTemplate(uint32_t base);
static void APP_SemaphoreBatchGive(void);


static void APP_SemaphoreBatchGive(void)
{
    /* Must be called after FreeRTOS_init */
    xSemaphoreGive(SCI0Tx_SemaphoreHandle);
    xSemaphoreGive(SPI0Tx_SemaphoreHandle);
}

void myTask0_func(void * pvParameters);
void FOC_Task_Func(void * pvParameters);
//
// Timer1/0 ISRs
//
__interrupt void timer1_ISR( void );
__interrupt void timer0_ISR( void );
//
// Main
//
void main(void)
{
    // Init device clock and peripherals
    Device_init();


    // Init PIE and clear PIE regs; disable CPU interrupts
    Interrupt_initModule();

    // Init GPIO
    Device_initGPIO();

    //
    // Disable all CPU interrupts and clear flags
    //
    DINT;
    IER = 0x0000;
    IFR = 0x0000;

    //
    // Init PIE vector table with default ISRs
    //
    Interrupt_initVectorTable();

    // SysCfg generated init
    Board_init();

    EALLOW; // Peripheral config before FreeRTOS_init
    DRV_SPI_init();
    // DRV_EPWM_init();
    // DRV_EQEP_init();
    DRV_SCI_init();
    //ePWMConfigurationTemplate(EPWM1_BASE);
    // GPIO_writePin(myLED1_GPIO,1);

    EDIS;

    // Init FreeRTOS
    FreeRTOS_init();

    EINT;
    ERTM;

    while(1)
    {
        // Should never reach here
    }
}



void myTask0_func(void * pvParameters)
{
    (void) pvParameters;
    const uint16_t testPayload[] = { 'T', 'E', 'S', 'T' };
    APP_PROTO_Ctx *ctx = (APP_PROTO_Ctx *)0;
    APP_SemaphoreBatchGive();

    while (1) {
        vTaskDelay(pdTICKS_TO_MS(1000));

        ctx = APP_PROTO_GetChannelCtx(APP_PROTO_CH1);
        if (ctx != (APP_PROTO_Ctx *)0)
        {
            // APP_PROTO_SlaveWrite(ctx,
            //                      testPayload,
            //                      (uint16_t)(sizeof(testPayload) / sizeof(testPayload[0])));
             APP_PROTO_MasterWrite(ctx,
                                 testPayload,
                                 (uint16_t)(sizeof(testPayload) / sizeof(testPayload[0])));
        }
        //等待ACK
        // if(xSemaphoreTake(PROTO_ACKHandle, portMAX_DELAY) == pdTRUE;){

            
        // }
        
        


        APP_LOGI0(TAG, "task running \n");

        GPIO_togglePin(myLED2_GPIO); // RTOS heartbeat

    }
}

//
// Timer0 ISR (100ms)
//
__interrupt void timer0_ISR( void )
{

    // GPIO_togglePin(myLED1_GPIO); // timer heartbeat

    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//
// Timer1 ISR (1ms)
//
__interrupt void timer1_ISR( void )
{

    // GPIO_togglePin(myLED2_GPIO); // timer heartbeat
    
    // xSemaphoreGive(TimeBase_SemaphoreHandle); // 1ms signal
    //DRV_EPWM_getState(&epwmstate0);


}

//
// vApplicationStackOverflowHook - stack overflow handler
//
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    /* Called when stack overflow is detected (configCHECK_FOR_STACK_OVERFLOW). */

    // GPIO_writePin(myLED1_GPIO,0);
    GPIO_writePin(myLED2_GPIO,0); // overflow indicator
    
    taskDISABLE_INTERRUPTS();
    for( ;; );
}

//
// vApplicationMallocFailedHook - pvPortMalloc failure hook
//
void vApplicationMallocFailedHook( void )
{
    /* Called on pvPortMalloc() failure when configUSE_MALLOC_FAILED_HOOK=1. */
    taskDISABLE_INTERRUPTS();
    for( ;; );
}



