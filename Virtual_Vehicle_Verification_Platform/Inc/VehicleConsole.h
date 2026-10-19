#ifndef VVVP_VEHICLE_CONSOLE_H
#define VVVP_VEHICLE_CONSOLE_H

#include "UartVehicleCommand.h"
#include "EngineControllerV2.h"

namespace vvvp {
namespace v2 {

// ASCII UI only. No UART hardware dependency and no engine control requests.
class VehicleConsole {
public:
    enum State { Menu, Brake, Gear, Battery, ConfigureBrake, ConfigureGear,
                 ConfigureBattery, GpioMenu };
    VehicleConsole(DriverInput& input, VehicleState& vehicle);
    void begin();
    UartVehicleCommand::Result receive(uint8_t byte);
    void invalidateLine() { parser_.invalidateLine(); }
    const char* outputData() const { return output_ + output_position_; }
    uint32_t outputSize() const { return output_length_ - output_position_; }
    void consumeOutput(uint32_t count);
    void queueFailure(EngineController::Failure failure);
    void pollFailureOutput();
    State state() const { return state_; }

private:
    void append(const char* text);
    void showMenu();
    void showQuestion();
    void showStatus();
    void showHelp();
    void showGpioMenu();
    void handleGpioLine(bool valid, const char* text);
    void outputError();
    DriverInput& input_;
    VehicleState& vehicle_;
    UartVehicleCommand parser_;
    State state_;
    char output_[1024];
    uint32_t output_length_;
    uint32_t output_position_;
    int32_t previous_sw1_;
    uint32_t pending_failures_[EngineController::FailureCount];
};

}
}
#endif
