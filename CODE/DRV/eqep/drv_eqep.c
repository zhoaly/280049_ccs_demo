/**
 * @file drv_eqep.c
 * @brief eQEP 驱动实现，面向磁编码器的角度与速度采集。
 */

#include "drv_eqep.h"

#include "device.h"
#include "driverlib/eqep.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"

#define DRV_EQEP_DEFAULT_COUNTS_PER_REV   (16384UL)  /**< 默认磁编码器分辨率（14bit）。 */
#define DRV_EQEP_DEFAULT_POLE_PAIRS       (1UL)      /**< 默认极对数。 指的是电机上用于检测位置的磁铁的极对数*/ 
#define DRV_EQEP_GPIO_QEPA                (6U)
#define DRV_EQEP_GPIO_QEPB                (7U)
#define DRV_EQEP_GPIO_INDEX               (9U)
#define DRV_EQEP_GPIO_STROBE              (8U)
#define DRV_EQEP_TWO_PI                   (6.28318530717958647692f)

typedef struct
{
    uint32_t base;
    uint32_t lastPosition;
    bool     initialized;
} DRV_EQEP_InternalState;

static DRV_EQEP_InternalState s_internalState =
{
    .base        = EQEP1_BASE,
    .lastPosition = 0U,
    .initialized  = false
};

static DRV_EQEP_State s_state =
{
    .countsPerRevolution = DRV_EQEP_DEFAULT_COUNTS_PER_REV,
    .polePairs           = DRV_EQEP_DEFAULT_POLE_PAIRS,
    .rawPosition         = 0U,
    .deltaCounts         = 0,
    .mechanicalAngleRad  = 0.0f,
    .electricalAngleRad  = 0.0f,
    .mechanicalSpeedRps  = 0.0f
};

static void DRV_EQEP_enableModuleClock(void)
{
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_EQEP1);
    SysCtl_resetPeripheral(SYSCTL_PERIPH_RES_EQEP1);
}

