#include "gpio_diag_platform.h"
#include "gpio_diagnostics.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_rcc.h"

volatile uint32_t vvvp_gpio_diag_ms = 0U;
volatile uint32_t vvvp_gpio_diag_hclk_hz = 0U;

void SysTick_Handler(void)
{
    ++vvvp_gpio_diag_ms;
}

uint32_t vvvp_gpio_platform_init(void)
{
    static const uint8_t ahb_shifts[16] = {0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9};
    uint32_t hclk;
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    vvvp_gpio_platform_drive_led(0U); // Confirmed Active High: initial Engine OFF.
    // Reuse the verified Shield input circuit; no internal pull is added.
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_10, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_10, LL_GPIO_MODE_INPUT);

    // The existing Empty Project does not initialize SystemCoreClock.
    // Read actual HSI/AHB settings; do not change CPU/UART clocks or guess PLL/HSE.
    if (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
        return VVVP_GPIO_DIAG_CLOCK_UNSUPPORTED;
    }
    hclk = 16000000U >> ((RCC->CR & RCC_CR_HSIDIV) >> RCC_CR_HSIDIV_Pos);
    hclk >>= ahb_shifts[(RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos];
    vvvp_gpio_diag_hclk_hz = hclk;
    if (hclk < 1000U || hclk % 1000U != 0U) { return VVVP_GPIO_DIAG_CLOCK_UNSUPPORTED; }
    if ((SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) != 0U) { return VVVP_GPIO_DIAG_SYSTICK_BUSY; }
    vvvp_gpio_diag_ms = 0U;
    if (SysTick_Config(hclk / 1000U) != 0U) { return VVVP_GPIO_DIAG_CLOCK_UNSUPPORTED; }
    return VVVP_GPIO_DIAG_READY;
}

uint32_t vvvp_gpio_platform_now_ms(void) { return vvvp_gpio_diag_ms; }
uint32_t vvvp_gpio_platform_read_sw1(void)
{
    return LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_10) != 0U ? 1U : 0U;
}
void vvvp_gpio_platform_drive_led(uint32_t high)
{
    // Preload the latch before enabling output to avoid a wrong initial level.
    if (high != 0U) { LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_5); }
    else { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_5); }
    LL_GPIO_SetPinOutputType(GPIOA, LL_GPIO_PIN_5, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_5, LL_GPIO_SPEED_FREQ_LOW);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_NO);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);
}
