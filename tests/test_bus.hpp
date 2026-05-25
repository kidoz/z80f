#pragma once

#include <z80f/bus.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace z80f {
struct detail_z80_access {
    static Registers& mutable_registers(Z80& cpu) noexcept {
        return cpu.unsafe_registers_mutable();
    }
};
}  // namespace z80f

namespace z80f::test {

struct IoRecord {
    std::uint16_t port;
    std::uint8_t value;
    bool is_read;
};

class TestBus final : public Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};
    std::vector<IoRecord> io_log;
    std::uint8_t io_in_value = 0xFF;
    int m_cycles = 0;

    std::uint8_t read_memory(std::uint16_t address) override { return memory[address]; }
    void write_memory(std::uint16_t address, std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint8_t read_io(std::uint16_t port) override {
        io_log.push_back({port, io_in_value, true});
        return io_in_value;
    }
    void write_io(std::uint16_t port, std::uint8_t value) override {
        io_log.push_back({port, value, false});
    }
    int on_m_cycle(std::uint16_t, int) override {
        ++m_cycles;
        return 0;
    }
};

}  // namespace z80f::test
