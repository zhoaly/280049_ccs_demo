/**
 * @file drv_epwm.c
 * @brief ePWM 驱动实现文件，完成互补 PWM 初始化、参数配置与状态查询。
 */

#include "drv_epwm.h"

#include "device.h"
#include "driverlib/gpio.h"
#include "driverlib/epwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"

#define DRV_EPWM_DEFAULT_FREQUENCY_HZ      (20000UL) /**< 默认 PWM 开关频率，单位 Hz。 */
#define DRV_EPWM_DEFAULT_DUTY              (0.5f)    /**< 默认占空比（0.0~1.0）。 */
#define DRV_EPWM_DEFAULT_RED_COUNT         (50U)     /**< 默认上升沿死区计数值。 */
#define DRV_EPWM_DEFAULT_FED_COUNT         (50U)     /**< 默认下降沿死区计数值。 */
#define DRV_EPWM_TBCLK_DIVIDER             (1UL)     /**< ePWM TBCLK 分频系数。 */
#define DRV_EPWM_TBCLK_HS_DIVIDER          (1UL)     /**< ePWM TBCLK 高速分频系数。 */

static uint32_t s_frequencyHz = DRV_EPWM_DEFAULT_FREQUENCY_HZ; /**< 当前 PWM 频率配置。 */
static float s_dutyCycle[DRV_EPWM_CHANNEL_COUNT] =
{
    DRV_EPWM_DEFAULT_DUTY,
    DRV_EPWM_DEFAULT_DUTY,
    DRV_EPWM_DEFAULT_DUTY
}; /**< 各通道占空比缓存，用于延迟生效与状态查询。 */
static uint16_t s_risingEdgeDelayCount = DRV_EPWM_DEFAULT_RED_COUNT; /**< 死区上升沿计数。 */
static uint16_t s_fallingEdgeDelayCount = DRV_EPWM_DEFAULT_FED_COUNT; /**< 死区下降沿计数。 */
static bool s_initialized = false; /**< 驱动初始化标志。 */

static const uint32_t s_epwmBase[DRV_EPWM_CHANNEL_COUNT] =
{
    EPWM1_BASE,
    EPWM2_BASE,
    EPWM3_BASE
}; /**< ePWM 基地址表。 */

static const uint32_t s_gpioPinConfigA[DRV_EPWM_CHANNEL_COUNT] =
{
    GPIO_0_EPWM1A,
    GPIO_2_EPWM2A,
    GPIO_4_EPWM3A
}; /**< ePWM A 相引脚复用配置表。 */

static const uint32_t s_gpioPinConfigB[DRV_EPWM_CHANNEL_COUNT] =
{
    GPIO_1_EPWM1B,
    GPIO_3_EPWM2B,
    GPIO_5_EPWM3B
}; /**< ePWM B 相引脚复用配置表。 */

static const uint32_t s_gpioPinA[DRV_EPWM_CHANNEL_COUNT] = { 0U, 2U, 4U }; /**< ePWM A 相 GPIO 序号表。 */
static const uint32_t s_gpioPinB[DRV_EPWM_CHANNEL_COUNT] = { 1U, 3U, 5U }; /**< ePWM B 相 GPIO 序号表。 */

/**
 * @brief 计算当前系统时钟对应的 ePWM 时基时钟频率。
 *
 * @return 64 位 TBCLK 频率值。
 */
static inline uint64_t DRV_EPWM_getTimeBaseClock(void)
{
    return ((uint64_t)DEVICE_SYSCLK_FREQ) /
           ((uint64_t)DRV_EPWM_TBCLK_DIVIDER * (uint64_t)DRV_EPWM_TBCLK_HS_DIVIDER);
}

/**
 * @brief 根据目标频率计算时基周期值。
 *
 * @param[in]  frequencyHz 目标 PWM 频率，单位 Hz。
 * @param[out] period      计算得到的 TBPRD 寄存器值。
 *
 * @retval true  计算成功并输出有效周期。
 * @retval false 输入或运算参数非法。
 */
static bool DRV_EPWM_calculatePeriod(uint32_t frequencyHz, uint16_t *period)
{
    uint64_t tbclk;
    uint64_t denominator;
    uint64_t period64;

    if((frequencyHz == 0U) || (period == NULL))
    {
        return false;
    }

    tbclk = DRV_EPWM_getTimeBaseClock();
    denominator = (uint64_t)frequencyHz * 2ULL;

    if(denominator == 0ULL)
    {
        return false;
    }

    period64 = tbclk / denominator;

    if((period64 == 0ULL) || (period64 > (uint64_t)UINT16_MAX))
    {
        return false;
    }

    *period = (uint16_t)period64;
    return true;
}

/**
 * @brief 将占空比转换为计比较值。
 *
 * @param[in] dutyCycle 占空比（0.0~1.0）。
 * @param[in] period    时基周期值。
 *
 * @return 对应的 CMPA 计比较值。
 */
