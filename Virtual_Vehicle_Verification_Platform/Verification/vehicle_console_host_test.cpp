// Host-only interactive UI/integration tests with a bounded 128-byte TX double.
// Compile with VehicleConsole/UartVehicleCommand/vehicle_runtime,
// EngineControllerV2, engine_control_v2_probe and the three original classes.
#include "VehicleConsole.h"
#include "vehicle_runtime.h"
#include "uart_echo_probe.h"
#include "engine_control_v2_probe.h"
#include <cfloat>
#include <climits>
#include <cstdio>
#include <deque>
#include <string>

volatile uint32_t vvvp_uart_banner_sent = 0U;

namespace {
unsigned checks = 0;
unsigned failures = 0;
unsigned rejected_writes = 0;
unsigned drain_rate = 1;
std::deque<int32_t> rx;
std::deque<char> tx;
std::string terminal;
const char menu[] =
    "\r\n=== VVVP Vehicle Control System ===\r\n"
    "1. Set Brake Position\r\n2. Set Gear Position\r\n"
    "3. Set Battery Voltage\r\n4. Show Vehicle Status\r\n"
    "5. Configure All\r\n6. Help\r\n7. GPIO Diagnostics\r\n\r\nSelect Menu: ";

void check(bool condition, const char* label)
{
    ++checks;
    if (!condition) { ++failures; std::printf("FAIL: %s\n", label); }
}
bool contains(const char* text) { return terminal.find(text) != std::string::npos; }
void pump(unsigned count = 6000)
{
    for (unsigned i = 0; i < count; ++i) {
        vvvp_vehicle_input_poll();
        for (unsigned j = 0; j < drain_rate && !tx.empty(); ++j) {
            terminal += tx.front();
            tx.pop_front();
        }
    }
}
void send(const std::string& text)
{
    terminal.clear();
    for (unsigned char c : text) { rx.push_back(c); }
    pump();
}
void menuReturned() { check(contains(menu), "menu returns after completed action"); }
}

extern "C" int32_t vvvp_uart_read_event(void)
{
    if (rx.empty()) { return VVVP_UART_RX_EMPTY; }
    const int32_t event = rx.front(); rx.pop_front(); return event;
}
extern "C" uint32_t vvvp_uart_try_write(const char* text, uint32_t length)
{
    if (length > 128U - tx.size()) { ++rejected_writes; return 0U; }
    for (uint32_t i = 0; i < length; ++i) { tx.push_back(text[i]); }
    return 1U;
}

