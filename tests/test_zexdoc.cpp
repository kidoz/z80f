#include <catch2/catch_test_macros.hpp>
#include <z80f/bus.hpp>
#include <z80f/snapshot.hpp>
#include <z80f/z80.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

using z80f::Z80;

namespace {
class ZexdocBus final : public z80f::Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};
    std::string output;
    bool finished = false;

    std::uint8_t read_memory(std::uint16_t address) override { return memory[address]; }

    void write_memory(std::uint16_t address, std::uint8_t value) override {
        memory[address] = value;
    }

    std::uint8_t read_io(std::uint16_t /*port*/) override { return 0xFF; }

    void write_io(std::uint16_t /*port*/, std::uint8_t /*value*/) override {}

    int on_m_cycle(std::uint16_t /*address*/, int /*t_states*/) override { return 0; }
};
}  // namespace

TEST_CASE("ZEXDOC compliance test", "[zexdoc][compliance]") {
    ZexdocBus bus;
    Z80 cpu(bus);

    // Read zexdoc.com into memory at 0x0100
    std::ifstream file("tests/zexdoc.com", std::ios::binary | std::ios::ate);
    if (!file) {
        SKIP("zexdoc.com not found, skipping compliance test");
    }
    const std::streamsize size = file.tellg();
    if (size > (0x10000 - 0x0100)) {
        FAIL("zexdoc.com is too large and would overflow memory");
    }
    file.seekg(0, std::ios::beg);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    file.read(reinterpret_cast<char*>(bus.memory.data() + 0x0100), size);
    REQUIRE(file.gcount() == size);

    cpu.reset();
    z80f::Snapshot snapshot = cpu.save_snapshot();
    snapshot.registers.pc = 0x0100;
    snapshot.registers.sp = 0xF000;
    cpu.load_snapshot(snapshot);

    // Patch CP/M system call at 0x0005
    bus.memory[0x0005] = 0xC9;  // RET

    while (!bus.finished) {
        const auto& r = cpu.registers();
        if (r.pc == 0x0005) {
            // CP/M BDOS call
            if (r.c == 2) {
                // Print char in E
                bus.output += static_cast<char>(r.e);
                std::cout << static_cast<char>(r.e) << std::flush;
            } else if (r.c == 9) {
                // Print string at DE until '$'
                std::uint16_t addr = r.de();
                while (bus.memory[addr] != '$') {
                    bus.output += static_cast<char>(bus.memory[addr]);
                    std::cout << static_cast<char>(bus.memory[addr]) << std::flush;
                    addr++;
                }
            }
        } else if (r.pc == 0x0000) {
            bus.finished = true;
            break;
        }

        cpu.step();

        // Safety timeout just in case it hangs
        if (cpu.cycle_counter() > 100000000000ULL) {
            FAIL("Test timed out!");
        }
    }

    REQUIRE(!bus.output.contains("ERROR"));
    REQUIRE(bus.output.contains("Tests complete"));
}
