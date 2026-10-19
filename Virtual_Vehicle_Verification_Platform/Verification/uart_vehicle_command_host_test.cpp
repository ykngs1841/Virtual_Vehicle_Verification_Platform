// Host-only tests; Verification/ is outside CubeIDE's Src source entry.
// Compile with UartVehicleCommand.cpp, VehicleConsole.cpp, vehicle_runtime.cpp, EngineControllerV2.cpp
// and the linked V1.0 DriverInput/VehicleState/EngineController implementations.
// UART is replaced by a test double: these tests do not prove MCU UART timing.
#include "UartVehicleCommand.h"
#include "vehicle_runtime.h"
#include "uart_echo_probe.h"
#include <climits>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>

namespace {
unsigned checks = 0U;
unsigned failures = 0U;
std::deque<int32_t> events;
std::string responses;
bool tx_blocked = false;

void check(bool condition, const char* label)
{
    ++checks;
    if (!condition) { ++failures; std::printf("FAIL: %s\n", label); }
}

struct State {
    int brake;
    GearPosition gear;
    float battery;
    bool engine;
    int rpm;
    bool dashboard;
    bool button;
    State(const DriverInput& d, const VehicleState& v)
        : brake(d.getBrakePosition()), gear(d.getGearPosition()),
          battery(v.getBatteryVoltage()), engine(v.getEngineState()),
          rpm(v.getEngineRpm()), dashboard(v.getDashboardState()),
          button(d.isEngineButtonPressed()) {}
    bool same(const State& s) const {
        return brake == s.brake && gear == s.gear && battery == s.battery &&
               engine == s.engine && rpm == s.rpm && dashboard == s.dashboard &&
               button == s.button;
    }
};

std::vector<vvvp::v2::UartVehicleCommand::Result> feed(
    vvvp::v2::UartVehicleCommand& parser, const std::string& text)
{
    std::vector<vvvp::v2::UartVehicleCommand::Result> result;
    for (unsigned char byte : text) {
        const auto r = parser.receive(byte);
        if (r != vvvp::v2::UartVehicleCommand::Incomplete) { result.push_back(r); }
    }
    return result;
}

void checkCommand(const std::string& text, bool accepted, int field = 0,
                  int integer = 0, float battery = 0.0f)
{
    DriverInput input;
    VehicleState state;
    input.setBrakePosition(7);
    input.setGearPosition(GearPosition::R);
    input.pressEngineButton();
    state.setBatteryVoltage(9.5f);
    state.setEngineState(true);
    state.setEngineRpm(432);
    state.setDashboardState(true);
    State expected(input, state);
    vvvp::v2::UartVehicleCommand parser(input, state);
    const auto result = feed(parser, text);
    check(result.size() == 1U && result[0] ==
          (accepted ? vvvp::v2::UartVehicleCommand::Accepted :
                      vvvp::v2::UartVehicleCommand::Rejected), text.c_str());
    if (accepted) {
        if (field == 1) { expected.brake = integer; }
        if (field == 2) { expected.gear = static_cast<GearPosition>(integer); }
        if (field == 3) { expected.battery = battery; }
    }
    check(expected.same(State(input, state)), "only the intended state changes");
}

void enqueue(const std::string& text)
{
    for (unsigned char byte : text) { events.push_back(byte); }
}

void pump(unsigned count = 2000U)
{
    for (unsigned i = 0; i < count; ++i) { vvvp_vehicle_input_poll(); }
}

// Preserve legacy assertions about OK/ERR while allowing the new menu/prompt UI.
std::string commandReplies()
{
    std::string result;
    size_t start = 0;
    while (start < responses.size()) {
        const size_t end = responses.find("\r\n", start);
        if (end == std::string::npos) { break; }
        const std::string line = responses.substr(start, end - start);
        if (line == "OK" || line == "ERR") { result += line + "\r\n"; }
        start = end + 2U;
    }
    return result;
}
}

volatile uint32_t vvvp_uart_banner_sent = 0U;

extern "C" int32_t vvvp_uart_read_event(void)
{
    if (events.empty()) { return VVVP_UART_RX_EMPTY; }
    const int32_t event = events.front();
    events.pop_front();
    return event;
}

extern "C" uint32_t vvvp_uart_try_write(const char* text, uint32_t length)
{
    if (tx_blocked) { return 0U; }
    responses.append(text, length);
    return 1U;
}

