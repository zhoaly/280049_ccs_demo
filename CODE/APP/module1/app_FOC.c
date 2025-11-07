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

#define     SQRT3_F                        (1.73205080757f)
#define     INV_SQRT3_F                    (0.57735026919f)

static FOC_Handle s_focHandle;

/**
 * @brief 软限幅函数，确保输入值位于指定范围内。
 *
 * @param[in] value     原始值。
 * @param[in] minValue  最小允许值。
 * @param[in] maxValue  最大允许值。
 *
 * @return 钳制后的数值。
 */
static float FOC_clamp(float value, float minValue, float maxValue)
{
    if(value < minValue)
    {
        return minValue;
    }

    if(value > maxValue)
    {
        return maxValue;
    }

    return value;
}

static bool FOC_init(FOC_Handle *handle);
static float FOC_normalizeAngle(float angle);

//FOC主函数
void FOC_Task_Func(void * pvParameters){
    FOC_Handle *handle = (FOC_Handle *)pvParameters;

    if(handle == NULL)
    {
        handle = &s_focHandle;
    }

    FOC_HandleInit(handle);

    if(!FOC_init(handle))
    {
        /* 初始化失败时进入安全等待，避免继续执行未配置好的控制逻辑。 */
        for(;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}

/**
 * @brief 初始化 FOC 句柄，填充默认配置并清除运行态缓存。
 */
void FOC_HandleInit(FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return;
    }

    if((handle->config.defaultFrequency == 0U) ||
       (handle->config.voltagePowerSupply <= 0.0f) ||
       (handle->config.defaultDuty < 0.0f) ||
       (handle->config.defaultDuty > 1.0f))
    {
        handle->config = FOC_GetDefaultConfig();
    }
    handle->phaseVoltage.Ua   = 0.0f;
    handle->phaseVoltage.Ub   = 0.0f;
    handle->phaseVoltage.Uc   = 0.0f;
    handle->phaseCurrent.Ia   = 0.0f;
    handle->phaseCurrent.Ib   = 0.0f;
    handle->phaseCurrent.Ic   = 0.0f;
    handle->voltageAlphaBeta.alpha = 0.0f;
    handle->voltageAlphaBeta.beta  = 0.0f;
    handle->currentAlphaBeta.alpha = 0.0f;
    handle->currentAlphaBeta.beta  = 0.0f;
    handle->voltageDQ.d       = 0.0f;
    handle->voltageDQ.q       = 0.0f;
    handle->currentDQ.d       = 0.0f;
    handle->currentDQ.q       = 0.0f;
}

static bool FOC_init(FOC_Handle *handle)
{
    uint16_t index;
    bool ret;

    if(handle == NULL)
    {
        return false;
    }

    /*
     * 初始化前确保配置项合法，若非法则回退为默认值，保证系统稳健运行。
     */
    if((handle->config.defaultFrequency == 0U) ||
       (handle->config.voltagePowerSupply <= 0.0f) ||
       (handle->config.defaultDuty < 0.0f) ||
       (handle->config.defaultDuty > 1.0f))
    {
        handle->config = FOC_GetDefaultConfig();
    }

    ret = DRV_EPWM_setDeadbandCounts(handle->config.defaultDeadband,
                                     handle->config.defaultDeadband);
    if(!ret)
    {
        return false;
    }

    ret = DRV_EPWM_setFrequency(handle->config.defaultFrequency); //设置默认频率
    if(!ret)
    {
        return false;
    }

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        ret = DRV_EPWM_setDutyCycle(index, handle->config.defaultDuty); //设置默认占空比
        if(!ret)
        {
            return false;
        }
    }

    return true;
}


// 归一化角度到 [0,2PI]
static float FOC_normalizeAngle(float angle)
{
    float a = fmodf(angle, 2.0f * PI);   //取余运算可以用于归一化，列出特殊值例子算便知
    return (a >= 0.0f) ? a : (a + 2.0f * PI);
}

// 设置PWM到控制器输出
//输入电压值
/**
 * @brief 根据句柄中的供电参数设置三相电压并输出 PWM 占空比。
 */
