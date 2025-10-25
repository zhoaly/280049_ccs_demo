#ifndef DRV_EPWM_H
#define DRV_EPWM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_EPWM_CHANNEL_COUNT    (3U)

typedef struct
{
    uint32_t frequencyHz;
    float dutyCycle[DRV_EPWM_CHANNEL_COUNT];
    uint16_t risingEdgeDelayCount;
    uint16_t fallingEdgeDelayCount;
} DRV_EPWM_State;

void DRV_EPWM_init(void);

bool DRV_EPWM_setFrequency(uint32_t frequencyHz);

bool DRV_EPWM_setDutyCycle(uint32_t channelIndex, float dutyCycle);

void DRV_EPWM_setDeadbandCounts(uint16_t risingEdgeCount, uint16_t fallingEdgeCount);

void DRV_EPWM_getState(DRV_EPWM_State *state);

#ifdef __cplusplus
}
#endif

#endif /* DRV_EPWM_H */
