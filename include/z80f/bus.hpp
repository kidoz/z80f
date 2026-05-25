#pragma once

#include <cstdint>

namespace z80f {

class Bus {
public:
    virtual ~Bus() = default;

    virtual std::uint8_t read_memory(std::uint16_t address) = 0;
    virtual void write_memory(std::uint16_t address, std::uint8_t value) = 0;

    virtual std::uint8_t read_io(std::uint16_t port) = 0;
    virtual void write_io(std::uint16_t port, std::uint8_t value) = 0;

    // Called once per Z80 machine cycle. T-state granularity gives hosts the
    // hook they need to layer wait states / contention (e.g. ZX Spectrum ULA).
    virtual int on_m_cycle(std::uint16_t address, int t_states) = 0;

    // Optional override: data bus byte for IM 0 / IM 2 interrupt acknowledge.
    // Default: 0xFF. Note: This default works perfectly for IM 1 (which effectively
    // uses 0xFF as a RST 38 vector). However, for IM 0, 0xFF is executed as a RST 38
    // instruction. Hosts that expect data-bus vectoring for IM 0 MUST override this
    // to supply the correct instruction.
    virtual std::uint8_t acknowledge_interrupt() { return 0xFF; }
};

}  // namespace z80f