static uint16_t DRV_EPWM_convertDutyToCompare(float dutyCycle, uint16_t period)
{
    return (uint16_t)((float)period * dutyCycle);
}

/**
 * @brief 对占空比参数进行上下限钳制。
 *
 * @param[in] dutyCycle 输入占空比值。
 *
 * @return 0.0~1.0 范围内的占空比。
 */
static float DRV_EPWM_clampDuty(float dutyCycle)
{
    if(dutyCycle < 0.0f)
    {
        return 0.0f;
    }
    else if(dutyCycle > 1.0f)
    {
        return 1.0f;
    }

    return dutyCycle;
}

/**
 * @brief 钳制死区计数值，确保满足硬件寄存器范围。
 *
 * @param[in] count 输入计数值。
 *
 * @return 合法的死区计数值。
 */
static uint16_t DRV_EPWM_clampDeadbandCount(uint16_t count)
{
    if(count >= 0x4000U)
    {
        return 0x3FFFU;
    }

    return count;
}

/**
 * @brief 配置动作限定，生成互补 PWM 输出波形。
 *
 * @param[in] base ePWM 模块基地址。
 */
static void DRV_EPWM_configureActionQualifier(uint32_t base)
{
    EPWM_setActionQualifierAction(base,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_ZERO);
    EPWM_setActionQualifierAction(base,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_LOW,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_UP_CMPA);
    EPWM_setActionQualifierAction(base,
                                  EPWM_AQ_OUTPUT_A,
                                  EPWM_AQ_OUTPUT_HIGH,
                                  EPWM_AQ_OUTPUT_ON_TIMEBASE_DOWN_CMPA);
}

/**
 * @brief 配置 ePWM 死区模块，实现互补输出与死区延时。
 *
 * @param[in] base ePWM 模块基地址。
 */
static void DRV_EPWM_configureDeadBand(uint32_t base)
{
    EPWM_setDeadBandDelayMode(base, EPWM_DB_RED, true);
    EPWM_setDeadBandDelayMode(base, EPWM_DB_FED, true);
    EPWM_setRisingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setFallingEdgeDeadBandDelayInput(base, EPWM_DB_INPUT_EPWMA);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_RED, EPWM_DB_POLARITY_ACTIVE_HIGH);
    EPWM_setDeadBandDelayPolarity(base, EPWM_DB_FED, EPWM_DB_POLARITY_ACTIVE_LOW);
    EPWM_setRisingEdgeDelayCount(base, s_risingEdgeDelayCount);
    EPWM_setFallingEdgeDelayCount(base, s_fallingEdgeDelayCount);
}

/**
 * @brief 初始化时基模块，配置计数模式、周期与比较值。
 *
 * @param[in] base    ePWM 模块基地址。
 * @param[in] period  时基周期值。
 * @param[in] compare CMPA 初始比较值。
 */
static void DRV_EPWM_configureTimeBase(uint32_t base, uint16_t period, uint16_t compare)
{
    EPWM_setClockPrescaler(base, EPWM_CLOCK_DIVIDER_1, EPWM_HSCLOCK_DIVIDER_1);
    EPWM_disablePhaseShiftLoad(base);
    EPWM_setPhaseShift(base, 0U);
    EPWM_setTimeBaseCounter(base, 0U);
    EPWM_setTimeBaseCounterMode(base, EPWM_COUNTER_MODE_UP_DOWN);
    EPWM_setTimeBasePeriod(base, period);

    EPWM_setCounterCompareShadowLoadMode(base,
                                         EPWM_COUNTER_COMPARE_A,
                                         EPWM_COMP_LOAD_ON_CNTR_ZERO);
    EPWM_setCounterCompareValue(base, EPWM_COUNTER_COMPARE_A, compare);
}

/**
 * @brief 配置 ePWM 输出相关 GPIO 引脚为互补推挽输出模式。
 */
static void DRV_EPWM_configureGPIO(void)
{
    uint32_t index;

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        GPIO_setPinConfig(s_gpioPinConfigA[index]);
        GPIO_setDirectionMode(s_gpioPinA[index], GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(s_gpioPinA[index], GPIO_PIN_TYPE_STD);
        GPIO_setQualificationMode(s_gpioPinA[index], GPIO_QUAL_SYNC);

        GPIO_setPinConfig(s_gpioPinConfigB[index]);
        GPIO_setDirectionMode(s_gpioPinB[index], GPIO_DIR_MODE_OUT);
        GPIO_setPadConfig(s_gpioPinB[index], GPIO_PIN_TYPE_STD);
        GPIO_setQualificationMode(s_gpioPinB[index], GPIO_QUAL_SYNC);
    }
}

/**
 * @brief 使能 ePWM 模块所需的外设时钟。
 */
