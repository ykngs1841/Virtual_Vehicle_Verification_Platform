#ifndef VVVP_CLASS_BEHAVIOR_PROBE_H
#define VVVP_CLASS_BEHAVIOR_PROBE_H

#include <stdint.h>

typedef struct {
    int32_t brake_position;
    uint32_t gear_position;
    float battery_voltage;
    uint32_t engine_state;
    int32_t engine_rpm;
    uint32_t dashboard_state;
    uint32_t engine_button_event;
} VvvpClassProbeSnapshot;

#ifdef __cplusplus
extern "C" {
#endif

extern volatile VvvpClassProbeSnapshot vvvp_class_probe_initial;
extern volatile VvvpClassProbeSnapshot vvvp_class_probe_start;
extern volatile VvvpClassProbeSnapshot vvvp_class_probe_stop;

/* 1 means all snapshots were captured; inspect their values for correctness. */
extern volatile uint32_t vvvp_class_probe_completed;

void vvvp_run_class_behavior_probe(void);

#ifdef __cplusplus
}
#endif

#endif
