/**
 * @file app_FOC.c
 * @brief 磁场定向控制（FOC）应用层实现。
 *
 * 该源文件包含 FOC 运行所需的初始化、矢量变换、占空比计算等核心逻辑。
 * 所有关键步骤均辅以详尽中文注释，帮助研发人员快速理解控制流程，
 * 同时为后续性能优化、故障排查提供参考。
 */

#include "app_FOC.h"
#include "app_FOC_OpenLoop.h"
#include "drv_epwm.h"


#include "c2000_freertos.h"
#include "device.h"
#include "board.h"
#include "math.h"
#include "driverlib/gpio.h"
#include "driverlib/epwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"

/**
 * @brief 默认 FOC 句柄实例。
 *
 * 在未显式传入句柄的场景下，任务入口会回退到该静态实例，确保模块
 * 始终拥有有效的运行环境。
 */
static FOC_Handle s_focHandle;

/**
 * @brief 默认 FOC 开环状态句柄实例。
 *
 */
static FOC_OpenLoopState s_focOpenLoopHandle;


/**
 * @brief 软限幅函数，确保输入值位于指定范围内。
 *
 * @param[in] value     原始值。
 * @param[in] minValue  最小允许值。
 * @param[in] maxValue  最大允许值。
 *
 * @return 钳制后的数值。
 */
float FOC_clamp(float value, float minValue, float maxValue)
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



/**
 * @brief FOC 主任务入口。
 *
 * @param[in] pvParameters 任务创建时传入的句柄指针，允许为 NULL。
 */
void FOC_Task_Func(void * pvParameters){

    FOC_Handle *handle_FOC = (FOC_Handle *)pvParameters;
    FOC_OpenLoopState *handle_OpenLoop_State = (FOC_OpenLoopState *)pvParameters;


    if(handle_FOC == NULL)//null的异常处理
    {
        handle_FOC = &s_focHandle;
    }

    if(handle_OpenLoop_State == NULL)//null的异常处理
    {
        handle_OpenLoop_State = &s_focOpenLoopHandle;
    }

    FOC_HandleInit(handle_FOC);
    FOC_OpenLoop_Init(handle_OpenLoop_State,6);//六极对

    if(!FOC_init(handle_FOC))
    {
        /* 初始化失败时进入安全等待，避免继续执行未配置好的控制逻辑。 */
        for(;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    while(1){

        vTaskDelay(pdMS_TO_TICKS(1));//1ms
        //暂时实现开环
        FOC_OpenLoop_RunVelocity(handle_OpenLoop_State,handle_FOC,20);
    }

}

/**
 * @brief 初始化 FOC 句柄，填充默认配置并清除运行态缓存。
 *
 * @param[in,out] handle 待初始化的 FOC 句柄指针，不能为空。
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
    handle->voltageAlphaBeta.alpha = 0.0f;
    handle->voltageAlphaBeta.beta  = 0.0f;
    handle->voltageDQ.d       = 0.0f;
    handle->voltageDQ.q       = 0.0f;
    handle->electricalAngle   = 0.0f;
}

/**
 * @brief 内部初始化例程，完成 PWM 配置等底层准备工作。
 *
 * @param[in,out] handle FOC 句柄指针，需提供有效配置并接收初始化后的状态。
 *
 * @retval true  初始化成功。
 * @retval false 参数非法或底层外设配置失败。
 */
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

    /*
     * 死区时间直接影响上下桥臂的安全裕量，在电机尚未运行前即完成配置，
     * 可避免后续切换过程出现直通风险。
     */
    ret = DRV_EPWM_setDeadbandCounts(handle->config.defaultDeadband,
                                     handle->config.defaultDeadband);
    if(!ret)
    {
        return false;
    }

    /* 配置 PWM 基频，决定了 SVPWM 或调制算法的采样率与控制带宽上限。 */
    ret = DRV_EPWM_setFrequency(handle->config.defaultFrequency); //设置默认频率
    if(!ret)
    {
        return false;
    }

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        /*
         * 在全部通道设置一致的初始占空比，可保证电机在启动前保持零矢量，
         * 防止意外转矩输出。
         */
        ret = DRV_EPWM_setDutyCycle(index, handle->config.defaultDuty); //设置默认占空比
        if(!ret)
        {
            return false;
        }
    }

    FOC_DriverEnable();//enable driver

    return true;
}

/**
 * @brief 三相位驱动的使能位,在初始化时使能
 */
static void FOC_DriverEnable()
{

    GPIO_writePin(FOC_DRV_EN1,1);//使能
    GPIO_writePin(FOC_DRV_EN2,1);
    GPIO_writePin(FOC_DRV_EN3,1);

}


