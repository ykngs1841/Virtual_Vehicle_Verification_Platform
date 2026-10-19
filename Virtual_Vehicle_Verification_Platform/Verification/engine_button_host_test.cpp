// Host integration tests. Real runtime/controller/console; simulated GPIO and time.
#include "EngineButton.h"
#include "vehicle_runtime.h"
#include "gpio_diagnostics.h"
#include "gpio_diag_platform_fake.h"
#include "uart_echo_probe.h"
#include "engine_control_v2_probe.h"
#include <cstdio>
#include <deque>
#include <string>

volatile uint32_t vvvp_uart_banner_sent = 0U;
namespace {
unsigned checks = 0;
unsigned failures = 0;
std::deque<int32_t> rx;
std::deque<char> tx;
std::string terminal;
bool tx_blocked = false;

void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL: %s\n", label); }
}
void poll()
{
    vvvp_vehicle_control_poll();
    vvvp_vehicle_input_poll();
    if (!tx_blocked && !tx.empty()) { terminal += tx.front(); tx.pop_front(); }
}
void pump(unsigned count = 5000)
{
    for (unsigned i = 0; i < count; ++i) { poll(); }
}
void send(const std::string& text)
{
    terminal.clear();
    for (unsigned char c : text) { rx.push_back(c); }
    pump();
}
void raw(uint32_t level) { test_gpio_raw = level; poll(); }
void elapsed(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; ++i) { ++vvvp_gpio_diag_ms; poll(); }
}
void press() { raw(0U); elapsed(30); }
void release() { raw(1U); elapsed(30); }
void reset(uint32_t initial_raw = 1U)
{
    tx_blocked = false; rx.clear(); tx.clear(); terminal.clear();
    auto& r = vvvp::v2::vehicleRuntime();
    r.driverInput().setBrakePosition(0);
    r.driverInput().setGearPosition(GearPosition::P);
    r.driverInput().clearEngineButtonEvent();
    r.vehicleState().setBatteryVoltage(0.0f);
    r.vehicleState().setEngineState(false);
    r.vehicleState().setEngineRpm(0);
    r.vehicleState().setDashboardState(false);
    test_gpio_raw = initial_raw;
    test_gpio_status = VVVP_GPIO_DIAG_READY;
    vvvp_gpio_diag_ms = 0U;
    vvvp_gpio_led_set_engine_state(0U);
    vvvp_gpio_diag_init();
    vvvp_vehicle_input_init();
    vvvp_vehicle_control_init();
    pump();
}
void expectEngine(bool on, const char* label)
{
    auto& r = vvvp::v2::vehicleRuntime();
    check(r.vehicleState().getEngineState() == on && r.vehicleState().getEngineRpm() == (on ? 850 : 0) &&
          r.vehicleState().getDashboardState() == on, label);
}
void expectLed(bool on, const char* label)
{
    const uint32_t expected = on ? VVVP_GPIO_LED_HIGH : VVVP_GPIO_LED_LOW;
    check(test_gpio_mode == expected &&
          vvvp_gpio_led_drive == test_gpio_mode && vvvp_gpio_led_owner == VVVP_GPIO_LED_ENGINE, label);
}
void validInputs() { send("BRAKE=10\nGEAR=P\nBATTERY=12.5\n"); }
void rejection(const char* command, const char* label)
{
    reset(); validInputs(); send(command);
    const uint32_t events = vvvp_sw1_event_count;
    const uint32_t writes = test_gpio_drive_count;
    press(); expectEngine(false, label); expectLed(false, "rejected Start keeps LED OFF");
    check(vvvp_sw1_event_count == events + 1 && test_gpio_drive_count == writes,
          "failed request emits once without rewriting unchanged LED");
    check(!vvvp::v2::vehicleRuntime().driverInput().isEngineButtonPressed(), "controller consumes rejected event");
}
}
extern "C" int32_t vvvp_uart_read_event(void)
{
    if (rx.empty()) { return VVVP_UART_RX_EMPTY; }
    const int32_t event = rx.front(); rx.pop_front(); return event;
}
extern "C" uint32_t vvvp_uart_try_write(const char* text, uint32_t length)
{
    if (length > 128U - tx.size()) { return 0U; }
    for (uint32_t i = 0; i < length; ++i) { tx.push_back(text[i]); }
    return 1U;
}

