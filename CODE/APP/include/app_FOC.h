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
void FOC_ClarkeTransform(float Ia, float Ib, float Ic, float *Ialpha, float *Ibeta);
void FOC_ParkTransform(float Ialpha, float Ibeta, float angle_el, float *Id, float *Iq);
void FOC_InverseClarkeTransform(float Ialpha, float Ibeta, float *Ia, float *Ib, float *Ic);
void FOC_InverseParkTransform(float Id, float Iq, float angle_el, float *Ialpha, float *Ibeta);


static bool FOC_init(void);

static float _electricalAngle(float shaft_angle, int pole_pairs);
static float _normalizeAngle(float angle);




#endif /* APP_FOC_H */
