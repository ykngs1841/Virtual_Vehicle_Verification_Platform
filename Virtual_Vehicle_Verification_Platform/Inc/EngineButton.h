#ifndef VVVP_ENGINE_BUTTON_H
#define VVVP_ENGINE_BUTTON_H
#include <stdint.h>

namespace vvvp {
namespace v2 {

// Confirmed SW1 Active Low. Timing is supplied by the caller, with no GPIO/UART.
class EngineButton {
public:
    enum { DebounceMs = 30 };
    EngineButton();
    void reset(bool raw_high, uint32_t now_ms);
    bool sample(bool raw_high, uint32_t now_ms);
    bool pressed() const { return stable_pressed_; }
    bool armed() const { return armed_; }
private:
    bool initialized_;
    bool stable_pressed_;
    bool candidate_pressed_;
    bool armed_;
    uint32_t candidate_since_ms_;
};

}
}
#endif
