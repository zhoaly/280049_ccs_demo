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

#ifndef C2000_FREERTOS_H
#define C2000_FREERTOS_H

//
// Includes
//
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "driverlib.h"

extern uint8_t ucHeap[ 1024 ];

//
// Idle task specific macros
//
#define IDLE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

//
// Idle task specific variables
//
extern StaticTask_t idleTaskTCBBuffer;
extern StackType_t idleTaskStack[IDLE_TASK_STACK_SIZE];

//
// Macro for task stack size
//
#define redTask_STACK_SIZE 256

//
// Variables for Task TCB and stack
//
extern StaticTask_t redTaskTCBBuffer;
extern StackType_t redTaskStackBuffer[redTask_STACK_SIZE];

//
// Task Handle for task redTask
//
extern TaskHandle_t redTaskHandle;

//
// Macro for task stack size
//
#define blueTask_STACK_SIZE 256

//
// Variables for Task TCB and stack
//
extern StaticTask_t blueTaskTCBBuffer;
extern StackType_t blueTaskStackBuffer[blueTask_STACK_SIZE];

//
// Task Handle for task blueTask
//
extern TaskHandle_t blueTaskHandle;

//
// Declare a variable to hold the handle of the created semaphore.
//
extern SemaphoreHandle_t  binarySem1Handle;

//
// Declare a variable to hold data associated with the created static semaphore.
//
extern StaticSemaphore_t binarySem1Buffer;


//
// Declaration for redTask task function. Application needs to implement this.
//
extern void LED_TaskRed( void * pvParameters );

//
// Declaration for blueTask task function. Application needs to implement this.
//
extern void LED_TaskBlue( void * pvParameters );

//
// Init Functions
//
void redTask_init();
void blueTask_init();
void binarySem1_init();
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    configSTACK_DEPTH_TYPE *pulIdleTaskStackSize );
void vApplicationSetupTimerInterrupt( void );
void FreeRTOS_init();

#endif /* C2000_FREERTOS_H */

