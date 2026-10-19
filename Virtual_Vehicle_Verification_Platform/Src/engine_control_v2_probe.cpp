#include "engine_control_v2_probe.h"
#include "EngineControllerV2.h"

extern "C" {
volatile VvvpV2ProbeResult vvvp_v2_probe_results[VVVP_V2_PROBE_CASE_COUNT] = {};
volatile uint32_t vvvp_v2_probe_completed = 0U;
volatile uint32_t vvvp_v2_probe_passed_count = 0U;
volatile uint32_t vvvp_v2_probe_all_passed = 0U;
}

namespace {
VvvpV2StateSnapshot read_state(const VehicleState& state)
{
    return {state.getBatteryVoltage(), state.getEngineState() ? 1U : 0U,
            state.getEngineRpm(), state.getDashboardState() ? 1U : 0U};
}

void store_state(volatile VvvpV2StateSnapshot& destination,
                 const VvvpV2StateSnapshot& source)
{
    destination.battery_voltage = source.battery_voltage;
    destination.engine_state = source.engine_state;
    destination.engine_rpm = source.engine_rpm;
    destination.dashboard_state = source.dashboard_state;
}

bool states_equal(const VvvpV2StateSnapshot& left,
                  const VvvpV2StateSnapshot& right)
{
    return left.battery_voltage == right.battery_voltage &&
           left.engine_state == right.engine_state &&
           left.engine_rpm == right.engine_rpm &&
           left.dashboard_state == right.dashboard_state;
}

void run_request(VvvpV2ProbeCase index, DriverInput& input, VehicleState& state,
                 vvvp::v2::EngineController& controller, bool press_button,
                 bool expect_transition)
{
    const VvvpV2StateSnapshot before = read_state(state);
    if (press_button) {
        input.pressEngineButton();
    }
    controller.processEngineButton();
    const VvvpV2StateSnapshot after = read_state(state);

    bool passed;
    if (expect_transition) {
        const uint32_t expected_on = before.engine_state == 0U ? 1U : 0U;
        passed = after.engine_state == expected_on &&
                 after.engine_rpm == (expected_on != 0U ? 850 : 0) &&
                 after.battery_voltage == before.battery_voltage;
        // Dashboard success behavior is inherited, not a new V2.0 LED requirement.
    } else {
        // SW-010/SW-018: compare every field of the existing VehicleState.
        passed = states_equal(before, after);
    }
    passed = passed && !input.isEngineButtonPressed();

    volatile VvvpV2ProbeResult& result = vvvp_v2_probe_results[index];
    result.brake_position = input.getBrakePosition();
    result.gear_position = static_cast<uint32_t>(input.getGearPosition());
    store_state(result.before, before);
    store_state(result.after, after);
    result.engine_button_event_after = input.isEngineButtonPressed() ? 1U : 0U;
    result.passed = passed ? 1U : 0U;
    if (passed) {
        ++vvvp_v2_probe_passed_count;
    }
}

void run_case(VvvpV2ProbeCase index, int brake, GearPosition gear, float battery,
              bool initially_on, bool press_button, bool expect_transition)
{
    DriverInput input;
    VehicleState state;
    vvvp::v2::EngineController controller(input, state);
    input.setBrakePosition(brake);
    input.setGearPosition(gear);
    state.setBatteryVoltage(battery);
    // Test fixtures, not a V2.0 system initialization policy.
    state.setEngineState(initially_on);
    state.setEngineRpm(initially_on ? 850 : 0);
    state.setDashboardState(initially_on);
    run_request(index, input, state, controller, press_button, expect_transition);
    // SW-003: ensure even the out-of-range test voltage was stored unchanged.
    if (vvvp_v2_probe_results[index].before.battery_voltage != battery) {
        if (vvvp_v2_probe_results[index].passed != 0U) {
            --vvvp_v2_probe_passed_count;
        }
        vvvp_v2_probe_results[index].passed = 0U;
    }
}
}

extern "C" void vvvp_run_engine_control_v2_probe(void)
{
    vvvp_v2_probe_completed = 0U;
    vvvp_v2_probe_passed_count = 0U;
    vvvp_v2_probe_all_passed = 0U;

    // Execute normal Start followed by Stop on the same objects.
    DriverInput input;
    VehicleState state;
    vvvp::v2::EngineController controller(input, state);
    input.setBrakePosition(1);
    input.setGearPosition(GearPosition::P);
    state.setBatteryVoltage(12.5f);
    run_request(VVVP_V2_NORMAL_START, input, state, controller, true, true);
    run_request(VVVP_V2_NORMAL_STOP, input, state, controller, true, true);

    run_case(VVVP_V2_START_11_5, 1, GearPosition::P, 11.5f, false, true, true);
    run_case(VVVP_V2_START_15_0, 1, GearPosition::P, 15.0f, false, true, true);
    run_case(VVVP_V2_START_15_1, 1, GearPosition::P, 15.1f, false, true, false);
    run_case(VVVP_V2_START_11_4, 1, GearPosition::P, 11.4f, false, true, false);
    run_case(VVVP_V2_START_BRAKE_ZERO, 0, GearPosition::P, 12.5f, false, true, false);
    run_case(VVVP_V2_START_BRAKE_NEGATIVE, -1, GearPosition::P, 12.5f, false, true, false);
    run_case(VVVP_V2_START_GEAR_R, 1, GearPosition::R, 12.5f, false, true, false);
    run_case(VVVP_V2_START_GEAR_N, 1, GearPosition::N, 12.5f, false, true, true);
    run_case(VVVP_V2_STOP_HIGH_BATTERY_GEAR_D, 1, GearPosition::D, 15.1f, true, true, true);
    run_case(VVVP_V2_STOP_LOW_BATTERY_GEAR_R, 1, GearPosition::R, 10.0f, true, true, true);
    run_case(VVVP_V2_STOP_BRAKE_ZERO, 0, GearPosition::P, 12.5f, true, true, false);
    run_case(VVVP_V2_NO_BUTTON, 1, GearPosition::P, 12.5f, false, false, false);

    vvvp_v2_probe_all_passed =
        vvvp_v2_probe_passed_count == VVVP_V2_PROBE_CASE_COUNT ? 1U : 0U;
    vvvp_v2_probe_completed = 1U;
}
