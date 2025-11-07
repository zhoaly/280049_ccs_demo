/**
 * @file app_FOC.h
 * @brief FOC控制逻辑实现
 */

#ifndef APP_FOC_H
#define APP_FOC_H

#include <stdint.h>
#include <stdbool.h>


#define PI 3.14159

#define _LIMITER(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))//限幅

void FOC_Task_Func(void * pvParameters);
void FOC_SetPhaseVoltage(float Ua, float Ub, float Uc) ;


static bool FOC_init(void);

static float _electricalAngle(float shaft_angle, int pole_pairs);
static float _normalizeAngle(float angle);




#endif /* APP_FOC_H */
