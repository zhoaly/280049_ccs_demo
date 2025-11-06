/**
 * @file app_FOC.h
 * @brief FOC控制逻辑实现
 */

#ifndef APP_FOC_H
#define APP_FOC_H

#include <stdint.h>
#include <stdbool.h>

void FOC_Task_Func(void * pvParameters);
bool FOC_init(void);

#endif /* APP_FOC_H */
