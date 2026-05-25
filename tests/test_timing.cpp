#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("NOP takes 4 T-states", "[timing]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;
    bus.memory[0x0000] = 0x00;

    int const t = cpu.step();

    REQUIRE(t == 4);
}

TEST_CASE("LD r,n takes 7 T-states", "[timing]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;
    bus.memory[0x0000] = 0x06;  // LD B,n
    bus.memory[0x0001] = 0x42;

    int const t = cpu.step();

    REQUIRE(t == 7);
}

TEST_CASE("JR taken vs not-taken", "[timing][branch]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.flags.set_zero(true);  // JR Z taken

    bus.memory[0x0000] = 0x28;  // JR Z,+2
    bus.memory[0x0001] = 0x02;

    int t = cpu.step();
    REQUIRE(t == 12);

    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;
    z80f::detail_z80_access::mutable_registers(cpu).flags.set_zero(false);  // not taken
    bus.memory[0x0000] = 0x28;
    bus.memory[0x0001] = 0x02;
    t = cpu.step();
    REQUIRE(t == 7);
}

TEST_CASE("DJNZ taken takes 13 T-states", "[timing][branch]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.b = 2;

    bus.memory[0x0000] = 0x10;  // DJNZ +2
    bus.memory[0x0001] = 0x02;

    int const t = cpu.step();
    REQUIRE(t == 13);
    REQUIRE(cpu.registers().b == 1);
}

TEST_CASE("CALL nn takes 17 T-states", "[timing]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;
    bus.memory[0x0000] = 0xCD;
    bus.memory[0x0001] = 0x00;
    bus.memory[0x0002] = 0x10;

    int const t = cpu.step();
    REQUIRE(t == 17);
}
