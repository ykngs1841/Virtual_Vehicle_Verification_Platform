#ifndef VVVP_ENGINE_CONTROLLER_V2_H
#define VVVP_ENGINE_CONTROLLER_V2_H

#include "EngineController.h"

namespace vvvp {
namespace v2 {

// Reuse V1.0 control behavior without changing the linked original class.
class EngineController {
public:
    // One cause per rejected request; voltage rejection precedes V1.0 checks.
    enum Failure { None, StartBattery, StartBrake, StartGear, StopBrake, FailureCount };
    EngineController(DriverInput& driver_input, VehicleState& vehicle_state);
    Failure processEngineButton();

private:
    DriverInput& driver_input_;
    VehicleState& vehicle_state_;
    ::EngineController reused_controller_;
};

}
}

#endif
