#ifndef VVVP_GPIO_DIAG_PLATFORM_H
#define VVVP_GPIO_DIAG_PLATFORM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t vvvp_gpio_platform_init(void);
uint32_t vvvp_gpio_platform_now_ms(void);
uint32_t vvvp_gpio_platform_read_sw1(void);
void vvvp_gpio_platform_drive_led(uint32_t high);
#ifdef __cplusplus
}
#endif
#endif
