/**
 * @file drv_eqep.c
 * @brief eQEP 驱动实现文件，完成 MT6701 等磁编码器的底层配置与状态查询。
 */

#include "drv_eqep.h"

#include "device.h"
#include "driverlib/gpio.h"
#include "driverlib/eqep.h"

#define DRV_EQEP_TWO_PI_F                     (6.28318530717958647692f)
#define DRV_EQEP_DEFAULT_MAX_POSITION_COUNT   (16384UL)
#define DRV_EQEP_DEFAULT_POLE_PAIRS           (0UL)
#define DRV_EQEP_DEFAULT_UNIT_TIMER_US        (1000UL)

static DRV_EQEP_Config s_config =
{
    DRV_EQEP_DEFAULT_MAX_POSITION_COUNT,
    DRV_EQEP_DEFAULT_POLE_PAIRS,
    DRV_EQEP_DEFAULT_UNIT_TIMER_US
};

static bool s_initialized = false;
static float s_countsToMechanicalRad = 0.0f;
static float s_countsToMechanicalDeg = 0.0f;
static float s_countsToElectricalRad = 0.0f;
static uint32_t s_unitTimerPeriodCycles = 0U;

static void DRV_EQEP_updateDerivedConstants(void);
static void DRV_EQEP_configureGPIO(void);
static void DRV_EQEP_configurePeripheral(void);
static uint32_t DRV_EQEP_calculateUnitTimerCycles(uint32_t periodUs);
static uint32_t DRV_EQEP_getWrappedCount(uint32_t rawCount);
static float DRV_EQEP_normalizeAngle(float angleRad);

void DRV_EQEP_init(const DRV_EQEP_Config *config)
{
    DRV_EQEP_Config sanitized = s_config;

    if(config != NULL)
    {
        if(config->maxPositionCount != 0U)
        {
            sanitized.maxPositionCount = config->maxPositionCount;
        }
        if(config->unitTimerPeriodMicroseconds != 0U)
        {
            sanitized.unitTimerPeriodMicroseconds =
                config->unitTimerPeriodMicroseconds;
        }
        sanitized.polePairs = config->polePairs;
    }

    if(sanitized.maxPositionCount == 0U)
    {
        sanitized.maxPositionCount = DRV_EQEP_DEFAULT_MAX_POSITION_COUNT;
    }

    if(sanitized.unitTimerPeriodMicroseconds == 0U)
    {
        sanitized.unitTimerPeriodMicroseconds = DRV_EQEP_DEFAULT_UNIT_TIMER_US;
    }

    s_config = sanitized;
    s_unitTimerPeriodCycles =
        DRV_EQEP_calculateUnitTimerCycles(s_config.unitTimerPeriodMicroseconds);
    DRV_EQEP_updateDerivedConstants();

    DRV_EQEP_configureGPIO();
    DRV_EQEP_configurePeripheral();

    s_initialized = true;
}

void DRV_EQEP_enable(void)
{
    if(!s_initialized)
    {
        return;
    }

    EQEP_enableModule(EQEP1_BASE);
}

void DRV_EQEP_disable(void)
{
    if(!s_initialized)
    {
        return;
    }

    EQEP_disableModule(EQEP1_BASE);
}

uint32_t DRV_EQEP_getPositionCount(void)
{
    return EQEP_getPosition(EQEP1_BASE);
}

void DRV_EQEP_setPositionCount(uint32_t position)
{
    EQEP_setPosition(EQEP1_BASE, position);
}

void DRV_EQEP_getState(DRV_EQEP_State *state)
{
    uint32_t rawCount;
    uint32_t wrappedCount;

    if(state == NULL)
    {
        return;
    }

    rawCount = EQEP_getPosition(EQEP1_BASE);
    wrappedCount = DRV_EQEP_getWrappedCount(rawCount);

    state->rawPositionCount = rawCount;
    state->direction = EQEP_getDirection(EQEP1_BASE);
    state->mechanicalAngleRad = (float)wrappedCount * s_countsToMechanicalRad;
    state->mechanicalAngleDeg = (float)wrappedCount * s_countsToMechanicalDeg;

    if((s_config.polePairs == 0U) || (s_countsToElectricalRad == 0.0f))
    {
        state->electricalAngleRad = 0.0f;
    }
    else
    {
        float electricalAngle = (float)wrappedCount * s_countsToElectricalRad;
        state->electricalAngleRad = DRV_EQEP_normalizeAngle(electricalAngle);
    }
}

