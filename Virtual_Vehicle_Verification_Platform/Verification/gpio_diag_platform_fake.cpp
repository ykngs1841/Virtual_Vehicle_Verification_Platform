// Host test double only; not part of the CubeIDE source entries.
#include "gpio_diag_platform_fake.h"
#include "gpio_diag_platform.h"
#include "gpio_diagnostics.h"
uint32_t test_gpio_status = VVVP_GPIO_DIAG_READY;
uint32_t test_gpio_raw = 1U;
uint32_t test_gpio_mode = VVVP_GPIO_LED_HIGH_Z;
uint32_t test_gpio_drive_count = 0U;
volatile uint32_t vvvp_gpio_diag_ms = 0U;
volatile uint32_t vvvp_gpio_diag_hclk_hz = 16000000U;
uint32_t vvvp_gpio_platform_init(void)
{
    vvvp_gpio_platform_drive_led(0U); // Production initialization preloads OFF.
    return test_gpio_status;
}
uint32_t vvvp_gpio_platform_now_ms(void) { return vvvp_gpio_diag_ms; }
uint32_t vvvp_gpio_platform_read_sw1(void) { return test_gpio_raw; }
void vvvp_gpio_platform_drive_led(uint32_t high)
{
    ++test_gpio_drive_count;
    test_gpio_mode = high;
}
