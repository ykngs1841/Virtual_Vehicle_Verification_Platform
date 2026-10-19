#include "vehicle_runtime.h"
#include "VehicleConsole.h"
#include "uart_echo_probe.h"
#include "EngineButton.h"
#include "gpio_diagnostics.h"
#include "gpio_diag_platform.h"

volatile VvvpRuntimeSnapshot vvvp_runtime_state = {};
volatile uint32_t vvvp_command_ok_count = 0U;
volatile uint32_t vvvp_command_err_count = 0U;
volatile uint32_t vvvp_sw1_event_count = 0U;
volatile uint32_t vvvp_sw1_debounced_pressed = 0U;
volatile uint32_t vvvp_sw1_armed = 0U;

namespace vvvp {
namespace v2 {
VehicleRuntime::VehicleRuntime()
    : driver_input_(), vehicle_state_(),
      engine_controller_(driver_input_, vehicle_state_)
{
}
}
}

namespace {
// Static storage: constructors run via the existing C++ startup before main.
// Independent of the temporary objects used by the preserved validation probes.
vvvp::v2::VehicleRuntime runtime;
vvvp::v2::VehicleConsole console(runtime.driverInput(), runtime.vehicleState());
bool startup_menu_pending = true;
vvvp::v2::EngineButton engine_button;

void observe()
{
    const DriverInput& input = runtime.driverInput();
    const VehicleState& state = runtime.vehicleState();
    vvvp_runtime_state.brake_position = input.getBrakePosition();
    vvvp_runtime_state.gear_position = static_cast<uint32_t>(input.getGearPosition());
    vvvp_runtime_state.battery_voltage = state.getBatteryVoltage();
    vvvp_runtime_state.engine_state = state.getEngineState() ? 1U : 0U;
    vvvp_runtime_state.engine_rpm = state.getEngineRpm();
    vvvp_runtime_state.dashboard_state = state.getDashboardState() ? 1U : 0U;
    vvvp_runtime_state.engine_button_event = input.isEngineButtonPressed() ? 1U : 0U;
}
}

vvvp::v2::VehicleRuntime& vvvp::v2::vehicleRuntime()
{
    return runtime;
}

void vvvp_vehicle_control_init(void)
{
    if (vvvp_gpio_diag_status == VVVP_GPIO_DIAG_READY) {
        engine_button.reset(vvvp_gpio_platform_read_sw1() != 0U, vvvp_gpio_platform_now_ms());
        vvvp_sw1_debounced_pressed = engine_button.pressed() ? 1U : 0U;
        vvvp_sw1_armed = engine_button.armed() ? 1U : 0U;
    }
    vvvp_gpio_led_set_engine_state(runtime.vehicleState().getEngineState() ? 1U : 0U);
    observe();
}

void vvvp_vehicle_control_poll(void)
{
    if (vvvp_gpio_diag_status == VVVP_GPIO_DIAG_READY) {
        if (engine_button.sample(vvvp_gpio_platform_read_sw1() != 0U,
                                 vvvp_gpio_platform_now_ms())) {
            ++vvvp_sw1_event_count;
            runtime.driverInput().pressEngineButton();
            console.queueFailure(runtime.engineController().processEngineButton());
        }
        vvvp_sw1_debounced_pressed = engine_button.pressed() ? 1U : 0U;
        vvvp_sw1_armed = engine_button.armed() ? 1U : 0U;
    }
    // Remember state even during a diagnostic override. Expiry restores THIS state.
    vvvp_gpio_led_set_engine_state(runtime.vehicleState().getEngineState() ? 1U : 0U);
    vvvp_gpio_diag_poll();
    observe(); // Physical transitions remain visible even with a full UART TX queue.
}

void vvvp_vehicle_input_init(void)
{
    observe(); // Preserve the real class constructor defaults; no new defaults.
    console.begin();
    startup_menu_pending = true;
}

void vvvp_vehicle_input_poll(void)
{
    console.pollFailureOutput();
    if (console.outputSize() != 0U) {
        const uint32_t remaining = console.outputSize();
        const uint32_t chunk = remaining < 32U ? remaining : 32U;
        if (vvvp_uart_try_write(console.outputData(), chunk) != 0U) {
            console.consumeOutput(chunk);
            if (startup_menu_pending && console.outputSize() == 0U) {
                // Initial menu queued, not proof of terminal reception.
                vvvp_uart_banner_sent = 1U;
                startup_menu_pending = false;
            }
        }
        return; // Retry retained bytes next poll; the UART poll still runs.
    }
    const int32_t event = vvvp_uart_read_event();
    if (event == VVVP_UART_RX_EMPTY) { return; }
    if (event == VVVP_UART_RX_FAULT) {
        console.invalidateLine(); // Discard through the next intact terminator.
        return;
    }
    const vvvp::v2::UartVehicleCommand::Result result =
        console.receive(static_cast<uint8_t>(event));
    if (result == vvvp::v2::UartVehicleCommand::Incomplete) { return; }
    observe();
    if (result == vvvp::v2::UartVehicleCommand::Accepted) {
        ++vvvp_command_ok_count;
    } else {
        ++vvvp_command_err_count;
    }
}
