/*
 * Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

//
// Includes
//
#include "c2000_freertos.h"

uint8_t ucHeap[ 1024 ];
#pragma DATA_SECTION(ucHeap,   ".freertosHeap")
#pragma DATA_ALIGN ( ucHeap , portBYTE_ALIGNMENT )

StaticTask_t idleTaskTCBBuffer;
StackType_t idleTaskStack[IDLE_TASK_STACK_SIZE];
#pragma DATA_SECTION(idleTaskStack, ".freertosStaticStack")
#pragma DATA_ALIGN ( idleTaskStack , portBYTE_ALIGNMENT )

//
// Variables for redTask TCB and stack
//
StaticTask_t redTaskTCBBuffer;
StackType_t redTaskStackBuffer[redTask_STACK_SIZE];
#pragma DATA_SECTION(redTaskStackBuffer, ".freertosStaticStack")
#pragma DATA_ALIGN ( redTaskStackBuffer , portBYTE_ALIGNMENT )

//
// Task Handle for redTask
//
TaskHandle_t redTaskHandle = NULL;

//
// Variables for blueTask TCB and stack
//
StaticTask_t blueTaskTCBBuffer;
StackType_t blueTaskStackBuffer[blueTask_STACK_SIZE];
#pragma DATA_SECTION(blueTaskStackBuffer, ".freertosStaticStack")
#pragma DATA_ALIGN ( blueTaskStackBuffer , portBYTE_ALIGNMENT )

//
// Task Handle for blueTask
//
TaskHandle_t blueTaskHandle = NULL;

//
// Declare a variable to hold the handle of the created semaphore.
//
SemaphoreHandle_t  binarySem1Handle = NULL;

//
// Declare a variable to hold data associated with the created static semaphore.
//
StaticSemaphore_t binarySem1Buffer;

//
// redTask_init() - Initializes task redTask
//
void redTask_init() {
    //
    // Create the task with static memory allocation.
    //
    redTaskHandle =
    xTaskCreateStatic(LED_TaskRed,          // Function that implements the task.
                      "redTask",              // Text name for the task.
                      256,        // Number of indexes in the xStack array.
                      (void *) 0xDEADBEAF,  // Parameter passed into the task.
                      2,         // Priority at which the task is created.
                      redTaskStackBuffer,      // Array to use as the task's stack.
                      &redTaskTCBBuffer );  // Variable to hold the task's TCB
}

//
// blueTask_init() - Initializes task blueTask
//
void blueTask_init() {
    //
    // Create the task with static memory allocation.
    //
    blueTaskHandle =
    xTaskCreateStatic(LED_TaskBlue,          // Function that implements the task.
                      "blueTask",              // Text name for the task.
                      256,        // Number of indexes in the xStack array.
                      (void *) 0xBAADF00D,  // Parameter passed into the task.
                      1,         // Priority at which the task is created.
                      blueTaskStackBuffer,      // Array to use as the task's stack.
                      &blueTaskTCBBuffer );  // Variable to hold the task's TCB
}

//
// binarySem1_init() - Initializes semaphore binarySem1
//
void binarySem1_init() {
    //
    // Create the binary semaphore with static memory allocation.
    //
    binarySem1Handle =
    xSemaphoreCreateBinaryStatic(&binarySem1Buffer);

}

//
// vApplicationGetIdleTaskMemory - Application must provide an implementation
// of vApplicationGetIdleTaskMemory() to provide the memory that is used by the
// Idle task if configUSE_STATIC_ALLOCATION is set to 1.
//
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &idleTaskTCBBuffer;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = idleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t, size is
    specified in words, not bytes. */
    *pulIdleTaskStackSize = IDLE_TASK_STACK_SIZE;
}




//
// FreeRTOS_init - Initializes added FreeRTOS constructs and starts the scheduler
//
void FreeRTOS_init(){
    redTask_init();
    blueTask_init();
    binarySem1_init();

    //
    // Start the scheduler
    //
    vTaskStartScheduler();
}
