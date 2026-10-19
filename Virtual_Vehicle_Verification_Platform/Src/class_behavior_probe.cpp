#include "class_behavior_probe.h"
#include "DriverInput.h"
#include "VehicleState.h"
#include "EngineController.h"

extern "C" {
volatile VvvpClassProbeSnapshot vvvp_class_probe_initial = {};
volatile VvvpClassProbeSnapshot vvvp_class_probe_start = {};
volatile VvvpClassProbeSnapshot vvvp_class_probe_stop = {};
volatile uint32_t vvvp_class_probe_completed = 0U;
}

namespace {
void capture_snapshot(volatile VvvpClassProbeSnapshot& snapshot,
                      const DriverInput& driver_input,
                      const VehicleState& vehicle_state)
{
    snapshot.brake_position = driver_input.getBrakePosition();
    snapshot.gear_position = static_cast<uint32_t>(driver_input.getGearPosition());
    snapshot.battery_voltage = vehicle_state.getBatteryVoltage();
    snapshot.engine_state = vehicle_state.getEngineState() ? 1U : 0U;
    snapshot.engine_rpm = vehicle_state.getEngineRpm();
    snapshot.dashboard_state = vehicle_state.getDashboardState() ? 1U : 0U;
    snapshot.engine_button_event = driver_input.isEngineButtonPressed() ? 1U : 0U;
}
}

extern "C" void vvvp_run_class_behavior_probe(void)
{
    vvvp_class_probe_completed = 0U;

    // Referenced state objects outlive the controller for the entire scenario.
    DriverInput driver_input;
    VehicleState vehicle_state;
    EngineController engine_controller(driver_input, vehicle_state);

    capture_snapshot(vvvp_class_probe_initial, driver_input, vehicle_state);

    driver_input.setBrakePosition(1);
    driver_input.setGearPosition(GearPosition::P);
    vehicle_state.setBatteryVoltage(12.5f);
    driver_input.pressEngineButton();
    engine_controller.processEngineButton();
    capture_snapshot(vvvp_class_probe_start, driver_input, vehicle_state);

    driver_input.setBrakePosition(1);
    driver_input.pressEngineButton();
    engine_controller.processEngineButton();
    capture_snapshot(vvvp_class_probe_stop, driver_input, vehicle_state);

    vvvp_class_probe_completed = 1U;
}
