#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <z80f/bus.hpp>
#include <z80f/z80.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

struct Cycle {
    std::uint16_t address;
    int duration;
    std::uint64_t start;
    int waits;
};

struct Access {
    std::uint16_t address;
    std::uint64_t time;
    bool write;
};

class CycleBus final : public z80f::Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};
    std::vector<Cycle> cycles;
    std::vector<Access> accesses;
    z80f::Z80* cpu = nullptr;
    bool contention = false;

    std::uint8_t read_memory(std::uint16_t address) override {
        accesses.push_back({.address = address, .time = cpu->cycle_counter(), .write = false});
        return memory[address];
    }
    void write_memory(std::uint16_t address, std::uint8_t value) override {
        accesses.push_back({.address = address, .time = cpu->cycle_counter(), .write = true});
        memory[address] = value;
    }
    std::uint8_t read_io(std::uint16_t /*port*/) override { return 0x55; }
    void write_io(std::uint16_t /*port*/, std::uint8_t /*value*/) override {}
    int on_m_cycle(std::uint16_t address, int t_states) override {
        const auto start = cpu->cycle_counter();
        const int waits = contention ? static_cast<int>((start + address) % 3) : 0;
        cycles.push_back(
            {.address = address, .duration = t_states, .start = start, .waits = waits});
        return waits;
    }
};

void check_cycles(const CycleBus& bus, int nominal, std::uint64_t start, int elapsed) {
    int base = 0;
    int waits = 0;
    for (const auto& cycle : bus.cycles) {
        REQUIRE(cycle.start == start + base + waits);
        base += cycle.duration;
        waits += cycle.waits;
    }
    REQUIRE(base == nominal);
    REQUIRE(elapsed == nominal + waits);
    REQUIRE(bus.cpu->cycle_counter() == start + elapsed);
}

struct Program {
    std::vector<std::uint8_t> bytes;
    int nominal;
};

}  // namespace

TEST_CASE("All timing families expose every T-state and advance the callback clock",
          "[timing][host][bus]") {
    const std::vector<Program> programs = {{.bytes = {0x00}, .nominal = 4},
                                           {.bytes = {0x03}, .nominal = 6},
                                           {.bytes = {0x0B}, .nominal = 6},
                                           {.bytes = {0x09}, .nominal = 11},
                                           {.bytes = {0x34}, .nominal = 11},
                                           {.bytes = {0x35}, .nominal = 11},
                                           {.bytes = {0x10, 0}, .nominal = 13},
                                           {.bytes = {0x18, 0}, .nominal = 12},
                                           {.bytes = {0x20, 0}, .nominal = 12},
                                           {.bytes = {0x28, 0}, .nominal = 7},
                                           {.bytes = {0xC0}, .nominal = 11},
                                           {.bytes = {0xC8}, .nominal = 5},
                                           {.bytes = {0xC4, 0, 0}, .nominal = 17},
                                           {.bytes = {0xCC, 0, 0}, .nominal = 10},
                                           {.bytes = {0xCD, 0, 0}, .nominal = 17},
                                           {.bytes = {0xC5}, .nominal = 11},
                                           {.bytes = {0xFF}, .nominal = 11},
                                           {.bytes = {0xE3}, .nominal = 19},
                                           {.bytes = {0xF9}, .nominal = 6},
                                           {.bytes = {0xD3, 0}, .nominal = 11},
                                           {.bytes = {0xCB, 0x46}, .nominal = 12},
                                           {.bytes = {0xCB, 0x06}, .nominal = 15},
                                           {.bytes = {0xED, 0x4A}, .nominal = 15},
                                           {.bytes = {0xED, 0x42}, .nominal = 15},
                                           {.bytes = {0xED, 0x47}, .nominal = 9},
                                           {.bytes = {0xED, 0x4F}, .nominal = 9},
                                           {.bytes = {0xED, 0x57}, .nominal = 9},
                                           {.bytes = {0xED, 0x5F}, .nominal = 9},
                                           {.bytes = {0xED, 0x67}, .nominal = 18},
                                           {.bytes = {0xED, 0x6F}, .nominal = 18},
                                           {.bytes = {0xED, 0xA0}, .nominal = 16},
                                           {.bytes = {0xED, 0xB0}, .nominal = 21},
                                           {.bytes = {0xED, 0xA1}, .nominal = 16},
                                           {.bytes = {0xED, 0xB1}, .nominal = 21},
                                           {.bytes = {0xED, 0xA2}, .nominal = 16},
                                           {.bytes = {0xED, 0xB2}, .nominal = 21},
                                           {.bytes = {0xED, 0xA3}, .nominal = 16},
                                           {.bytes = {0xED, 0xB3}, .nominal = 21},
                                           {.bytes = {0xDD, 0x23}, .nominal = 10},
                                           {.bytes = {0xFD, 0x2B}, .nominal = 10},
                                           {.bytes = {0xDD, 0x09}, .nominal = 15},
                                           {.bytes = {0xDD, 0x34, 0}, .nominal = 23},
                                           {.bytes = {0xFD, 0x35, 0}, .nominal = 23},
                                           {.bytes = {0xDD, 0x36, 0, 1}, .nominal = 19},
                                           {.bytes = {0xFD, 0x46, 0}, .nominal = 19},
                                           {.bytes = {0xDD, 0xE5}, .nominal = 15},
                                           {.bytes = {0xFD, 0xE3}, .nominal = 23},
                                           {.bytes = {0xDD, 0xF9}, .nominal = 10},
                                           {.bytes = {0xDD, 0xCB, 0, 0x46}, .nominal = 20},
                                           {.bytes = {0xFD, 0xCB, 0, 0x06}, .nominal = 23},
                                           {.bytes = {0xDD, 0xFD, 0xDD, 0x00}, .nominal = 16},
                                           {.bytes = {0xFD, 0xED, 0x4A}, .nominal = 19}};
    for (bool const contention : {false, true}) {
        for (const auto& program : programs) {
            CAPTURE(program.bytes, contention);
            CycleBus bus;
            z80f::Z80 cpu(bus);
            bus.cpu = &cpu;
            bus.contention = contention;
            auto s = cpu.save_snapshot();
            s.cycle_counter = 100;
            s.registers.sp = 0x9000;
            s.registers.set_bc(2);
            s.registers.set_hl(0x4000);
            s.registers.set_de(0x5000);
            s.registers.ix = 0x4000;
            s.registers.iy = 0x4000;
            s.registers.flags.bits = 0;
            s.registers.a = 0x10;
            cpu.load_snapshot(s);
            std::uint16_t address = 0;
            for (const auto byte : program.bytes) {
                bus.memory[address++] = byte;
            }
            bus.memory[0x4000] = 0x55;
            const int elapsed = cpu.step();
            check_cycles(bus, program.nominal, 100, elapsed);
        }
    }
}

