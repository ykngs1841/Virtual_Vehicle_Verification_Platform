#include "cpp_link_probe.h"

namespace {
enum class ProbeValue : uint32_t {
    Expected = 42U
};

constexpr uint32_t expected_value = static_cast<uint32_t>(ProbeValue::Expected);
static_assert(expected_value == 42U, "C++11 compile probe failed");
}

extern "C" uint32_t vvvp_cpp_link_probe(void)
{
    return expected_value;
}
