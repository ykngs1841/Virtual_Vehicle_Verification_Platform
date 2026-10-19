#ifndef VVVP_ENGINE_CONTROL_V2_PROBE_H
#define VVVP_ENGINE_CONTROL_V2_PROBE_H

#include <stdint.h>

typedef enum {
    VVVP_V2_NORMAL_START = 0,
    VVVP_V2_NORMAL_STOP,
    VVVP_V2_START_11_5,
    VVVP_V2_START_15_0,
    VVVP_V2_START_15_1,
    VVVP_V2_START_11_4,
    VVVP_V2_START_BRAKE_ZERO,
    VVVP_V2_START_BRAKE_NEGATIVE,
    VVVP_V2_START_GEAR_R,
    VVVP_V2_START_GEAR_N,
    VVVP_V2_STOP_HIGH_BATTERY_GEAR_D,
    VVVP_V2_STOP_LOW_BATTERY_GEAR_R,
    VVVP_V2_STOP_BRAKE_ZERO,
    VVVP_V2_NO_BUTTON,
    VVVP_V2_PROBE_CASE_COUNT
} VvvpV2ProbeCase;

typedef struct {
    float battery_voltage;
    uint32_t engine_state;
    int32_t engine_rpm;
    uint32_t dashboard_state;
} VvvpV2StateSnapshot;

typedef struct {
    int32_t brake_position;
    uint32_t gear_position;
    VvvpV2StateSnapshot before;
    VvvpV2StateSnapshot after;
    uint32_t engine_button_event_after;
    uint32_t passed;
} VvvpV2ProbeResult;

#ifdef __cplusplus
extern "C" {
#endif

extern volatile VvvpV2ProbeResult vvvp_v2_probe_results[VVVP_V2_PROBE_CASE_COUNT];
extern volatile uint32_t vvvp_v2_probe_completed;
extern volatile uint32_t vvvp_v2_probe_passed_count;
extern volatile uint32_t vvvp_v2_probe_all_passed;

void vvvp_run_engine_control_v2_probe(void);

#ifdef __cplusplus
}
#endif

#endif
