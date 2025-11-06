/**
 * @file app_FOC.c
 * @brief FOC控制逻辑实现
 */

#include "app_FOC.h"
#include "drv_epwm.h"

#include "c2000_freertos.h"
#include "device.h"
#include "driverlib/gpio.h"
#include "driverlib/epwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"

#define     DEFAULT_FOC_PWM_DUTY           (0U)
#define     DEFAULT_FOC_PWM_DEADBAND       (0U)
#define     DEFAULT_FOC_PWM_FREQUENCY      (10000U)

//FOC主函数
void FOC_Task_Func(void * pvParameters){
    (void) pvParameters;
    
    FOC_init();//初始化

    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}

bool FOC_init(void){

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