/**
 * @brief 归一化角度值，保证角度输入始终位于标准区间。
 *
 * @param[in] angle 原始角度，单位 rad。
 *
 * @return 归一化后的角度，范围 [0, 2π)。
 */
float FOC_normalizeAngle(float angle)
{
    /*
     * fmodf 可保留浮点数的符号信息，通过与 2π 的取模实现周期化处理。
     * 对高频运行的角度积分器而言，可有效抑制数值逐渐增大造成的溢出风险。
     */
    float a = fmodf(angle, TWO_PI);
    return (a >= 0.0f) ? a : (a + TWO_PI);
}

/**
 * @brief 根据句柄中的供电参数设置三相电压并输出 PWM 占空比。
 *
 * @param[in,out] handle FOC 句柄指针，需提供电压指令与母线电压。
 */
void FOC_SetPhaseVoltage(FOC_Handle *handle)
{
    float dutyA;
    float dutyB;
    float dutyC;

    if(handle == NULL)
    {
        return;
    }

    if(handle->config.voltagePowerSupply <= 0.0f)
    {
        return;
    }

    /*
     * 计算占空比并限制在 [0, 1] 范围内，防止非法值损坏驱动器件。
     */
    dutyA = FOC_clamp(handle->phaseVoltage.Ua / handle->config.voltagePowerSupply, 0.0f, 1.0f);
    dutyB = FOC_clamp(handle->phaseVoltage.Ub / handle->config.voltagePowerSupply, 0.0f, 1.0f);
    dutyC = FOC_clamp(handle->phaseVoltage.Uc / handle->config.voltagePowerSupply, 0.0f, 1.0f);

    (void)DRV_EPWM_setDutyCycle(0U, dutyA);
    (void)DRV_EPWM_setDutyCycle(1U, dutyB);
    (void)DRV_EPWM_setDutyCycle(2U, dutyC);
}


/**
 * @brief 执行逆 Clarke 变换，并将结果回写至句柄缓存。
 *
 * @param[in,out] handle FOC 句柄指针，需提供 αβ 电压指令。
 */
void FOC_InverseClarkeTransform(FOC_Handle *handle)
{
    float vb_temp;
    float vc_temp;

    if(handle == NULL)
    {
        return;
    }

    /*
     * 基于 αβ 分量重构三相量时，需利用三相系统 Ia + Ib + Ic = 0 的约束。
     * 这里采用对称三相系统的标准公式，便于直接驱动三相逆变桥。
     */
    vb_temp = (-handle->voltageAlphaBeta.alpha + SQRT3 * handle->voltageAlphaBeta.beta) * 0.5f;
    vc_temp = (-handle->voltageAlphaBeta.alpha - SQRT3 * handle->voltageAlphaBeta.beta) * 0.5f;

    handle->phaseVoltage.Ua = handle->voltageAlphaBeta.alpha;
    handle->phaseVoltage.Ub = vb_temp;
    handle->phaseVoltage.Uc = vc_temp;
}


/**
 * @brief 执行逆 Park 变换，将 dq 量转换为 αβ 量。
 *
 * @param[in,out] handle FOC 句柄指针，需提供 dq 电压指令与电角度。
 */
void FOC_InverseParkTransform(FOC_Handle *handle)
{
    float sin_angle;
    float cos_angle;
    float transformAngle;

    if(handle == NULL)
    {
        return;
    }

    transformAngle = FOC_normalizeAngle(handle->electricalAngle + handle->config.zeroElectricAngle);
    sin_angle = sinf(transformAngle);
    cos_angle = cosf(transformAngle);

    /*
     * 逆帕克变换将旋转坐标系下的指令量映射回定子坐标系，
     * 是后续进行空-间矢量脉宽调制（SVPWM）的前置步骤。
     */
    handle->voltageAlphaBeta.alpha = handle->voltageDQ.d * cos_angle - handle->voltageDQ.q * sin_angle;
    handle->voltageAlphaBeta.beta  = handle->voltageDQ.d * sin_angle + handle->voltageDQ.q * cos_angle;
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

    /*
     * 结构体整体赋值可保持字段一致性，避免遗漏，便于后续扩展其他参数。
     */
    handle->config = *config;
    return true;
}

/**
 * @brief 获取默认配置，用于初始化或恢复安全配置。
 *
 * @return 默认配置结构体，供调用方直接使用或在此基础上修改。
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
 * @param[in,out] handle    FOC 句柄指针，不能为空。
 * @param[in]     zeroAngle 校准值，单位 rad。
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

const FOC_AlphaBeta *FOC_GetAlphaBetaVoltage(const FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return NULL;
    }

    return &handle->voltageAlphaBeta;
}

const FOC_DQ *FOC_GetDQVoltage(const FOC_Handle *handle)
{
    if(handle == NULL)
    {
        return NULL;
    }

    return &handle->voltageDQ;
}