int main()
{
    vvvp::v2::EngineButton button;
    button.reset(true, 0U);
    check(!button.sample(true, 100U), "Active Low: HIGH is released");
    check(!button.sample(false, 101U) && !button.sample(false, 130U), "LOW must persist 30ms");
    check(button.sample(false, 131U), "30ms LOW emits exactly one event");
    check(!button.sample(false, 10000U), "long hold emits no duplicate");
    button.sample(true, 10001U);
    check(!button.sample(true, 10030U) && !button.armed(), "release also needs 30ms");
    button.sample(false, 10031U); button.sample(true, 10032U); button.sample(true, 10062U);
    check(button.armed(), "release bounce settles before rearming");
    button.sample(false, 10063U);
    check(button.sample(false, 10093U), "release and press emits a new event");
    button.reset(true, 0U);
    button.sample(false, 1U); button.sample(true, 10U); button.sample(false, 20U);
    check(!button.sample(false, 49U) && button.sample(false, 50U), "press bounce restarts stable interval");
    button.reset(false, 0U);
    check(!button.sample(false, 1000U) && !button.armed(), "held-at-boot produces no event");
    button.sample(true, 1001U); button.sample(true, 1031U); button.sample(false, 1032U);
    check(button.sample(false, 1062U), "held-at-boot must release then press");
    button.reset(true, 0xfffffff0U); button.sample(false, 0xfffffff1U);
    check(!button.sample(false, 14U) && button.sample(false, 15U), "debounce survives millisecond wrap");

    reset(); expectEngine(false, "initial engine OFF"); expectLed(false, "initial PA5 LOW");
    auto& runtime = vvvp::v2::vehicleRuntime();
    auto* const original_input = &runtime.driverInput();
    auto* const original_state = &runtime.vehicleState();
    auto* const original_controller = &runtime.engineController();
    validInputs();
    const uint32_t initial_events = vvvp_sw1_event_count;
    raw(0U); elapsed(29); expectEngine(false, "no Start before debounce interval");
    elapsed(1); expectEngine(true, "normal Start through existing V2 controller"); expectLed(true, "Start lights LED1");
    check(vvvp_runtime_state.engine_state == 1U && vvvp_runtime_state.engine_rpm == 850 &&
          vvvp_runtime_state.dashboard_state == 1U && vvvp_sw1_debounced_pressed == 1U,
          "physical transitions update preserved debugger snapshot");
    elapsed(2000); check(vvvp_sw1_event_count == initial_events + 1U, "long hold processes one request");
    expectEngine(true, "long hold cannot immediately Stop");
    release(); press(); expectEngine(false, "normal Stop"); expectLed(false, "Stop extinguishes LED1");

    rejection("BRAKE=0\n", "Start rejected for Brake zero");
    rejection("GEAR=D\n", "Start rejected for Gear D");
    rejection("BATTERY=11.4\n", "Start rejected below 11.5V");
    rejection("BATTERY=15.1\n", "Start rejected above 15.0V");
    check(runtime.vehicleState().getBatteryVoltage() == 15.1f, "Start rejection does not clamp stored Battery");
    // Fixing a failed Start while SW1 remains held must not create a new request.
    send("BATTERY=12.5\n"); elapsed(1000); expectEngine(false, "fixing input while held does not retry Start");
    release(); press(); expectEngine(true, "new press after release retries successfully");
    release(); send("BRAKE=0\n");
    const uint32_t before_failed_stop = test_gpio_drive_count;
    press(); expectEngine(true, "Stop rejected for Brake zero"); expectLed(true, "failed Stop keeps LED1 ON");
    check(test_gpio_drive_count == before_failed_stop, "failed Stop does not rewrite unchanged LED");
    release(); send("BRAKE=1\nGEAR=D\nBATTERY=100\n"); press();
    expectEngine(false, "Stop ignores Gear D and high Battery"); expectLed(false, "Stop LED OFF with high Battery");
    reset(); validInputs(); send("BATTERY=11.5\nGEAR=N\n"); press();
    expectEngine(true, "11.5V inclusive boundary and Gear N accepted");
    release(); send("GEAR=D\nBATTERY=-2\n"); press(); expectEngine(false, "Stop ignores negative Battery and Gear D");
    reset(); validInputs(); send("BATTERY=15.0\n"); press(); expectEngine(true, "15.0V inclusive boundary accepted");

    reset(0U); validInputs(); const uint32_t boot_events = vvvp_sw1_event_count;
    elapsed(1000); expectEngine(false, "runtime suppresses SW1 held during initialization");
    check(vvvp_sw1_event_count == boot_events && vvvp_sw1_armed == 0U, "boot hold creates zero events");
    release(); press(); expectEngine(true, "boot hold followed by release/new press Starts");

    reset(); validInputs(); send("7\n2\n");
    check(vvvp_gpio_led_owner == VVVP_GPIO_LED_DIAGNOSTIC && test_gpio_mode == VVVP_GPIO_LED_LOW,
          "diagnostics owns PA5 only during explicit output test");
    press(); expectEngine(true, "SW1 operates during diagnostic Low override");
    check(test_gpio_mode == VVVP_GPIO_LED_LOW, "engine does not overwrite diagnostic output");
    elapsed(969); check(test_gpio_mode == VVVP_GPIO_LED_LOW, "diagnostic override lasts to 999ms");
    elapsed(1); expectLed(true, "automatic expiry restores latest Engine ON");
    release(); send("3\n"); press(); expectEngine(false, "Stop works during diagnostic High override");
    check(test_gpio_mode == VVVP_GPIO_LED_HIGH && vvvp_gpio_led_owner == VVVP_GPIO_LED_DIAGNOSTIC,
          "High override is preserved while engine Stops");
    send("4\n"); expectLed(false, "manual end restores latest Engine OFF");
    release(); send("3\n"); press(); expectEngine(true, "Start during High override");
    send("0\n"); expectLed(true, "menu exit restores latest state instead of saved OFF state");
    send("7\n2\n"); vvvp_vehicle_input_init(); pump(); expectLed(true, "UI restart restores Engine ownership");

    reset(); validInputs();
    // While a value is partially typed, button processing must not inject UART text.
    send("1\n123"); const std::string prompt_output = terminal;
    press(); pump(); expectEngine(true, "SW1 remains independent of incomplete UART value");
    check(terminal == prompt_output && runtime.driverInput().getBrakePosition() == 10,
          "button does not corrupt prompt or apply incomplete value");
    send("\n"); check(runtime.driverInput().getBrakePosition() == 123, "partial value completes normally");
    send("4\n");
    check(terminal.find("Engine State: ON\r\nEngine RPM: 850\r\nDashboard State: ON") != std::string::npos,
          "UART status reads physical Start result from shared runtime");
    release(); press(); send("4\n");
    check(terminal.find("Engine State: OFF\r\nEngine RPM: 0\r\nDashboard State: OFF") != std::string::npos,
          "UART status reads physical Stop result");
    release(); tx_blocked = true; send("6\n");
    check(tx.size() == 128U, "UART TX queue saturated for independent control test");
    press(); expectEngine(true, "physical control runs while UART output is blocked"); expectLed(true, "LED sync despite UART backpressure");
    tx_blocked = false; pump();
    check(terminal.find("Select Menu:") != std::string::npos, "UART menu resumes after blocked output");
    check(original_input == &runtime.driverInput() && original_state == &runtime.vehicleState() &&
          original_controller == &runtime.engineController(), "UART and SW1 use identical runtime objects");

    // SYS-014/SW-012 and SYS-021/SW-020: real controller -> console -> UART TX.
    const auto verifyFailure = [&](const char* command, bool initially_on, const char* reason) {
        reset(); validInputs();
        if (initially_on) { press(); release(); }
        send(command);
        const float battery = runtime.vehicleState().getBatteryVoltage();
        const bool engine = runtime.vehicleState().getEngineState();
        const int rpm = runtime.vehicleState().getEngineRpm();
        const bool dashboard = runtime.vehicleState().getDashboardState();
        const uint32_t led = test_gpio_mode;
        terminal.clear(); press(); pump();
        check(terminal.find(reason) != std::string::npos &&
              terminal.find("requests=1") != std::string::npos, "rejection reason reaches UART TX");
        check(terminal.find("Failure Reason:") == terminal.rfind("Failure Reason:"),
              "one rejection emits one report");
        check(runtime.vehicleState().getBatteryVoltage() == battery &&
              runtime.vehicleState().getEngineState() == engine &&
              runtime.vehicleState().getEngineRpm() == rpm &&
              runtime.vehicleState().getDashboardState() == dashboard && test_gpio_mode == led,
              "reason reporting preserves entire VehicleState and LED");
        const std::string reported = terminal;
        elapsed(1000); pump(); check(terminal == reported, "holding failed button does not repeat report");
    };
    verifyFailure("BRAKE=0\n", false, "Failure Reason: START_BRAKE");
    verifyFailure("BRAKE=-1\n", false, "Failure Reason: START_BRAKE");
    verifyFailure("GEAR=D\n", false, "Failure Reason: START_GEAR");
    verifyFailure("GEAR=R\n", false, "Failure Reason: START_GEAR");
    verifyFailure("BATTERY=5\n", false, "Failure Reason: START_BATTERY");
    verifyFailure("BATTERY=15.1\n", false, "Failure Reason: START_BATTERY");
    verifyFailure("BRAKE=0\n", true, "Failure Reason: STOP_BRAKE");
    verifyFailure("BRAKE=-1\nGEAR=D\nBATTERY=100\n", true, "Failure Reason: STOP_BRAKE");
    verifyFailure("BRAKE=0\nGEAR=D\nBATTERY=5\n", false, "Failure Reason: START_BATTERY");

    reset(); validInputs(); terminal.clear(); press(); pump();
    check(terminal.empty(), "successful Start emits no Failure Reason");
    release(); press(); pump(); check(terminal.empty(), "successful Stop emits no Failure Reason");

    reset(); validInputs(); send("BRAKE=0\n"); send("2\nN");
    const std::string unfinished = terminal; press(); pump();
    check(terminal == unfinished && runtime.driverInput().getGearPosition() == GearPosition::P,
          "failure notice waits for a partial UART line without changing input");
    send("\r\n");
    check(runtime.driverInput().getGearPosition() == GearPosition::N &&
          terminal.find("Gear updated successfully.") != std::string::npos &&
          terminal.find("Failure Reason: START_BRAKE") != std::string::npos,
          "CRLF completes the original question then delivers queued failure");

    reset(); validInputs(); send("BRAKE=0\n"); terminal.clear(); tx_blocked = true;
    for (unsigned i = 0; i < 6; ++i) { press(); release(); }
    pump(); check(terminal.empty(), "failure output does not block on a full UART queue");
    tx_blocked = false; pump();
    unsigned reported_requests = 0U;
    size_t offset = 0U;
    while ((offset = terminal.find("requests=", offset)) != std::string::npos) {
        unsigned value = 0U;
        std::sscanf(terminal.c_str() + offset, "requests=%u", &value);
        reported_requests += value; offset += 9U;
    }
    check(reported_requests == 6U, "backpressure retains every rejection, batching repeated causes");
    expectEngine(false, "queued failures preserve engine OFF"); expectLed(false, "queued failures preserve LED OFF");
    send("4\n"); check(terminal.find("Engine State: OFF\r\nEngine RPM: 0") != std::string::npos,
                          "CLI remains usable after failure queue drains");

    reset(); validInputs(); send("BRAKE=0\n7\n"); terminal.clear(); press(); pump();
    check(terminal.find("Failure Reason: START_BRAKE") != std::string::npos &&
          terminal.find("Select Diagnostic:") != std::string::npos,
          "failure report restores the diagnostic prompt without leaving its menu");

    reset(); validInputs(); test_gpio_status = VVVP_GPIO_DIAG_CLOCK_UNSUPPORTED;
    vvvp_gpio_diag_init(); vvvp_vehicle_control_init();
    const uint32_t disabled_events = vvvp_sw1_event_count;
    press(); check(vvvp_sw1_event_count == disabled_events && !runtime.vehicleState().getEngineState(),
                   "no uncalibrated debounce when timebase unavailable");
    vvvp_run_engine_control_v2_probe();
    check(vvvp_v2_probe_completed == 1U && vvvp_v2_probe_all_passed == 1U &&
          vvvp_v2_probe_passed_count == 14U, "original 14 V2 control probes still pass");
    std::printf("%u engine-button checks, %u failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
