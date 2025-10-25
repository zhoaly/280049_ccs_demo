#include "drv_epwm.h"

#include "device.h"
#include "driverlib/gpio.h"
#include "driverlib/epwm.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"

#define DRV_EPWM_DEFAULT_FREQUENCY_HZ      (20000UL)
#define DRV_EPWM_DEFAULT_DUTY              (0.5f)
#define DRV_EPWM_DEFAULT_RED_COUNT         (50U)
#define DRV_EPWM_DEFAULT_FED_COUNT         (50U)
#define DRV_EPWM_TBCLK_DIVIDER             (1UL)
#define DRV_EPWM_TBCLK_HS_DIVIDER          (1UL)

static uint32_t s_frequencyHz = DRV_EPWM_DEFAULT_FREQUENCY_HZ;
static float s_dutyCycle[DRV_EPWM_CHANNEL_COUNT] =
{
    DRV_EPWM_DEFAULT_DUTY,
    DRV_EPWM_DEFAULT_DUTY,
    DRV_EPWM_DEFAULT_DUTY
};
static uint16_t s_risingEdgeDelayCount = DRV_EPWM_DEFAULT_RED_COUNT;
static uint16_t s_fallingEdgeDelayCount = DRV_EPWM_DEFAULT_FED_COUNT;
static bool s_initialized = false;

static const uint32_t s_epwmBase[DRV_EPWM_CHANNEL_COUNT] =
{
    EPWM1_BASE,
    EPWM2_BASE,
    EPWM3_BASE
};

static const uint32_t s_gpioPinConfigA[DRV_EPWM_CHANNEL_COUNT] =
{
    GPIO_0_EPWM1A,
    GPIO_2_EPWM2A,
    GPIO_4_EPWM3A
};

static const uint32_t s_gpioPinConfigB[DRV_EPWM_CHANNEL_COUNT] =
{
    GPIO_1_EPWM1B,
    GPIO_3_EPWM2B,
    GPIO_5_EPWM3B
};

static const uint32_t s_gpioPinA[DRV_EPWM_CHANNEL_COUNT] = { 0U, 2U, 4U };
static const uint32_t s_gpioPinB[DRV_EPWM_CHANNEL_COUNT] = { 1U, 3U, 5U };

static inline uint64_t DRV_EPWM_getTimeBaseClock(void)
{
    return ((uint64_t)DEVICE_SYSCLK_FREQ) /
           ((uint64_t)DRV_EPWM_TBCLK_DIVIDER * (uint64_t)DRV_EPWM_TBCLK_HS_DIVIDER);
}

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

static uint16_t DRV_EPWM_convertDutyToCompare(float dutyCycle, uint16_t period)
{
    return (uint16_t)((float)period * dutyCycle);
}

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

static uint16_t DRV_EPWM_clampDeadbandCount(uint16_t count)
{
    if(count >= 0x4000U)
    {
        return 0x3FFFU;
    }

    return count;
}

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

static void DRV_EPWM_enableModuleClocks(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM2);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EPWM3);
}

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
