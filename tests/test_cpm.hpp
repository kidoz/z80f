#pragma once

#include <z80f/bus.hpp>
#include <z80f/snapshot.hpp>
#include <z80f/z80.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace z80f::test {

class CpmBus final : public Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};

    std::uint8_t read_memory(std::uint16_t address) override { return memory[address]; }
    void write_memory(std::uint16_t address, std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint8_t read_io(std::uint16_t /*port*/) override { return 0xFF; }
    void write_io(std::uint16_t /*port*/, std::uint8_t /*value*/) override {}
    int on_m_cycle(std::uint16_t /*address*/, int /*t_states*/) override { return 0; }
};

struct CpmResult {
    bool loaded = false;
    bool timed_out = false;
    std::string output;
};

// Runs a CP/M .com image loaded at 0x0100 with a minimal BDOS console trap
// (function 2: char in E, function 9: '$'-terminated string at DE) until the
// program warm-boots through 0x0000 or max_cycles elapse.
inline CpmResult run_cpm_program(const std::string& path, std::uint64_t max_cycles) {
    CpmResult result;

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return result;
    }
    const std::streamsize size = file.tellg();
    if (size <= 0 || size > (0x10000 - 0x0100)) {
        return result;
    }
    CpmBus bus;
    file.seekg(0, std::ios::beg);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file.read(reinterpret_cast<char*>(bus.memory.data() + 0x0100), size);
    if (file.gcount() != size) {
        return result;
    }
    result.loaded = true;

    Z80 cpu(bus);
    cpu.reset();
    Snapshot snapshot = cpu.save_snapshot();
    snapshot.registers.pc = 0x0100;
    snapshot.registers.sp = 0xF000;
    cpu.load_snapshot(snapshot);

    bus.memory[0x0005] = 0xC9;  // RET from the BDOS entry point

    while (true) {
        const auto& r = cpu.registers();
        if (r.pc == 0x0005) {
            if (r.c == 2) {
                result.output += static_cast<char>(r.e);
                std::cout << static_cast<char>(r.e) << std::flush;
            } else if (r.c == 9) {
                std::uint16_t addr = r.de();
                while (bus.memory[addr] != '$') {
                    result.output += static_cast<char>(bus.memory[addr]);
                    std::cout << static_cast<char>(bus.memory[addr]) << std::flush;
                    addr++;
                }
            }
        } else if (r.pc == 0x0000) {
            break;
        }
        cpu.step();
        if (cpu.cycle_counter() > max_cycles) {
            result.timed_out = true;
            break;
        }
    }
    return result;
}

}  // namespace z80f::test