static void DRV_EPWM_enableModuleClocks(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM3);
}

/**
 * @brief 初始化 ePWM 驱动，配置三对互补 PWM 输出。
 *
 * 若在初始化前已调用频率、占空比或死区配置接口，则此函数会使用缓存参数进行初始化。
 */
void DRV_EPWM_init(void)
{
    uint16_t period;
    uint32_t index;

    if(s_initialized)
    {
        return;
    }

    DRV_EPWM_enableModuleClocks();
    DRV_EPWM_configureGPIO();

    if(!DRV_EPWM_calculatePeriod(s_frequencyHz, &period))
    {
        period = 1U;
    }

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        uint16_t compare = DRV_EPWM_convertDutyToCompare(s_dutyCycle[index], period);
        DRV_EPWM_configureTimeBase(s_epwmBase[index], period, compare);
        DRV_EPWM_configureActionQualifier(s_epwmBase[index]);
        DRV_EPWM_configureDeadBand(s_epwmBase[index]);
    }

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    s_initialized = true;
}

/**
 * @brief 设置所有通道的 PWM 开关频率。
 *
 * @param[in] frequencyHz 目标频率，单位 Hz。
 *
 * @retval true  配置成功。
 * @retval false 参数非法或超出硬件可支持范围。
 */
bool DRV_EPWM_setFrequency(uint32_t frequencyHz)
{
    uint16_t period;
    uint32_t index;

    if(!DRV_EPWM_calculatePeriod(frequencyHz, &period))
    {
        return false;
    }

    if(!s_initialized)
    {
        s_frequencyHz = frequencyHz;
        return true;
    }

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        uint16_t compare = DRV_EPWM_convertDutyToCompare(s_dutyCycle[index], period);
        EPWM_setTimeBasePeriod(s_epwmBase[index], period);
        EPWM_setCounterCompareValue(s_epwmBase[index], EPWM_COUNTER_COMPARE_A, compare);
        EPWM_setTimeBaseCounter(s_epwmBase[index], 0U);
    }

    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    s_frequencyHz = frequencyHz;
    return true;
}

/**
 * @brief 设置单通道 PWM 占空比。
 *
 * @param[in] channelIndex 通道索引（0~DRV_EPWM_CHANNEL_COUNT-1）。
 * @param[in] dutyCycle    目标占空比，范围 0.0~1.0。
 *
 * @retval true  配置成功或已缓存待初始化。
 * @retval false 参数非法。
 */
bool DRV_EPWM_setDutyCycle(uint32_t channelIndex, float dutyCycle)
{
    uint16_t period;
    uint16_t compare;

    if(channelIndex >= DRV_EPWM_CHANNEL_COUNT)
    {
        return false;
    }

    if(!DRV_EPWM_calculatePeriod(s_frequencyHz, &period))
    {
        return false;
    }

    dutyCycle = DRV_EPWM_clampDuty(dutyCycle);
    compare = DRV_EPWM_convertDutyToCompare(dutyCycle, period);

    s_dutyCycle[channelIndex] = dutyCycle;

    if(!s_initialized)
    {
        return true;
    }

    EPWM_setCounterCompareValue(s_epwmBase[channelIndex],
                                EPWM_COUNTER_COMPARE_A,
                                compare);

    return true;
}

/**
 * @brief 设置互补输出的死区计数。
 *
 * @param[in] risingEdgeCount  上升沿死区计数。
 * @param[in] fallingEdgeCount 下降沿死区计数。
 */
void DRV_EPWM_setDeadbandCounts(uint16_t risingEdgeCount, uint16_t fallingEdgeCount)
{
    uint32_t index;

    s_risingEdgeDelayCount = DRV_EPWM_clampDeadbandCount(risingEdgeCount);
    s_fallingEdgeDelayCount = DRV_EPWM_clampDeadbandCount(fallingEdgeCount);

    if(!s_initialized)
    {
        return;
    }

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        EPWM_setRisingEdgeDelayCount(s_epwmBase[index], s_risingEdgeDelayCount);
        EPWM_setFallingEdgeDelayCount(s_epwmBase[index], s_fallingEdgeDelayCount);
    }
}

/**
 * @brief 获取当前 ePWM 配置信息。
 *
 * @param[out] state 状态结构体指针。
 */
void DRV_EPWM_getState(DRV_EPWM_State *state)
{
    uint32_t index;

    if(state == NULL)
    {
        return;
    }

    state->frequencyHz = s_frequencyHz;
    state->risingEdgeDelayCount = s_risingEdgeDelayCount;
    state->fallingEdgeDelayCount = s_fallingEdgeDelayCount;

    for(index = 0U; index < DRV_EPWM_CHANNEL_COUNT; index++)
    {
        state->dutyCycle[index] = s_dutyCycle[index];
    }
}