int main()
{
    auto& runtime = vvvp::v2::vehicleRuntime();
    DriverInput* const original_input = &runtime.driverInput();
    VehicleState* const original_vehicle = &runtime.vehicleState();
    auto* const original_controller = &runtime.engineController();
    vvvp_vehicle_input_init();
    drain_rate = 0;
    pump(1000);
    check(tx.size() == 128U && terminal.empty() && rejected_writes > 0,
          "startup fills finite queue without discarding remaining menu");
    drain_rate = 1;
    pump();
    check(terminal == menu, "startup menu resumes exactly, no loss or duplication");
    check(vvvp_uart_banner_sent == 1U, "startup menu flag retained");

    send("1\r\n");
    check(terminal == "\r\nEnter Brake Position: ", "menu 1 and CRLF single transition");
    send("bad\n");
    check(contains("Invalid value.") && contains("Enter Brake Position: ") &&
          runtime.driverInput().getBrakePosition() == 0, "Brake error reasks without update");
    send("10\r");
    check(contains("Brake updated successfully.") && runtime.driverInput().getBrakePosition() == 10,
          "Brake valid value applied"); menuReturned();

    send("2\n"); check(contains("Enter Gear (P/R/N/D): "), "menu 2");
    send("p\n");
    check(contains("Invalid value.") && contains("Enter Gear (P/R/N/D): ") &&
          runtime.driverInput().getGearPosition() == GearPosition::P, "invalid Gear retries");
    send("D\r\n");
    check(contains("Gear updated successfully.") &&
          runtime.driverInput().getGearPosition() == GearPosition::D, "Gear value applied"); menuReturned();

    send("3\r"); check(contains("Enter Battery Voltage (V): "), "menu 3");
    const char* invalid_battery[] = {"NaN\n", "Inf\n", "1e999\n", "1e-999\n", "12,5\n"};
    for (const char* value : invalid_battery) {
        send(value);
        check(contains("Invalid value.") && contains("Enter Battery Voltage (V): ") &&
              runtime.vehicleState().getBatteryVoltage() == 0.0f, "Battery invalid value preserves state");
    }
    send(std::string(64, '1') + "\r\n");
    check(contains("Invalid value.") && !contains(menu) &&
          runtime.vehicleState().getBatteryVoltage() == 0.0f, "overlong value retries once");
    send("15.1\n");
    check(contains("Battery updated successfully.") &&
          runtime.vehicleState().getBatteryVoltage() == 15.1f, "Battery storage above 15V"); menuReturned();

    runtime.driverInput().setBrakePosition(-17);
    runtime.vehicleState().setBatteryVoltage(12.5f);
    runtime.vehicleState().setEngineState(true);
    runtime.vehicleState().setEngineRpm(123);
    runtime.vehicleState().setDashboardState(false);
    send("4\r\n");
    check(contains("Brake Position: -17\r\nGear Position: D\r\nBattery Voltage: 12.5 V\r\n"
                   "Engine State: ON\r\nEngine RPM: 123\r\nDashboard State: OFF\r\n"),
          "menu 4 reads actual getters, not the debugger snapshot"); menuReturned();
    runtime.vehicleState().setEngineState(false);
    runtime.vehicleState().setEngineRpm(0);

    send("5\n"); check(contains("Enter Brake Position: "), "Configure All starts with Brake");
    send("-3\n");
    check(runtime.driverInput().getBrakePosition() == -3 && contains("Enter Gear (P/R/N/D): "),
          "Configure Brake advances to Gear");
    send("X\n");
    check(runtime.driverInput().getBrakePosition() == -3 &&
          runtime.driverInput().getGearPosition() == GearPosition::D &&
          contains("Enter Gear (P/R/N/D): "), "Configure retry preserves previous accepted Brake");
    send("N\r\n");
    check(runtime.driverInput().getGearPosition() == GearPosition::N &&
          contains("Enter Battery Voltage (V): "), "Configure Gear advances to Battery");
    send("NaN\n");
    check(runtime.driverInput().getBrakePosition() == -3 &&
          runtime.driverInput().getGearPosition() == GearPosition::N &&
          runtime.vehicleState().getBatteryVoltage() == 12.5f &&
          contains("Enter Battery Voltage (V): "), "Configure Battery retry keeps earlier settings");
    send("-2.5\n");
    check(contains("Configuration Complete.") && contains("Brake Position: -3") &&
          contains("Gear Position: N") && contains("Battery Voltage: -2.5 V"),
          "Configure completes with actual state"); menuReturned();

    send("6\n");
    check(contains("signed 32-bit") && contains("finite decimal float") &&
          contains("Local echo and Local line editing = Force on."), "menu 6 help"); menuReturned();
    for (unsigned char c : terminal) { if (c > 127U) { check(false, "ASCII only output"); break; } }
    check(!contains("Console output error"), "help plus menu fits output storage");
    const char* bad_menu[] = {"0\n", "8\r", "12\r\n", "abc\n", "\n", "START\n"};
    for (const char* value : bad_menu) {
        send(value);
        check(contains("Invalid menu selection.") && contains(menu), "invalid menu redisplays menu");
    }
    send("BRAKE=7\r\n"); check(contains("OK\r\n") && runtime.driverInput().getBrakePosition() == 7,
                                  "legacy Brake supported at menu");
    send("GEAR=P\n"); check(contains("OK\r\n") && runtime.driverInput().getGearPosition() == GearPosition::P,
                              "legacy Gear supported at menu");
    send("BATTERY=16\n"); check(contains("OK\r\n") && runtime.vehicleState().getBatteryVoltage() == 16.0f,
                                  "legacy Battery supported without clamp");
    send("1\n"); send("GEAR=D\n");
    check(contains("Invalid value.") && contains("Enter Brake Position: ") &&
          runtime.driverInput().getGearPosition() == GearPosition::P, "legacy command cannot escape a question");
    send("8\n");

    send("3\n"); send("BRAKE=1\n");
    check(contains("Invalid value.") && runtime.driverInput().getBrakePosition() == 8,
          "command prefix rejected as Battery value"); send("1e2\n");
    check(runtime.vehicleState().getBatteryVoltage() == 100.0f, "exponent input retained");
    runtime.driverInput().setBrakePosition(INT_MIN);
    runtime.vehicleState().setBatteryVoltage(FLT_MAX);
    runtime.vehicleState().setEngineRpm(INT_MAX);
    runtime.vehicleState().setDashboardState(true);
    send("4\n");
    check(contains("Brake Position: -2147483648") && contains("Battery Voltage: 3.40282347e+38 V") &&
          contains("Engine RPM: 2147483647") && contains("Dashboard State: ON"), "extreme finite status fits and round-trips");

    terminal.clear(); drain_rate = 0;
    rx.push_back('6'); rx.push_back('\n'); pump(1000);
    check(tx.size() == 128U, "help stalls at bounded TX capacity");
    drain_rate = 1; pump();
    const std::string slow_help = terminal;
    send("6\n");
    check(terminal == slow_help && contains(menu), "saturated help resumes without loss or duplicates");

    send("2\n"); terminal.clear(); rx.push_back(VVVP_UART_RX_FAULT);
    rx.push_back('D'); rx.push_back('\n'); pump();
    check(contains("Invalid value.") && contains("Enter Gear (P/R/N/D): "), "RX fault reasks current question");
    send("N\n");
    check(original_input == &runtime.driverInput() && original_vehicle == &runtime.vehicleState() &&
          original_controller == &runtime.engineController(), "runtime and controller identities preserved");
    check(!runtime.driverInput().isEngineButtonPressed() && !runtime.vehicleState().getEngineState(),
          "menu never generates engine requests");
    vvvp_run_engine_control_v2_probe();
    check(vvvp_v2_probe_completed == 1U && vvvp_v2_probe_passed_count == 14U &&
          vvvp_v2_probe_all_passed == 1U, "all 14 original V2 control scenarios preserved");

    // Direct state-machine checks use the same production UI class.
    DriverInput input; VehicleState vehicle;
    vvvp::v2::VehicleConsole ui(input, vehicle); ui.begin();
    check(ui.state() == vvvp::v2::VehicleConsole::Menu, "explicit Menu state");
    ui.consumeOutput(ui.outputSize()); ui.receive('5'); ui.receive('\n');
    check(ui.state() == vvvp::v2::VehicleConsole::ConfigureBrake, "explicit ConfigureBrake state");
    ui.consumeOutput(ui.outputSize()); ui.receive('1'); ui.receive('\n');
    check(ui.state() == vvvp::v2::VehicleConsole::ConfigureGear, "explicit ConfigureGear state");
    ui.consumeOutput(ui.outputSize()); ui.receive('P'); ui.receive('\n');
    check(ui.state() == vvvp::v2::VehicleConsole::ConfigureBattery, "explicit ConfigureBattery state");
    ui.consumeOutput(ui.outputSize()); ui.receive('0'); ui.receive('\n');
    check(ui.state() == vvvp::v2::VehicleConsole::Menu, "explicit return to Menu");

    std::printf("%u menu checks, %u failures; %u bounded TX retries\n", checks, failures, rejected_writes);
    return failures == 0 ? 0 : 1;
}
