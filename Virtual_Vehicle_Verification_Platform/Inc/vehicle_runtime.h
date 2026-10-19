#ifndef VVVP_VEHICLE_RUNTIME_H
#define VVVP_VEHICLE_RUNTIME_H

#include <stdint.h>

#ifdef __cplusplus
#include "EngineControllerV2.h"

namespace vvvp {
namespace v2 {
// The UART and future button/control path must use this same runtime instance.
class VehicleRuntime {
public:
    VehicleRuntime();
    VehicleRuntime(const VehicleRuntime&) = delete;
    VehicleRuntime& operator=(const VehicleRuntime&) = delete;
    DriverInput& driverInput() { return driver_input_; }
    VehicleState& vehicleState() { return vehicle_state_; }
    EngineController& engineController() { return engine_controller_; }
private:
    DriverInput driver_input_;
    VehicleState vehicle_state_;
    EngineController engine_controller_;
};
VehicleRuntime& vehicleRuntime();
}
}
extern "C" {
#endif

// Observation copies only; control must use vehicleRuntime(), never this copy.
typedef struct {
    int32_t brake_position;
    uint32_t gear_position; // V1.0 enum: P=0, R=1, N=2, D=3.
    float battery_voltage;
    uint32_t engine_state;
    int32_t engine_rpm;
    uint32_t dashboard_state;
    uint32_t engine_button_event;
} VvvpRuntimeSnapshot;

extern volatile VvvpRuntimeSnapshot vvvp_runtime_state;
extern volatile uint32_t vvvp_command_ok_count;
extern volatile uint32_t vvvp_command_err_count;
extern volatile uint32_t vvvp_sw1_event_count;
extern volatile uint32_t vvvp_sw1_debounced_pressed;
extern volatile uint32_t vvvp_sw1_armed;

void vvvp_vehicle_input_init(void);
void vvvp_vehicle_input_poll(void);
void vvvp_vehicle_control_init(void);
void vvvp_vehicle_control_poll(void);

#ifdef __cplusplus
}
#endif
#endif