void FOC_SetPhaseVoltage(FOC_Handle *handle, const FOC_PhaseVoltage *voltage)
{
    float dutyA;
    float dutyB;
    float dutyC;

    if((handle == NULL) || (voltage == NULL))
    {
        return;
    }

    handle->phaseVoltage = *voltage;

    if(handle->config.voltagePowerSupply <= 0.0f)
    {
        return;
    }

    /*
     * 计算占空比并限制在 [0, 1] 范围内，防止非法值损坏驱动器件。
     */
    dutyA = FOC_clamp(voltage->Ua / handle->config.voltagePowerSupply, 0.0f, 1.0f);
    dutyB = FOC_clamp(voltage->Ub / handle->config.voltagePowerSupply, 0.0f, 1.0f);
    dutyC = FOC_clamp(voltage->Uc / handle->config.voltagePowerSupply, 0.0f, 1.0f);

    (void)DRV_EPWM_setDutyCycle(0U, dutyA);
    (void)DRV_EPWM_setDutyCycle(1U, dutyB);
    (void)DRV_EPWM_setDutyCycle(2U, dutyC);
}


/**
 * @brief 基于句柄保存的上下文执行 Clarke 变换。
 */
void FOC_ClarkeTransform(FOC_Handle *handle,
                         const FOC_ThreePhaseCurrent *current,
                         FOC_AlphaBeta *output)
{
    if((handle == NULL) || (current == NULL))
    {
        return;
    }

    /*
     * 在三相对称系统中，Clarke 变换使用两相电流即可描述矢量。
     */
    handle->phaseCurrent = *current;
    handle->currentAlphaBeta.alpha = current->Ia;
    handle->currentAlphaBeta.beta  = (current->Ia + 2.0f * current->Ib) * INV_SQRT3_F;

    if(output != NULL)
    {
        *output = handle->currentAlphaBeta;
    }
}


/**
 * @brief 基于句柄保存的 αβ 电流执行 Park 变换，结果写回句柄。
 */
void FOC_ParkTransform(FOC_Handle *handle,
                       const FOC_AlphaBeta *input,
                       float angle_el,
                       FOC_DQ *output)
{
    float sin_angle;
    float cos_angle;
    const FOC_AlphaBeta *source;

    if(handle == NULL)
    {
        return;
    }

    source = (input != NULL) ? input : &handle->currentAlphaBeta;

    if(input != NULL)
    {
        handle->currentAlphaBeta = *input;
    }

    sin_angle = sinf(angle_el);
    cos_angle = cosf(angle_el);

    handle->currentDQ.d = source->alpha * cos_angle + source->beta * sin_angle;
    handle->currentDQ.q = -source->alpha * sin_angle + source->beta * cos_angle;

    if(output != NULL)
    {
        *output = handle->currentDQ;
    }
}


/**
 * @brief 执行逆 Clarke 变换，并将结果回写至句柄缓存。
 */
void FOC_InverseClarkeTransform(FOC_Handle *handle,
                                const FOC_AlphaBeta *input,
                                FOC_ThreePhaseCurrent *output)
{
    float ib_temp;
    float ic_temp;
    const FOC_AlphaBeta *source;

    if(handle == NULL)
    {
        return;
    }

    source = (input != NULL) ? input : &handle->voltageAlphaBeta;

    if(input != NULL)
    {
        handle->voltageAlphaBeta = *input;
    }

    ib_temp = (-source->alpha + SQRT3_F * source->beta) * 0.5f;
    ic_temp = (-source->alpha - SQRT3_F * source->beta) * 0.5f;

    handle->phaseCurrent.Ia = source->alpha;
    handle->phaseCurrent.Ib = ib_temp;
    handle->phaseCurrent.Ic = ic_temp;

    if(output != NULL)
    {
        *output = handle->phaseCurrent;
    }
}


/**
 * @brief 执行逆 Park 变换，将 dq 量转换为 αβ 量。
 */