int main()
{
    static_assert(sizeof(int) == 4, "The MCU Brake API uses a 32-bit int");
    checkCommand("BRAKE=1\r", true, 1, 1);
    checkCommand("BRAKE=0\n", true, 1, 0);
    checkCommand("BRAKE=-3\r\n", true, 1, -3);
    checkCommand("BRAKE=+42\n", true, 1, 42);
    checkCommand("BRAKE=2147483647\n", true, 1, INT_MAX);
    checkCommand("BRAKE=-2147483648\n", true, 1, INT_MIN);
    checkCommand("GEAR=P\n", true, 2, 0);
    checkCommand("GEAR=R\n", true, 2, 1);
    checkCommand("GEAR=N\n", true, 2, 2);
    checkCommand("GEAR=D\n", true, 2, 3);
    checkCommand("BATTERY=12.5\n", true, 3, 0, 12.5f);
    checkCommand("BATTERY=11.5\r", true, 3, 0, 11.5f);
    checkCommand("BATTERY=15.0\r\n", true, 3, 0, 15.0f);
    checkCommand("BATTERY=15.1\n", true, 3, 0, 15.1f);
    checkCommand("BATTERY=-2.5\n", true, 3, 0, -2.5f);
    checkCommand("BATTERY=+1.25e1\n", true, 3, 0, 12.5f);
    checkCommand("BATTERY=.5\n", true, 3, 0, .5f);
    checkCommand("BATTERY=1.\n", true, 3, 0, 1.0f);
    checkCommand("BATTERY=0\n", true, 3, 0, 0.0f);
    const char* rejected[] = {
        "\n", "UNKNOWN=1\n", "START\n", "STOP\n", "brake=1\n",
        "BRAKE=\n", "BRAKE=1.0\n", "BRAKE=2147483648\n",
        "BRAKE=-2147483649\n", "BRAKE=999999999999999999999999999999\n",
        "BRAKE= 1\n", "BRAKE=1 \n", "BRAKE=1x\n", "BRAKE=0x10\n",
        "GEAR=\n", "GEAR=X\n", "GEAR=p\n", "GEAR=PN\n", "GEAR=1\n",
        "BATTERY=\n", "BATTERY=NaN\n", "BATTERY=nan(1)\n", "BATTERY=Inf\n",
        "BATTERY=-Infinity\n", "BATTERY=1e999\n", "BATTERY=3.5e38\n",
        "BATTERY=1e-999\n", "BATTERY=12.5x\n", "BATTERY=12,5\n",
        "BATTERY=0x1p2\n", "BATTERY=.\n", "BATTERY=1e\n", "BATTERY= 12\n",
        "BRAKE=1;GEAR=P\n"
    };
    for (const char* text : rejected) { checkCommand(text, false); }
    checkCommand(std::string("BRAKE=1\0junk\n", 13), false);
    checkCommand("BRAKE=1\b\n", false);
    checkCommand(std::string("BRAKE=") + "\xff" + "1\n", false);
    checkCommand(std::string("BATTERY=") + std::string(55, '0') + "\n", true, 3);
    checkCommand(std::string("BATTERY=") + std::string(56, '0') + "\n", false);

    DriverInput input;
    VehicleState state;
    vvvp::v2::UartVehicleCommand parser(input, state);
    check(feed(parser, "BRAKE=").empty() && input.getBrakePosition() == 0,
          "partial input does not modify state");
    check(feed(parser, "9\r\n").size() == 1U && input.getBrakePosition() == 9,
          "fragmented CRLF applies once");
    check(feed(parser, std::string(100, 'X') + "BRAKE=1\r\n")[0] ==
          vvvp::v2::UartVehicleCommand::Rejected && input.getBrakePosition() == 9,
          "overflow rejects entire line and suffix");
    check(feed(parser, "BRAKE=2\n")[0] == vvvp::v2::UartVehicleCommand::Accepted &&
          input.getBrakePosition() == 2, "recovery after overflow");
    check(feed(parser, "GEAR=P\rGEAR=N\nGEAR=D\r\n").size() == 3U &&
          input.getGearPosition() == GearPosition::D, "mixed terminators");

    vvvp_vehicle_input_init();
    pump();
    responses.clear();
    auto& runtime = vvvp::v2::vehicleRuntime();
    check(&runtime == &vvvp::v2::vehicleRuntime(), "one persistent runtime");
    enqueue("BRAKE=-4\r\nGEAR=N\nBATTERY=15.1\r");
    pump();
    check(commandReplies() == "OK\r\nOK\r\nOK\r\n", "runtime replies once per command");
    check(runtime.driverInput().getBrakePosition() == -4 &&
          runtime.driverInput().getGearPosition() == GearPosition::N &&
          runtime.vehicleState().getBatteryVoltage() == 15.1f,
          "UART path writes persistent real objects");
    check(vvvp_runtime_state.brake_position == -4 &&
          vvvp_runtime_state.gear_position == 2U &&
          vvvp_runtime_state.battery_voltage == 15.1f,
          "debugger snapshot reflects real getters");
    const State before(runtime.driverInput(), runtime.vehicleState());
    responses.clear();
    enqueue("BATTERY=NaN\nSTART\nGEAR=X\n");
    pump();
    check(commandReplies() == "ERR\r\nERR\r\nERR\r\n" &&
          before.same(State(runtime.driverInput(), runtime.vehicleState())),
          "invalid runtime commands preserve entire state");
    check(!runtime.vehicleState().getEngineState() &&
          runtime.vehicleState().getEngineRpm() == 0 &&
          !runtime.driverInput().isEngineButtonPressed(), "no UART engine request");

    responses.clear();
    tx_blocked = true;
    enqueue("BRAKE=8\nBRAKE=9\n");
    pump();
    check(runtime.driverInput().getBrakePosition() == 8 && responses.empty(),
          "blocked response retained and later commands paused");
    tx_blocked = false;
    pump();
    check(runtime.driverInput().getBrakePosition() == 9 &&
          commandReplies() == "OK\r\nOK\r\n", "resume without lost response");

    responses.clear();
    enqueue("BRAKE=");
    events.push_back(VVVP_UART_RX_FAULT);
    enqueue("22\nGEAR=P\n");
    pump();
    check(runtime.driverInput().getBrakePosition() == 9 &&
          runtime.driverInput().getGearPosition() == GearPosition::P &&
          commandReplies() == "ERR\r\nOK\r\n", "transport fault rejects damaged line");
    std::printf("%u checks, %u failures\n", checks, failures);
    return failures == 0U ? 0 : 1;
}