static void DRV_EQEP_configureGPIO(void)
{
    GPIO_setPinConfig(GPIO_6_EQEP1_A);
    GPIO_setPinConfig(GPIO_7_EQEP1_B);
    GPIO_setPinConfig(GPIO_8_EQEP1_STROBE);
    GPIO_setPinConfig(GPIO_9_EQEP1_INDEX);

    GPIO_setDirectionMode(DRV_EQEP_GPIO_QEPA, GPIO_DIR_MODE_IN);
    GPIO_setDirectionMode(DRV_EQEP_GPIO_QEPB, GPIO_DIR_MODE_IN);
    GPIO_setDirectionMode(DRV_EQEP_GPIO_INDEX, GPIO_DIR_MODE_IN);
    GPIO_setDirectionMode(DRV_EQEP_GPIO_STROBE, GPIO_DIR_MODE_IN);

    GPIO_setQualificationMode(DRV_EQEP_GPIO_QEPA, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(DRV_EQEP_GPIO_QEPB, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(DRV_EQEP_GPIO_INDEX, GPIO_QUAL_SYNC);
    GPIO_setQualificationMode(DRV_EQEP_GPIO_STROBE, GPIO_QUAL_SYNC);
}

static void DRV_EQEP_applyCountsPerRevolution(uint32_t counts)
{
    uint32_t base = s_internalState.base;
    uint32_t position = EQEP_getPosition(base);
    uint32_t maxPosition = (counts == 0U) ? 0U : (counts - 1U);

    if(counts == 0U)
    {
        return;
    }

    EQEP_setPositionCounterConfig(base, EQEP_POSITION_RESET_MAX_POS, maxPosition);

    if(position > maxPosition)
    {
        position %= counts;
    }

    EQEP_setPosition(base, position);
    s_internalState.lastPosition = position;
    s_state.rawPosition = position;
}

static float DRV_EQEP_normalizeAngle(float angle)
{
    while(angle >= DRV_EQEP_TWO_PI)
    {
        angle -= DRV_EQEP_TWO_PI;
    }

    while(angle < 0.0f)
    {
        angle += DRV_EQEP_TWO_PI;
    }

    return angle;
}

static void DRV_EQEP_configureModule(void)
{
    uint32_t base = s_internalState.base;

    EQEP_disableModule(base);
    EQEP_setDecoderConfig(base,
                          EQEP_CONFIG_QUADRATURE |
                          EQEP_CONFIG_2X_RESOLUTION |
                          EQEP_CONFIG_NO_SWAP |
                          EQEP_CONFIG_IGATE_DISABLE);
    EQEP_setEmulationMode(base, EQEP_EMULATIONMODE_RUNFREE);
    EQEP_setPosition(base, 0U);
    s_internalState.lastPosition = 0U;
    s_state.rawPosition = 0U;
    s_state.deltaCounts = 0;
    s_state.mechanicalAngleRad = 0.0f;
    s_state.electricalAngleRad = 0.0f;
    s_state.mechanicalSpeedRps = 0.0f;
    DRV_EQEP_applyCountsPerRevolution(s_state.countsPerRevolution);
    EQEP_enableModule(base);
}

void DRV_EQEP_init(void)
{
    if(s_internalState.initialized)
    {
        return;
    }

    DRV_EQEP_enableModuleClock();
    DRV_EQEP_configureGPIO();
    DRV_EQEP_configureModule();

    s_internalState.initialized = true;
}

bool DRV_EQEP_setCountsPerRevolution(uint32_t counts)
{
    if(counts == 0U)
    {
        return false;
    }

    s_state.countsPerRevolution = counts;

    if(s_internalState.initialized)
    {
        DRV_EQEP_applyCountsPerRevolution(counts);
    }

    return true;
}

bool DRV_EQEP_setPolePairs(uint32_t polePairs)
{
    if(polePairs == 0U)
    {
        return false;
    }

    s_state.polePairs = polePairs;
    return true;
}

void DRV_EQEP_resetPosition(void)
{
    s_internalState.lastPosition = 0U;
    s_state.rawPosition = 0U;
    s_state.deltaCounts = 0;
    s_state.mechanicalAngleRad = 0.0f;
    s_state.electricalAngleRad = 0.0f;

    if(s_internalState.initialized)
    {
        EQEP_setPosition(s_internalState.base, 0U);
    }
}

void DRV_EQEP_update(float samplePeriodSeconds)
{
    uint32_t position;
    int32_t delta;
    int32_t halfCpr;
    float mechanicalAngle;
    float electricalAngle;

    if(!s_internalState.initialized)
    {
        return;
    }

    position = EQEP_getPosition(s_internalState.base);
    s_state.rawPosition = position;

    delta = (int32_t)position - (int32_t)s_internalState.lastPosition;
    halfCpr = (int32_t)(s_state.countsPerRevolution / 2U);

    if(delta > halfCpr)
    {
        delta -= (int32_t)s_state.countsPerRevolution;
    }
    else if(delta < -halfCpr)
    {
        delta += (int32_t)s_state.countsPerRevolution;
    }

    s_internalState.lastPosition = position;
    s_state.deltaCounts = delta;

    mechanicalAngle = ((float)position / (float)s_state.countsPerRevolution) * DRV_EQEP_TWO_PI;
    s_state.mechanicalAngleRad = DRV_EQEP_normalizeAngle(mechanicalAngle);

    electricalAngle = s_state.mechanicalAngleRad * (float)s_state.polePairs;
    s_state.electricalAngleRad = DRV_EQEP_normalizeAngle(electricalAngle);

    if(samplePeriodSeconds > 0.0f)
    {
        s_state.mechanicalSpeedRps = ((float)delta / (float)s_state.countsPerRevolution) /
                                     samplePeriodSeconds;
    }
    else
    {
        s_state.mechanicalSpeedRps = 0.0f;
    }
}

void DRV_EQEP_getState(DRV_EQEP_State *state)
{
    if(state == NULL)
    {
        return;
    }

    *state = s_state;
}

uint32_t DRV_EQEP_getRawPosition(void)
{
    return s_state.rawPosition;
}

float DRV_EQEP_getMechanicalAngleRad(void)
{
    return s_state.mechanicalAngleRad;
}
