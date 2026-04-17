#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "project_config.h"

/* Backward-compatible aliases. Prefer including project_config.h directly for
 * new code, but keep these names so existing modules remain easy to diff.
 */
#define LED_GPIO_PORT   PROJECT_LED_GPIO_PORT
#define LED_GPIO_PIN    PROJECT_LED_GPIO_PIN

#ifdef __cplusplus
}
#endif

#endif /* APP_CONFIG_H */
