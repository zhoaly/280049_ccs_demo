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

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// Kernel configuration
#define configCPU_CLOCK_HZ                        ( ( unsigned long ) 100000000 )
#define configTICK_RATE_HZ                        ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                      16
#define configMINIMAL_STACK_SIZE                  ( ( unsigned short ) 256 )
#define configMAX_TASK_NAME_LEN                   16
#define configQUEUE_REGISTRY_SIZE                 10
#define configTASK_NOTIFICATION_ARRAY_ENTRIES     3
#define configUSE_PREEMPTION                      1
#define configUSE_TIME_SLICING                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION   0
#define configUSE_TICKLESS_IDLE                   0
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP     2
#define configUSE_16_BIT_TICKS                    0
#define configIDLE_SHOULD_YIELD                   0
#define configUSE_TASK_NOTIFICATIONS              1
#define configUSE_MUTEXES                         1
#define configUSE_RECURSIVE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES             1
#define configUSE_QUEUE_SETS                      0
#define configENABLE_BACKWARD_COMPATIBILITY       0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS   0
#define configUSE_MINI_LIST_ITEM                  1
#define configRECORD_STACK_HIGH_ADDRESS           0

// Memory allocation related configurations
#define configSUPPORT_STATIC_ALLOCATION           1
#define configSUPPORT_DYNAMIC_ALLOCATION          1
#define configAPPLICATION_ALLOCATED_HEAP          1
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP 0
#define configTOTAL_HEAP_SIZE                     ( ( size_t ) ( 1024 ) )

// Hooks/Callbacks related configurations
#define configUSE_IDLE_HOOK                       0
#define configUSE_TICK_HOOK                       0
#define configCHECK_FOR_STACK_OVERFLOW            0
#define configUSE_MALLOC_FAILED_HOOK              0
#define configUSE_DAEMON_TASK_STARTUP_HOOK        0

// Run time statistics related configurations
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY      0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0

// Co-routines specific configurations
#define configUSE_CO_ROUTINES  0
#define configMAX_CO_ROUTINE_PRIORITIES 0

// Software Timers specific configurations
#define configUSE_TIMERS 0
#define configTIMER_TASK_PRIORITY 3
#define configTIMER_QUEUE_LENGTH 10
#define configTIMER_TASK_STACK_DEPTH 0

// Optional APIs inclusion-specific configurations
#define INCLUDE_vTaskPrioritySet                  0
#define INCLUDE_uxTaskPriorityGet                 0
#define INCLUDE_vTaskDelete                       0
#define INCLUDE_vTaskSuspend                      0
#define INCLUDE_xResumeFromISR                    0
#define INCLUDE_vTaskDelayUntil                   1
#define INCLUDE_vTaskDelay                        1
#define INCLUDE_xTaskGetSchedulerState            0
#define INCLUDE_xTaskGetCurrentTaskHandle         1
#define INCLUDE_uxTaskGetStackHighWaterMark       0
#define INCLUDE_uxTaskGetStackHighWaterMark2      0
#define INCLUDE_xTaskGetIdleTaskHandle            0
#define INCLUDE_eTaskGetState                     0
#define INCLUDE_xEventGroupSetBitFromISR          0
#define INCLUDE_xTimerPendFunctionCall            0
#define INCLUDE_xTaskAbortDelay                   0
#define INCLUDE_xTaskGetHandle                    0
#define INCLUDE_xTaskResumeFromISR                0

// Set the following definitions to 1 to include the API function, or zero
// to exclude the API function.


#endif /* FREERTOS_CONFIG_H */

