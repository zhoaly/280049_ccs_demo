/**
 * @file app_FOC.c
 * @brief FOC控制逻辑实现
 */

#include "app_FOC.h"
#include "drv_epwm.h"

#include "c2000_freertos.h"
#include "device.h"
#include "math.h"
#include "driverlib/gpio.h"
#include "driverlib/epwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"


#define     DEFAULT_FOC_PWM_DUTY           (0U)
#define     DEFAULT_FOC_PWM_DEADBAND       (0U)
#define     DEFAULT_FOC_PWM_FREQUENCY      (10000U)

#define     VOLTAGE_POWER_SUPPLY           (12.0f)
#define     SQRT3_F                        (1.73205080757f)
#define     INV_SQRT3_F                    (0.57735026919f)

//FOC主函数
void FOC_Task_Func(void * pvParameters){
    (void) pvParameters;
    
    FOC_init();//初始化

    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}

static bool FOC_init(void){

    uint16_t index;
    bool ret;

    ret = DRV_EPWM_setDeadbandCounts(DEFAULT_FOC_PWM_DEADBAND, DEFAULT_FOC_PWM_DEADBAND);
    if (! ret) {
        return false;
    }
    ret = DRV_EPWM_setFrequency(DEFAULT_FOC_PWM_FREQUENCY);//设置默认频率
    if (! ret) {
        return false;
    }

    for (index = 0; index < DRV_EPWM_CHANNEL_COUNT; index++) {
        ret = DRV_EPWM_setDutyCycle(index,DEFAULT_FOC_PWM_DUTY);//设置默认占空比
        if (! ret) {
            return false;
        }
    }

    return true;    
}


// 电角度求解
static float _electricalAngle(float shaft_angle, int pole_pairs) {
  return (shaft_angle * pole_pairs);
}


// 归一化角度到 [0,2PI]
static float _normalizeAngle(float angle){
  float a = fmod(angle, 2*PI);   //取余运算可以用于归一化，列出特殊值例子算便知
  return a >= 0 ? a : (a + 2*PI);  
}

// 设置PWM到控制器输出
//输入电压值
void FOC_SetPhaseVoltage(float Ua, float Ub, float Uc) {


    float duty_a,duty_b,duty_c;
    
    // 计算占空比
    // 限制占空比从0到1
    duty_a = _LIMITER(Ua / VOLTAGE_POWER_SUPPLY, 0.0f , 1.0f );
    duty_b = _LIMITER(Ub / VOLTAGE_POWER_SUPPLY, 0.0f , 1.0f );
    duty_c = _LIMITER(Uc / VOLTAGE_POWER_SUPPLY, 0.0f , 1.0f );

    //写入PWM
    DRV_EPWM_setDutyCycle(0,duty_a);
    DRV_EPWM_setDutyCycle(1,duty_b);
    DRV_EPWM_setDutyCycle(2,duty_c);

}


void FOC_ClarkeTransform(float Ia, float Ib, float Ic, float *Ialpha, float *Ibeta) {
    (void)Ic; // 三相对称系统中Ic可以由Ia和Ib推导，此处保留参数以兼容接口

    if (Ialpha != NULL) {
        *Ialpha = Ia;
    }
    if (Ibeta != NULL) {
        *Ibeta = (Ia + 2.0f * Ib) * INV_SQRT3_F;
    }
}


void FOC_ParkTransform(float Ialpha, float Ibeta, float angle_el, float *Id, float *Iq) {
    float sin_angle;
    float cos_angle;

    sin_angle = sinf(angle_el);
    cos_angle = cosf(angle_el);

    if (Id != NULL) {
        *Id = Ialpha * cos_angle + Ibeta * sin_angle;
    }
    if (Iq != NULL) {
        *Iq = -Ialpha * sin_angle + Ibeta * cos_angle;
    }
}


void FOC_InverseClarkeTransform(float Ialpha, float Ibeta, float *Ia, float *Ib, float *Ic) {
    float ib_temp;
    float ic_temp;

    ib_temp = (-Ialpha + SQRT3_F * Ibeta) * 0.5f;
    ic_temp = (-Ialpha - SQRT3_F * Ibeta) * 0.5f;

    if (Ia != NULL) {
        *Ia = Ialpha;
    }
    if (Ib != NULL) {
        *Ib = ib_temp;
    }
    if (Ic != NULL) {
        *Ic = ic_temp;
    }
}


void FOC_InverseParkTransform(float Id, float Iq, float angle_el, float *Ialpha, float *Ibeta) {
    float sin_angle;
    float cos_angle;

    sin_angle = sinf(angle_el);
    cos_angle = cosf(angle_el);

    if (Ialpha != NULL) {
        *Ialpha = Id * cos_angle - Iq * sin_angle;
    }
    if (Ibeta != NULL) {
        *Ibeta = Id * sin_angle + Iq * cos_angle;
    }
}


void FOC_SetAlphaBetaVoltage(float Uq,float Ud, float angle_el) {
    angle_el = _normalizeAngle(angle_el + zero_electric_angle);
    // 帕克逆变换
    Ualpha =  -Uq*sin(angle_el); 
    Ubeta =   Uq*cos(angle_el); 

}





