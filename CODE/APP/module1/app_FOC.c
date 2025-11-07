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


void FOC_SetAlphaBetaVoltage(float Uq,float Ud, float angle_el) {
    angle_el = _normalizeAngle(angle_el + zero_electric_angle);
    // 帕克逆变换
    Ualpha =  -Uq*sin(angle_el); 
    Ubeta =   Uq*cos(angle_el); 

}