static void DRV_EQEP_updateDerivedConstants(void)
{
    const float maxCount = (float)s_config.maxPositionCount;

    if(maxCount <= 0.0f)
    {
        s_countsToMechanicalRad = 0.0f;
        s_countsToMechanicalDeg = 0.0f;
        s_countsToElectricalRad = 0.0f;
        return;
    }

    s_countsToMechanicalRad = DRV_EQEP_TWO_PI_F / maxCount;
    s_countsToMechanicalDeg = 360.0f / maxCount;

    if(s_config.polePairs == 0U)
    {
        s_countsToElectricalRad = 0.0f;
    }
    else
    {
        s_countsToElectricalRad = s_countsToMechanicalRad *
                                  (float)s_config.polePairs;
    }
}

static void DRV_EQEP_configureGPIO(void)
{
    GPIO_setPinConfig(DEVICE_GPIO_CFG_EQEP1A);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_EQEP1A, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_EQEP1A, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_EQEP1A, GPIO_QUAL_SYNC);

    GPIO_setPinConfig(DEVICE_GPIO_CFG_EQEP1B);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_EQEP1B, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_EQEP1B, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_EQEP1B, GPIO_QUAL_SYNC);

#ifdef DEVICE_GPIO_CFG_EQEP1I
    GPIO_setPinConfig(DEVICE_GPIO_CFG_EQEP1I);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_EQEP1I, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_EQEP1I, GPIO_DIR_MODE_IN);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_EQEP1I, GPIO_QUAL_SYNC);
#endif
}

static void DRV_EQEP_configurePeripheral(void)
{
    uint32_t maxPositionRegister;

    EQEP_disableModule(EQEP1_BASE);

    maxPositionRegister = (s_config.maxPositionCount == 0U) ?
                          (DRV_EQEP_DEFAULT_MAX_POSITION_COUNT - 1U) :
                          (s_config.maxPositionCount - 1U);

    EQEP_setEmulationMode(EQEP1_BASE, EQEP_EMULATIONMODE_RUNFREE);
    EQEP_setDecoderConfig(EQEP1_BASE,
                          EQEP_CONFIG_QUADRATURE |
                          EQEP_CONFIG_2X_RESOLUTION |
                          EQEP_CONFIG_NO_SWAP |
                          EQEP_CONFIG_IGATE_DISABLE);
    EQEP_setPositionCounterConfig(EQEP1_BASE,
                                  EQEP_POSITION_RESET_MAX_POS,
                                  maxPositionRegister);
    EQEP_setInitialPosition(EQEP1_BASE, 0U);
    EQEP_setPosition(EQEP1_BASE, 0U);
    EQEP_setLatchMode(EQEP1_BASE,
                      EQEP_LATCH_UNIT_TIME_OUT |
                      EQEP_LATCH_RISING_STROBE |
                      EQEP_LATCH_RISING_INDEX);

    EQEP_setCaptureConfig(EQEP1_BASE,
                          EQEP_CAPTURE_CLK_DIV_32,
                          EQEP_UNIT_POS_EVNT_DIV_32);
    EQEP_enableCapture(EQEP1_BASE);

    EQEP_enableUnitTimer(EQEP1_BASE, s_unitTimerPeriodCycles);

    EQEP_enableModule(EQEP1_BASE);
}

static uint32_t DRV_EQEP_calculateUnitTimerCycles(uint32_t periodUs)
{
    const uint64_t ticksPerUs =
        (uint64_t)DEVICE_SYSCLK_FREQ / (uint64_t)1000000U;
    uint64_t cycles;

    if(periodUs == 0U)
    {
        periodUs = DRV_EQEP_DEFAULT_UNIT_TIMER_US;
    }

    cycles = ticksPerUs * (uint64_t)periodUs;

    if(cycles == 0ULL)
    {
        cycles = 1ULL;
    }

    if(cycles > (uint64_t)UINT32_MAX)
    {
        cycles = (uint64_t)UINT32_MAX;
    }

    return (uint32_t)cycles;
}

static uint32_t DRV_EQEP_getWrappedCount(uint32_t rawCount)
{
    if(s_config.maxPositionCount == 0U)
    {
        return rawCount;
    }

    return rawCount % s_config.maxPositionCount;
}

static float DRV_EQEP_normalizeAngle(float angleRad)
{
    float normalized = angleRad;

    while(normalized >= DRV_EQEP_TWO_PI_F)
    {
        normalized -= DRV_EQEP_TWO_PI_F;
    }

    while(normalized < 0.0f)
    {
        normalized += DRV_EQEP_TWO_PI_F;
    }

    return normalized;
}