TEST_CASE("Read-modify-write and indexed delays occur before the affected access",
          "[timing][host][indexed]") {
    CycleBus bus;
    z80f::Z80 cpu(bus);
    bus.cpu = &cpu;
    auto s = cpu.save_snapshot();
    s.registers.set_hl(0x4000);
    cpu.load_snapshot(s);
    bus.memory[0] = 0x34;
    REQUIRE(cpu.step() == 11);
    REQUIRE(bus.cycles.size() == 4);
    REQUIRE(bus.cycles[2].address == 0x4000);
    REQUIRE(bus.cycles[2].start == 7);
    REQUIRE(bus.cycles[2].duration == 1);
    REQUIRE(bus.accesses.size() == 3);
    REQUIRE(bus.accesses[1].time == 7);
    REQUIRE(bus.accesses[2].time == 11);
    REQUIRE(bus.accesses[2].write);

    cpu.reset();
    bus.cycles.clear();
    bus.accesses.clear();
    s = cpu.save_snapshot();
    s.registers.ix = 0x4000;
    cpu.load_snapshot(s);
    bus.memory[0] = 0xDD;
    bus.memory[1] = 0x46;
    bus.memory[2] = 5;
    REQUIRE(cpu.step() == 19);
    REQUIRE(bus.cycles.size() == 9);
    for (std::size_t i = 3; i < 8; ++i) {
        REQUIRE(bus.cycles[i].address == 2);
        REQUIRE(bus.cycles[i].start == 8 + i);
        REQUIRE(bus.cycles[i].duration == 1);
    }
    REQUIRE(bus.accesses.back().address == 0x4005);
    REQUIRE(bus.accesses.back().time == 19);
}

TEST_CASE("HALT and interrupt cycles include dynamic host waits", "[timing][interrupts][halt]") {
    for (int mode = -1; mode <= 2; ++mode) {
        CycleBus bus;
        z80f::Z80 cpu(bus);
        bus.cpu = &cpu;
        bus.contention = true;
        auto s = cpu.save_snapshot();
        s.registers.sp = 0x9000;
        s.registers.im = static_cast<std::uint8_t>(mode < 0 ? 0 : mode);
        s.registers.iff1 = true;
        cpu.load_snapshot(s);
        bus.memory[0] = 0x76;
        int elapsed = cpu.step();
        check_cycles(bus, 4, 0, elapsed);
        bus.cycles.clear();
        auto start = cpu.cycle_counter();
        elapsed = cpu.step();
        check_cycles(bus, 4, start, elapsed);
        REQUIRE(bus.accesses.size() == 2);
        bus.cycles.clear();
        start = cpu.cycle_counter();
        elapsed = cpu.run_for(8);
        REQUIRE(elapsed >= 8);
        REQUIRE(cpu.cycle_counter() - start == static_cast<std::uint64_t>(elapsed));
        bus.cycles.clear();
        start = cpu.cycle_counter();
        if (mode < 0) {
            cpu.set_nmi_line(true);
        } else {
            cpu.set_int_line(true);
        }
        elapsed = cpu.step();
        const int maskable_t_states = mode == 2 ? 19 : 13;
        check_cycles(bus, mode < 0 ? 11 : maskable_t_states, start, elapsed);
    }
}
