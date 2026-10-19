#ifndef VVVP_UART_VEHICLE_COMMAND_H
#define VVVP_UART_VEHICLE_COMMAND_H

#include <stdint.h>
#include "DriverInput.h"
#include "VehicleState.h"

namespace vvvp {
namespace v2 {

// Trial text commands only; no Engine Button or control operation is performed.
class UartVehicleCommand {
public:
    enum Result { Incomplete, Accepted, Rejected };
    enum LineResult { LineIncomplete, LineReady, LineInvalid };
    enum { MaxLineLength = 63 };

    UartVehicleCommand(DriverInput& driver_input, VehicleState& vehicle_state);
    Result receive(uint8_t byte);
    void invalidateLine();
    void reset();
    LineResult receiveLine(uint8_t byte);
    const char* line() const { return line_; }
    bool lineInProgress() const { return length_ != 0U || invalid_; }
    bool applyCurrentLine();
    bool setBrakeText(const char* text);
    bool setGearText(const char* text);
    bool setBatteryText(const char* text);

private:
    DriverInput& driver_input_;
    VehicleState& vehicle_state_;
    char line_[MaxLineLength + 1];
    uint32_t length_;
    bool invalid_;
    bool after_cr_;
};

}
}
#endif
