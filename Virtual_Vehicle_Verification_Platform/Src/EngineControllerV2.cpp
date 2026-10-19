#include "EngineControllerV2.h"

namespace vvvp {
namespace v2 {

EngineController::EngineController(DriverInput& driver_input,
                                   VehicleState& vehicle_state)
    : driver_input_(driver_input),
      vehicle_state_(vehicle_state),
      reused_controller_(driver_input, vehicle_state)
{
}

EngineController::Failure EngineController::processEngineButton()
{
    if (!driver_input_.isEngineButtonPressed()) {
        return None;
    }

    // SW-005/SW-006: inspect the stored voltage only for an OFF -> Start request.
    // SW-003: do not clamp or rewrite Battery Voltage during control processing.
    const bool was_on = vehicle_state_.getEngineState();
    if (!was_on) {
        const float voltage = vehicle_state_.getBatteryVoltage();
        if (!(voltage >= 11.5f && voltage <= 15.0f)) {
            // SW-010: no VehicleState field changes on rejection.
            driver_input_.clearEngineButtonEvent();
            return StartBattery;
        }
    }

    // Reuse Brake/Gear checks, state/RPM transitions and event consumption.
    // SW-013/SW-014: the ON -> Stop path has no voltage or Gear precondition.
    // SW-007/SW-015 and SW-010/SW-018 use the original transition/retention logic.
    // Dashboard State follows existing V1.0 behavior; no physical LED is driven.
    reused_controller_.processEngineButton();
    if (vehicle_state_.getEngineState() != was_on) { return None; }
    // Classify the actual V1.0 rejection, without deciding or executing control
    // a second time. With voltage accepted, its remaining Start checks are
    // Brake and Gear; Stop rejection can only be Brake.
    if (was_on) { return StopBrake; }
    return driver_input_.getBrakePosition() <= 0 ? StartBrake : StartGear;
}

}
}
