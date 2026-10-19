#include "VehicleConsole.h"
#include <cstdio>
#include <cstring>
#include "gpio_diagnostics.h"

namespace vvvp {
namespace v2 {

VehicleConsole::VehicleConsole(DriverInput& input, VehicleState& vehicle)
    : input_(input), vehicle_(vehicle), parser_(input, vehicle), state_(Menu),
      output_{}, output_length_(0U), output_position_(0U), previous_sw1_(-1),
      pending_failures_{}
{
}

void VehicleConsole::queueFailure(EngineController::Failure failure)
{
    if (failure > EngineController::None && failure < EngineController::FailureCount) {
        ++pending_failures_[failure];
    }
}

void VehicleConsole::pollFailureOutput()
{
    // Keep queued output intact and never insert a notice into an MCU input line.
    if (outputSize() != 0U || parser_.lineInProgress()) { return; }
    static const char* const reasons[] = {
        "", "START_BATTERY: Battery Voltage must be 11.5..15.0 V",
        "START_BRAKE: Brake Position must be > 0",
        "START_GEAR: Gear Position must be P or N",
        "STOP_BRAKE: Brake Position must be > 0"
    };
    for (unsigned i = 1U; i < EngineController::FailureCount; ++i) {
        if (pending_failures_[i] == 0U) { continue; }
        char report[160];
        std::snprintf(report, sizeof(report), "\r\nFailure Reason: %s (requests=%lu)\r\n",
                      reasons[i], static_cast<unsigned long>(pending_failures_[i]));
        append(report);
        pending_failures_[i] = 0U;
        if (state_ == Menu) { append("Select Menu: "); }
        else if (state_ == GpioMenu) { append("Select Diagnostic: "); }
        else { showQuestion(); }
        return; // Bounded work; remaining reasons are retained for a later poll.
    }
}

void VehicleConsole::outputError()
{
    vvvp_gpio_diag_end_led_test();
    const char fallback[] = "\r\nConsole output error.\r\nSelect Menu: ";
    std::memcpy(output_, fallback, sizeof(fallback));
    output_length_ = sizeof(fallback) - 1U;
    output_position_ = 0U;
    state_ = Menu;
}

void VehicleConsole::append(const char* text)
{
    const size_t length = std::strlen(text);
    if (length >= sizeof(output_) - output_length_) {
        outputError();
        return;
    }
    std::memcpy(output_ + output_length_, text, length + 1U);
    output_length_ += static_cast<uint32_t>(length);
}

void VehicleConsole::consumeOutput(uint32_t count)
{
    if (count <= outputSize()) { output_position_ += count; }
    if (output_position_ == output_length_) {
        output_position_ = 0U;
        output_length_ = 0U;
        output_[0] = '\0';
    }
}

void VehicleConsole::showMenu()
{
    append("\r\n=== VVVP Vehicle Control System ===\r\n"
           "1. Set Brake Position\r\n"
           "2. Set Gear Position\r\n"
           "3. Set Battery Voltage\r\n"
           "4. Show Vehicle Status\r\n"
           "5. Configure All\r\n"
           "6. Help\r\n"
           "7. GPIO Diagnostics\r\n\r\nSelect Menu: ");
}

void VehicleConsole::begin()
{
    parser_.reset();
    vvvp_gpio_diag_end_led_test();
    previous_sw1_ = -1;
    state_ = Menu;
    output_length_ = 0U;
    output_position_ = 0U;
    showMenu();
}

void VehicleConsole::showQuestion()
{
    switch (state_) {
    case Brake: case ConfigureBrake: append("Enter Brake Position: "); break;
    case Gear: case ConfigureGear: append("Enter Gear (P/R/N/D): "); break;
    case Battery: case ConfigureBattery: append("Enter Battery Voltage (V): "); break;
    case Menu: case GpioMenu: break;
    }
}

void VehicleConsole::showStatus()
{
    char gear = '?';
    switch (input_.getGearPosition()) {
    case GearPosition::P: gear = 'P'; break;
    case GearPosition::R: gear = 'R'; break;
    case GearPosition::N: gear = 'N'; break;
    case GearPosition::D: gear = 'D'; break;
    }
    char status[256];
    // Nine significant digits round-trip a stored float, including large values.
    // CubeIDE's newlib-nano float printf support is explicitly enabled.
    const int length = std::snprintf(status, sizeof(status),
        "Brake Position: %d\r\nGear Position: %c\r\n"
        "Battery Voltage: %.9g V\r\nEngine State: %s\r\n"
        "Engine RPM: %d\r\nDashboard State: %s\r\n",
        input_.getBrakePosition(), gear,
        static_cast<double>(vehicle_.getBatteryVoltage()),
        vehicle_.getEngineState() ? "ON" : "OFF", vehicle_.getEngineRpm(),
        vehicle_.getDashboardState() ? "ON" : "OFF");
    if (length < 0 || static_cast<size_t>(length) >= sizeof(status)) {
        outputError();
        return;
    }
    append(status);
}

void VehicleConsole::showHelp()
{
    append("1: Set Brake Position (signed 32-bit decimal integer).\r\n"
           "2: Set Gear Position (P/R/N/D, uppercase).\r\n"
           "3: Set Battery Voltage (finite decimal float; exponent allowed).\r\n"
           "   Battery storage is not limited to 0..15 V.\r\n"
           "4: Show the current vehicle state.\r\n"
           "5: Configure Brake, Gear, then Battery.\r\n"
           "6: Show this help.\r\n"
           "7: GPIO tests; SW1 Active Low, LED1 Active High.\r\n"
           "SW1: press/release for Start/Stop. LED1 shows Engine State.\r\n"
           "At Select Menu: BRAKE=, GEAR=, BATTERY= commands are supported.\r\n"
           "Use CR, LF or CRLF. Maximum line length: 63 ASCII characters.\r\n"
           "No spaces, NaN/Inf or numeric range errors. No Start/Stop command.\r\n"
           "PuTTY: Local echo and Local line editing = Force on.\r\n");
}

void VehicleConsole::showGpioMenu()
{
    append("\r\n=== GPIO Diagnostics ===\r\n"
           "Verified: SW1 D2/PA10 Active Low; LED1 D13/PA5 Active High.\r\n"
           "1. Read SW1 raw level (0=pressed, 1=released)\r\n"
           "2. Drive PA5 Low for 1 second\r\n"
           "3. Drive PA5 High for 1 second\r\n"
           "4. End LED test (restore Engine State)\r\n"
           "0. Return to Main Menu (restore Engine State)\r\n"
           "Tests temporarily override LED1; SW1 engine control stays active.\r\n"
           "Select Diagnostic: ");
}

void VehicleConsole::handleGpioLine(bool valid, const char* text)
{
    if (!valid || std::strlen(text) != 1U || text[0] < '0' || text[0] > '4') {
        append("ERR\r\nInvalid diagnostic selection.\r\n");
        showGpioMenu();
        return;
    }
    char report[256];
    switch (text[0]) {
    case '0':
        vvvp_gpio_diag_end_led_test();
        state_ = Menu;
        append("LED test ended. LED1 follows the latest Engine State.\r\n");
        showMenu();
        return;
    case '1': {
        const int32_t raw = vvvp_gpio_diag_read_sw1();
        if (raw < 0) { append("GPIO diagnostics are not initialized.\r\n"); break; }
        if (previous_sw1_ < 0) {
            std::snprintf(report, sizeof(report), "SW1 PA10 raw: %d. First sample.\r\n",
                          static_cast<int>(raw));
        } else {
            std::snprintf(report, sizeof(report),
                          "SW1 PA10: previous=%d current=%d (%s).\r\n",
                          static_cast<int>(previous_sw1_), static_cast<int>(raw),
                          previous_sw1_ == raw ? "UNCHANGED" : "CHANGED");
        }
        append(report);
        append("If pressing/releasing does not change this value, check the pin/wiring.\r\n");
        previous_sw1_ = raw;
        break;
    }
    case '2': case '3': {
        const uint32_t high = text[0] == '3' ? 1U : 0U;
        if (vvvp_gpio_diag_test_led(high) == 0U) {
            append("LED test unavailable: inspect vvvp_gpio_diag_status. PA5 is not newly driven.\r\n");
        } else {
            append(high != 0U ? "PA5 drive: HIGH.\r\n" : "PA5 drive: LOW.\r\n");
            append("Latest Engine State display resumes after 1000 ms of MCU execution.\r\n");
        }
        break;
    }
    case '4':
        vvvp_gpio_diag_end_led_test();
        append("LED test ended. LED1 follows the latest Engine State.\r\n");
        break;
    }
    showGpioMenu();
}

UartVehicleCommand::Result VehicleConsole::receive(uint8_t byte)
{
    // The caller must finish enqueueing the previous output before reading more.
    const UartVehicleCommand::LineResult line = parser_.receiveLine(byte);
    if (line == UartVehicleCommand::LineIncomplete) { return UartVehicleCommand::Incomplete; }
    append("\r\n");
    const char* text = parser_.line();
    if (state_ == GpioMenu) {
        handleGpioLine(line == UartVehicleCommand::LineReady, text);
        return UartVehicleCommand::Incomplete;
    }
    if (state_ == Menu) {
        if (line == UartVehicleCommand::LineReady && std::strchr(text, '=') != 0) {
            const bool accepted = parser_.applyCurrentLine();
            append(accepted ? "OK\r\n" : "ERR\r\n");
            showMenu();
            return accepted ? UartVehicleCommand::Accepted : UartVehicleCommand::Rejected;
        }
        if (line != UartVehicleCommand::LineReady || std::strlen(text) != 1U ||
            text[0] < '1' || text[0] > '7') {
            append("ERR\r\nInvalid menu selection. Choose 1-7.\r\n");
            showMenu();
            return UartVehicleCommand::Rejected;
        }
        switch (text[0]) {
        case '1': state_ = Brake; break;
        case '2': state_ = Gear; break;
        case '3': state_ = Battery; break;
        case '4': showStatus(); showMenu(); return UartVehicleCommand::Incomplete;
        case '5': state_ = ConfigureBrake; break;
        case '6': showHelp(); showMenu(); return UartVehicleCommand::Incomplete;
        case '7': state_ = GpioMenu; previous_sw1_ = -1; showGpioMenu(); return UartVehicleCommand::Incomplete;
        }
        showQuestion();
        return UartVehicleCommand::Incomplete;
    }
    bool accepted = false;
    if (line == UartVehicleCommand::LineReady) {
        switch (state_) {
        case Brake: case ConfigureBrake: accepted = parser_.setBrakeText(text); break;
        case Gear: case ConfigureGear: accepted = parser_.setGearText(text); break;
        case Battery: case ConfigureBattery: accepted = parser_.setBatteryText(text); break;
        case Menu: case GpioMenu: break;
        }
    }
    if (!accepted) {
        append("ERR\r\nInvalid value. Please try again.\r\n");
        showQuestion();
        return UartVehicleCommand::Rejected;
    }
    switch (state_) {
    case Brake: case ConfigureBrake: append("Brake updated successfully.\r\n"); break;
    case Gear: case ConfigureGear: append("Gear updated successfully.\r\n"); break;
    case Battery: case ConfigureBattery: append("Battery updated successfully.\r\n"); break;
    case Menu: case GpioMenu: break;
    }
    if (state_ == ConfigureBrake) { state_ = ConfigureGear; showQuestion(); }
    else if (state_ == ConfigureGear) { state_ = ConfigureBattery; showQuestion(); }
    else {
        if (state_ == ConfigureBattery) { append("Configuration Complete.\r\n"); showStatus(); }
        state_ = Menu;
        showMenu();
    }
    return UartVehicleCommand::Accepted;
}

}
}
