#include "gpio_diagnostics.h"
#include "gpio_diag_platform.h"

volatile uint32_t vvvp_gpio_diag_status = VVVP_GPIO_DIAG_NOT_READY;
volatile int32_t vvvp_gpio_sw1_raw = -1;
volatile uint32_t vvvp_gpio_led_drive = VVVP_GPIO_LED_HIGH_Z;
volatile uint32_t vvvp_gpio_led_owner = VVVP_GPIO_LED_ENGINE;
namespace {
uint32_t test_started_ms = 0U;
const uint32_t test_duration_ms = 1000U;
uint32_t engine_led_level = VVVP_GPIO_LED_LOW;

// One PA5 arbiter. Diagnostic and engine paths never write it independently.
void drive(uint32_t level)
{
    if (vvvp_gpio_diag_status != VVVP_GPIO_DIAG_NOT_READY && vvvp_gpio_led_drive != level) {
        vvvp_gpio_platform_drive_led(level);
        vvvp_gpio_led_drive = level;
    }
}
}

void vvvp_gpio_led_set_engine_state(uint32_t engine_on)
{
    engine_led_level = engine_on != 0U ? VVVP_GPIO_LED_HIGH : VVVP_GPIO_LED_LOW;
    if (vvvp_gpio_led_owner == VVVP_GPIO_LED_ENGINE) { drive(engine_led_level); }
}

void vvvp_gpio_diag_end_led_test(void)
{
    vvvp_gpio_led_owner = VVVP_GPIO_LED_ENGINE;
    drive(engine_led_level); // Restore the latest state, never a saved LED level.
}

void vvvp_gpio_diag_init(void)
{
    vvvp_gpio_diag_status = vvvp_gpio_platform_init();
    vvvp_gpio_sw1_raw = -1;
    vvvp_gpio_led_owner = VVVP_GPIO_LED_ENGINE;
    // Platform initialization preloads LOW before enabling the PA5 output.
    vvvp_gpio_led_drive = vvvp_gpio_diag_status == VVVP_GPIO_DIAG_NOT_READY ?
        VVVP_GPIO_LED_HIGH_Z : VVVP_GPIO_LED_LOW;
    drive(engine_led_level);
}

void vvvp_gpio_diag_poll(void)
{
    if (vvvp_gpio_diag_status == VVVP_GPIO_DIAG_NOT_READY) { return; }
    vvvp_gpio_sw1_raw = static_cast<int32_t>(vvvp_gpio_platform_read_sw1());
    if (vvvp_gpio_led_owner == VVVP_GPIO_LED_DIAGNOSTIC &&
        static_cast<uint32_t>(vvvp_gpio_platform_now_ms() - test_started_ms) >= test_duration_ms) {
        vvvp_gpio_diag_end_led_test();
    }
}

int32_t vvvp_gpio_diag_read_sw1(void)
{
    vvvp_gpio_diag_poll();
    return vvvp_gpio_sw1_raw;
}

uint32_t vvvp_gpio_diag_test_led(uint32_t high)
{
    if (vvvp_gpio_diag_status != VVVP_GPIO_DIAG_READY || high > 1U) { return 0U; }
    vvvp_gpio_led_owner = VVVP_GPIO_LED_DIAGNOSTIC;
    drive(high);
    test_started_ms = vvvp_gpio_platform_now_ms();
    return 1U;
}
