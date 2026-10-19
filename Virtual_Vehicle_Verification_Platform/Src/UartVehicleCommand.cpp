#include "UartVehicleCommand.h"
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {
bool digit(char c)
{
    return c >= '0' && c <= '9';
}

bool integerText(const char* p)
{
    if (*p == '+' || *p == '-') { ++p; }
    if (!digit(*p)) { return false; }
    while (digit(*p)) { ++p; }
    return *p == '\0';
}

// Trial grammar: signed decimal, optional decimal point and decimal exponent.
// Spaces, hex floats, NaN and Inf are deliberately not part of this grammar.
bool decimalText(const char* p)
{
    if (*p == '+' || *p == '-') { ++p; }
    bool has_digits = false;
    while (digit(*p)) { has_digits = true; ++p; }
    if (*p == '.') {
        ++p;
        while (digit(*p)) { has_digits = true; ++p; }
    }
    if (!has_digits) { return false; }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') { ++p; }
        if (!digit(*p)) { return false; }
        while (digit(*p)) { ++p; }
    }
    return *p == '\0';
}
}

namespace vvvp {
namespace v2 {

UartVehicleCommand::UartVehicleCommand(DriverInput& driver_input,
                                     VehicleState& vehicle_state)
    : driver_input_(driver_input), vehicle_state_(vehicle_state),
      line_{}, length_(0U), invalid_(false), after_cr_(false)
{
}

void UartVehicleCommand::invalidateLine()
{
    invalid_ = true;
    after_cr_ = false;
}

void UartVehicleCommand::reset()
{
    length_ = 0U;
    invalid_ = false;
    after_cr_ = false;
    line_[0] = '\0';
}

UartVehicleCommand::Result UartVehicleCommand::receive(uint8_t byte)
{
    const LineResult result = receiveLine(byte);
    if (result == LineIncomplete) { return Incomplete; }
    return result == LineReady && applyCurrentLine() ? Accepted : Rejected;
}

UartVehicleCommand::LineResult UartVehicleCommand::receiveLine(uint8_t byte)
{
    if (after_cr_ && byte == '\n') {
        after_cr_ = false;
        return LineIncomplete; // CRLF is exactly one terminator, including ERR lines.
    }
    after_cr_ = false;
    if (byte == '\r' || byte == '\n') {
        line_[length_] = '\0';
        const bool valid = !invalid_;
        length_ = 0U;
        invalid_ = false;
        after_cr_ = byte == '\r';
        return valid ? LineReady : LineInvalid;
    }
    if (byte < 0x20U || byte > 0x7eU || length_ == MaxLineLength) {
        invalid_ = true;
    }
    // Once invalid, discard the entire remainder, never interpret a suffix.
    if (!invalid_) { line_[length_++] = static_cast<char>(byte); }
    return LineIncomplete;
}

bool UartVehicleCommand::applyCurrentLine()
{
    if (std::strncmp(line_, "BRAKE=", 6U) == 0) {
        return setBrakeText(line_ + 6U);
    }
    if (std::strncmp(line_, "GEAR=", 5U) == 0) {
        return setGearText(line_ + 5U);
    }
    if (std::strncmp(line_, "BATTERY=", 8U) == 0) {
        return setBatteryText(line_ + 8U);
    }
    return false;
}

bool UartVehicleCommand::setBrakeText(const char* text)
{
    if (!integerText(text)) { return false; }
    char* end = 0;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || value < INT_MIN || value > INT_MAX) {
        return false;
    }
    driver_input_.setBrakePosition(static_cast<int>(value));
    return true;
}

bool UartVehicleCommand::setGearText(const char* text)
{
    if (std::strlen(text) != 1U) { return false; }
    GearPosition gear;
    switch (text[0]) {
    case 'P': gear = GearPosition::P; break;
    case 'N': gear = GearPosition::N; break;
    case 'R': gear = GearPosition::R; break;
    case 'D': gear = GearPosition::D; break;
    default: return false;
    }
    driver_input_.setGearPosition(gear);
    return true;
}

bool UartVehicleCommand::setBatteryText(const char* text)
{
    if (!decimalText(text)) { return false; }
    char* end = 0;
    errno = 0;
    const float value = std::strtof(text, &end);
    if (errno == ERANGE || *end != '\0' || !std::isfinite(value)) {
        return false;
    }
    // SW-003: storage is independent of the EngineControllerV2 Start limits.
    vehicle_state_.setBatteryVoltage(value);
    return true;
}

}
}
