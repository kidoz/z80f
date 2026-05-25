#pragma once

#include <z80f/registers.hpp>

#include <cstdint>

namespace z80f {

// Plain data describing the entire CPU state at one instant.
// Hosts are responsible for saving and restoring their bus / memory state.
struct Snapshot {
    Registers registers{};
    std::uint64_t cycle_counter = 0;
    bool nmi_line = false;
    bool nmi_pending = false;
    bool int_line = false;
    bool int_pulse_pending = false;
};

}  // namespace z80f
