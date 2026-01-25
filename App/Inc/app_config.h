#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* =========================
 *  System enable/disable
 * ========================= */
#define SYSTEM_CAN_ENABLED        (1)
#define SYSTEM_PCB_LED_ENABLED    (1)

/*
 * IMPORTANT:
 * Your build still compiles blink_system.c even if you disable it here.
 * So we keep BLINK_PERIOD_MS defined regardless.
 */
#define SYSTEM_BLINK_ENABLED      (0)

/* =========================
 *  Blink configuration
 * ========================= */
#define BLINK_PERIOD_MS           (5000U)

/* =========================
 *  LED configuration
 * ========================= */
/* Onboard LED is PC5 */
#define LED_GPIO_PORT             GPIOC
#define LED_GPIO_PIN              GPIO_PIN_5

/* =========================
 *  CAN test configuration
 * ========================= */
#define CAN_TX_PERIOD_MS          (100U)
#define CAN_TEST_TX_STD_ID        (0x123U)

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
