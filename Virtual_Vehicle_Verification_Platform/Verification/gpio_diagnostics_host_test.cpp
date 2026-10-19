#include "gpio_diagnostics.h"
#include "gpio_diag_platform_fake.h"
#include "vehicle_runtime.h"
#include "uart_echo_probe.h"
#include <cstdio>
#include <deque>
#include <string>

volatile uint32_t vvvp_uart_banner_sent = 0U;
namespace {
unsigned checks = 0;
unsigned failures = 0;
std::deque<int32_t> rx;
std::string output;
void check(bool condition, const char* label)
{
    ++checks;
    if (!condition) { ++failures; std::printf("FAIL: %s\n", label); }
}
void send(const std::string& input)
{
    output.clear();
    for (unsigned char c : input) { rx.push_back(c); }
    for (unsigned i = 0; i < 3000; ++i) {
        vvvp_gpio_diag_poll();
        vvvp_vehicle_input_poll();
    }
}
bool has(const char* text) { return output.find(text) != std::string::npos; }
}
extern "C" int32_t vvvp_uart_read_event(void)
{
    if (rx.empty()) { return VVVP_UART_RX_EMPTY; }
    int32_t byte = rx.front(); rx.pop_front(); return byte;
}
extern "C" uint32_t vvvp_uart_try_write(const char* text, uint32_t length)
{
    output.append(text, length); return 1U;
}
int main()
{
    vvvp_gpio_diag_init();
    check(vvvp_gpio_diag_status == VVVP_GPIO_DIAG_READY && test_gpio_mode == VVVP_GPIO_LED_LOW,
          "initialization drives verified Active High LED OFF");
    vvvp_vehicle_input_init(); send("");
    check(has("1. Set Brake Position") && has("6. Help") && has("7. GPIO Diagnostics"), "original menus retained");
    send("BRAKE=10\nGEAR=P\nBATTERY=12.5\n");
    auto& runtime = vvvp::v2::vehicleRuntime();
    auto* const original_controller = &runtime.engineController();
    runtime.vehicleState().setEngineState(true);
    runtime.vehicleState().setEngineRpm(850);
    runtime.vehicleState().setDashboardState(true);
    vvvp_gpio_led_set_engine_state(1U);
    send("7\r\n");
    check(has("SW1 D2/PA10 Active Low") && has("Select Diagnostic:"), "verified diagnostic screen");
    test_gpio_raw = 1U; send("1\n");
    check(has("raw: 1. First sample"), "released raw sample remains observable");
    test_gpio_raw = 0U; send("1\r\n");
    check(has("previous=1 current=0 (CHANGED)"), "pressed sample compared to preceding sample");
    send("1\n");
    check(has("previous=0 current=0 (UNCHANGED)") && has("check the pin/wiring"),
          "unchanged raw value is reported");
    test_gpio_raw = 1U; send("1\n");
    check(has("previous=0 current=1 (CHANGED)"), "reverse transition also reported raw");
    send("2\n");
    check(has("PA5 drive: LOW") && test_gpio_mode == VVVP_GPIO_LED_LOW, "diagnostic Low overrides engine ON indication");
    vvvp_gpio_diag_ms = 999U; vvvp_gpio_diag_poll();
    check(test_gpio_mode == VVVP_GPIO_LED_LOW, "test active before 1000ms");
    vvvp_gpio_diag_ms = 1000U; vvvp_gpio_diag_poll();
    check(test_gpio_mode == VVVP_GPIO_LED_HIGH && vvvp_gpio_led_drive == VVVP_GPIO_LED_HIGH,
          "automatic return to latest engine ON indication");
    send("3\n");
    check(has("PA5 drive: HIGH") && test_gpio_mode == VVVP_GPIO_LED_HIGH, "explicit High test");
    send("4\n");
    check(test_gpio_mode == VVVP_GPIO_LED_HIGH && has("follows the latest Engine State"), "manual test end restores engine ON");
    send("3\n"); send("0\n");
    check(test_gpio_mode == VVVP_GPIO_LED_HIGH && has("Select Menu:"), "leaving diagnostics restores engine ON");
    send("7\n"); send("9\n");
    check(has("Invalid diagnostic selection") && has("Select Diagnostic:"), "invalid selection retries diagnostics");
    send("BRAKE=99\n");
    check(runtime.driverInput().getBrakePosition() == 10 && has("Invalid diagnostic selection"),
          "diagnostic mode cannot change vehicle via command text");
    check(runtime.vehicleState().getEngineState() && runtime.vehicleState().getEngineRpm() == 850 &&
          runtime.vehicleState().getDashboardState() && runtime.vehicleState().getBatteryVoltage() == 12.5f &&
          runtime.driverInput().getGearPosition() == GearPosition::P &&
          !runtime.driverInput().isEngineButtonPressed() && original_controller == &runtime.engineController(),
          "GPIO tests preserve vehicle state, engine event and shared controller");
    send("0\n"); send("4\n");
    check(has("Engine State: ON") && has("Engine RPM: 850") && has("Dashboard State: ON"), "vehicle status still reads same runtime");

    vvvp_gpio_diag_ms = 0xfffffe00U;
    check(vvvp_gpio_diag_test_led(0U) == 1U, "start close to tick wrap");
    vvvp_gpio_diag_ms += 999U; vvvp_gpio_diag_poll();
    check(test_gpio_mode == VVVP_GPIO_LED_LOW, "wrap keeps test active through 999ms");
    ++vvvp_gpio_diag_ms; vvvp_gpio_diag_poll();
    check(test_gpio_mode == VVVP_GPIO_LED_HIGH, "wrap-safe expiry restores engine ON at 1000ms");
    const uint32_t drives = test_gpio_drive_count;
    check(vvvp_gpio_diag_test_led(2U) == 0U && test_gpio_drive_count == drives, "reject invalid electrical level");

    test_gpio_status = VVVP_GPIO_DIAG_CLOCK_UNSUPPORTED; vvvp_gpio_diag_init();
    test_gpio_raw = 0;
    check(vvvp_gpio_diag_read_sw1() == 0, "raw read available without timer");
    check(vvvp_gpio_diag_test_led(1U) == 0U && test_gpio_mode == VVVP_GPIO_LED_HIGH,
          "unsupported clock blocks timed output");
    test_gpio_status = VVVP_GPIO_DIAG_SYSTICK_BUSY; vvvp_gpio_diag_init();
    check(vvvp_gpio_diag_test_led(0U) == 0U && test_gpio_mode == VVVP_GPIO_LED_HIGH,
          "busy timebase does not allow indefinite test output");
    test_gpio_status = VVVP_GPIO_DIAG_READY; vvvp_gpio_diag_init();
    vvvp_gpio_diag_test_led(1U); vvvp_vehicle_input_init();
    check(test_gpio_mode == VVVP_GPIO_LED_HIGH, "UI restart returns LED ownership to engine");
    std::printf("%u GPIO diagnostic checks, %u failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}

