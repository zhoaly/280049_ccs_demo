/* app_pintest.h */
#ifndef APP_PINTEST_H
#define APP_PINTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "app_proto.h"

/* Configuration */
#ifndef APP_PINTEST_CHANNEL
#define APP_PINTEST_CHANNEL APP_PROTO_CH1
#endif

#ifndef APP_PINTEST_PIN_ID
#define APP_PINTEST_PIN_ID 0u
#endif

#ifndef APP_PINTEST_GPIO_PIN
#define APP_PINTEST_GPIO_PIN 12u
#endif

#ifndef APP_PINTEST_GPIO_PIN_CONFIG
#define APP_PINTEST_GPIO_PIN_CONFIG GPIO_12_GPIO12
#endif

#ifndef APP_PINTEST_INPUT_PULLUP
#define APP_PINTEST_INPUT_PULLUP 0u
#endif

#ifndef APP_PINTEST_ACK_TIMEOUT_MS
#define APP_PINTEST_ACK_TIMEOUT_MS 100u
#endif

#ifndef APP_PINTEST_SETTLE_DELAY_MS
#define APP_PINTEST_SETTLE_DELAY_MS 2u
#endif

#ifndef APP_PINTEST_PERIOD_MS
#define APP_PINTEST_PERIOD_MS 1000u
#endif

#ifndef APP_PINTEST_RESET_AFTER
#define APP_PINTEST_RESET_AFTER 1u
#endif

/* Payload format: [CMD, PIN_ID, LEVEL] */
#define APP_PINTEST_CMD_SET_LEVEL  0x01u
#define APP_PINTEST_LEVEL_LOW      0u
#define APP_PINTEST_LEVEL_HIGH     1u
#define APP_PINTEST_PAYLOAD_LEN    3u

/* ACK semaphores signaled by PROTO master handler */
extern SemaphoreHandle_t PROTO_ACK_CH0Handle;
extern SemaphoreHandle_t PROTO_ACK_CH1Handle;

void APP_PINTEST_Init(void);
void APP_PINTEST_Task_Func(void *pvParameters);

/* Called by PROTO slave handler when a WRITE frame arrives. Returns 1 if handled. */
uint16_t APP_PINTEST_OnProtoWrite(uint16_t channel,
                                  const uint16_t *payload,
                                  uint16_t len);

/* Called by PROTO master handler when an ACK frame arrives. */
void APP_PINTEST_OnProtoAck(uint16_t channel);

#ifdef __cplusplus
}
#endif

#endif /* APP_PINTEST_H */
