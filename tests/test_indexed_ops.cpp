#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("LD IX,nn and LD A,(IX+d)", "[indexed][ix]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    bus.memory[0x0000] = 0xDD;
    bus.memory[0x0001] = 0x21;
    bus.memory[0x0002] = 0x00;
    bus.memory[0x0003] = 0x30;  // LD IX,0x3000

    bus.memory[0x3005] = 0x77;
    bus.memory[0x0004] = 0xDD;
    bus.memory[0x0005] = 0x7E;
    bus.memory[0x0006] = 0x05;  // LD A,(IX+5)

    cpu.step();
    REQUIRE(cpu.registers().ix == 0x3000);

    cpu.step();
    REQUIRE(cpu.registers().a == 0x77);
}

TEST_CASE("LD (IX+d),n", "[indexed][ix]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.ix = 0x4000;

    bus.memory[0x0000] = 0xDD;
    bus.memory[0x0001] = 0x36;
    bus.memory[0x0002] = 0x02;  // d = +2
    bus.memory[0x0003] = 0xAB;  // n

    cpu.step();

    REQUIRE(bus.memory[0x4002] == 0xAB);
}

TEST_CASE("ADD A,(IY+d)", "[indexed][iy]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.iy = 0x5000;
    r.a = 0x10;
    bus.memory[0x5003] = 0x20;

    bus.memory[0x0000] = 0xFD;
    bus.memory[0x0001] = 0x86;
    bus.memory[0x0002] = 0x03;

    cpu.step();

    REQUIRE(cpu.registers().a == 0x30);
}

TEST_CASE("DDCB SET 0,(IX+d)", "[indexed][ddcb]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.ix = 0x6000;
    bus.memory[0x6001] = 0x00;

    bus.memory[0x0000] = 0xDD;
    bus.memory[0x0001] = 0xCB;
    bus.memory[0x0002] = 0x01;  // d = +1
    bus.memory[0x0003] = 0xC6;  // SET 0,(IX+d)

    cpu.step();

    REQUIRE(bus.memory[0x6001] == 0x01);
}