void FOC_InverseParkTransform(FOC_Handle *handle,
                              const FOC_DQ *input,
                              float angle_el,
                              FOC_AlphaBeta *output)
{
    float sin_angle;
    float cos_angle;
    const FOC_DQ *source;

    if(handle == NULL)
    {
        return;
    }

    source = (input != NULL) ? input : &handle->voltageDQ;

    if(input != NULL)
    {
        handle->voltageDQ = *input;
    }

    sin_angle = sinf(angle_el);
    cos_angle = cosf(angle_el);

    handle->voltageAlphaBeta.alpha = source->d * cos_angle - source->q * sin_angle;
    handle->voltageAlphaBeta.beta  = source->d * sin_angle + source->q * cos_angle;

    if(output != NULL)
    {
        *output = handle->voltageAlphaBeta;
    }
}


/**
 * @brief 综合句柄配置执行 dq->αβ 变换，并缓存 αβ 电压指令。
 */
void FOC_SetAlphaBetaVoltage(FOC_Handle *handle, const FOC_DQ *voltageDQ, float angle_el)
{
    float normalizedAngle;

    if((handle == NULL) || (voltageDQ == NULL))
    {
        return;
    }

    handle->voltageDQ = *voltageDQ;

    normalizedAngle = FOC_normalizeAngle(angle_el + handle->config.zeroElectricAngle);

    /*
     * 采用标准逆帕克变换，同时缓存结果用于后续阶段的矢量调制或监控。
     */
    {
        float sinAngle = sinf(normalizedAngle);
        float cosAngle = cosf(normalizedAngle);

        handle->voltageAlphaBeta.alpha =
            voltageDQ->d * cosAngle - voltageDQ->q * sinAngle;
        handle->voltageAlphaBeta.beta  =
            voltageDQ->d * sinAngle + voltageDQ->q * cosAngle;
    }
}


/**
 * @brief 设置 FOC 配置参数。
 *
 * @param[in,out] handle FOC 句柄指针，不能为空。
 * @param[in]     config 待应用的配置指针。
 *
 * @retval true  配置生效。
 * @retval false 参数非法，未更新配置。
 */
bool FOC_Configure(FOC_Handle *handle, const FOC_Config *config)
{
    if((handle == NULL) || (config == NULL))
    {
        return false;
    }

    if((config->defaultFrequency == 0U) ||
       (config->voltagePowerSupply <= 0.0f) ||
       (config->defaultDuty < 0.0f) ||
       (config->defaultDuty > 1.0f))
    {
        return false;
    }

    handle->config = *config;
    return true;
}

/**
 * @brief 获取默认配置，用于初始化或恢复安全配置。
 */
FOC_Config FOC_GetDefaultConfig(void)
{
    FOC_Config defaultConfig =
    {
        .defaultDuty        = 0.0f,
        .defaultDeadband    = 0U,
        .defaultFrequency   = 10000U,
        .voltagePowerSupply = 12.0f,
        .zeroElectricAngle  = 0.0f
    };

    return defaultConfig;
}

/**
 * @brief 更新电角度零点校准值。
 *
 * @param[in,out] handle     FOC 句柄指针，不能为空。
 * @param[in] zeroAngle 校准值，单位 rad。
 *
 * @retval true  设置成功。
 * @retval false 参数非法。
 */
bool FOC_SetZeroElectricAngle(FOC_Handle *handle, float zeroAngle)
{
    if((handle == NULL) || !isfinite(zeroAngle))
    {
        return false;
    }

    handle->config.zeroElectricAngle = zeroAngle;
    return true;
}

/**
 * @brief 获取最近一次逆帕克变换后的 αβ 电压指令。
 */
const FOC_AlphaBeta *FOC_GetAlphaBetaVoltage(const FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return NULL;
    }

    return &handle->voltageAlphaBeta;
}

/**
 * @brief 获取最近一次 Clarke 变换后的 αβ 电流矢量。
 */
const FOC_AlphaBeta *FOC_GetAlphaBetaCurrent(const FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return NULL;
    }

    return &handle->currentAlphaBeta;
}

/**
 * @brief 获取最近一次设置的 dq 电压矢量。
 */
const FOC_DQ *FOC_GetDQVoltage(const FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return NULL;
    }

    return &handle->voltageDQ;
}

/**
 * @brief 获取最近一次 Park 变换得到的 dq 电流矢量。
 */
const FOC_DQ *FOC_GetDQCurrent(const FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return NULL;
    }

    return &handle->currentDQ;
}




