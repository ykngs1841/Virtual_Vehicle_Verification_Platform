#include "EngineButton.h"

namespace vvvp {
namespace v2 {

EngineButton::EngineButton()
    : initialized_(false), stable_pressed_(false), candidate_pressed_(false),
      armed_(false), candidate_since_ms_(0U)
{
}

void EngineButton::reset(bool raw_high, uint32_t now_ms)
{
    initialized_ = true;
    stable_pressed_ = !raw_high;
    candidate_pressed_ = stable_pressed_;
    candidate_since_ms_ = now_ms;
    // A button held during initialization must first be stably released.
    armed_ = raw_high;
}

bool EngineButton::sample(bool raw_high, uint32_t now_ms)
{
    if (!initialized_) { reset(raw_high, now_ms); return false; }
    const bool pressed_now = !raw_high;
    if (pressed_now != candidate_pressed_) {
        candidate_pressed_ = pressed_now;
        candidate_since_ms_ = now_ms;
    }
    if (candidate_pressed_ == stable_pressed_ ||
        static_cast<uint32_t>(now_ms - candidate_since_ms_) < DebounceMs) {
        return false;
    }
    stable_pressed_ = candidate_pressed_;
    if (!stable_pressed_) { armed_ = true; return false; }
    const bool emit = armed_;
    armed_ = false;
    return emit;
}

}
}
