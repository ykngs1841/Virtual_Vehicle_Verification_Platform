#ifndef VVVP_GPIO_DIAGNOSTICS_H
#define VVVP_GPIO_DIAGNOSTICS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* User verified on hardware: SW1 D2/PA10 Active Low, LED1 D13/PA5 Active High. */
enum { VVVP_GPIO_DIAG_NOT_READY = 0, VVVP_GPIO_DIAG_READY = 1,
       VVVP_GPIO_DIAG_CLOCK_UNSUPPORTED = 2, VVVP_GPIO_DIAG_SYSTICK_BUSY = 3 };
enum { VVVP_GPIO_LED_LOW = 0, VVVP_GPIO_LED_HIGH = 1, VVVP_GPIO_LED_HIGH_Z = 2 };
enum { VVVP_GPIO_LED_ENGINE = 0, VVVP_GPIO_LED_DIAGNOSTIC = 1 };
extern volatile uint32_t vvvp_gpio_diag_status;
extern volatile int32_t vvvp_gpio_sw1_raw;
extern volatile uint32_t vvvp_gpio_led_drive;
extern volatile uint32_t vvvp_gpio_led_owner;
extern volatile uint32_t vvvp_gpio_diag_ms;
extern volatile uint32_t vvvp_gpio_diag_hclk_hz;

void vvvp_gpio_diag_init(void);
void vvvp_gpio_diag_poll(void);
int32_t vvvp_gpio_diag_read_sw1(void);
uint32_t vvvp_gpio_diag_test_led(uint32_t high);
void vvvp_gpio_diag_end_led_test(void);
void vvvp_gpio_led_set_engine_state(uint32_t engine_on);

#ifdef __cplusplus
}
#endif
#endif
