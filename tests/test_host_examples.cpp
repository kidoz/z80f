#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("IN A,(n) reads from host I/O", "[host][io]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0xAB;
    bus.io_in_value = 0x5A;

    bus.memory[0x0000] = 0xDB;  // IN A,(n)
    bus.memory[0x0001] = 0xFE;

    cpu.step();

    REQUIRE(cpu.registers().a == 0x5A);
    REQUIRE(bus.io_log.size() == 1);
    REQUIRE(bus.io_log[0].port == 0xABFE);
    REQUIRE(bus.io_log[0].is_read);
}

TEST_CASE("OUT (n),A writes to host I/O", "[host][io]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x12;

    bus.memory[0x0000] = 0xD3;
    bus.memory[0x0001] = 0xFE;

    cpu.step();

    REQUIRE(bus.io_log.size() == 1);
    REQUIRE(bus.io_log[0].port == 0x12FE);
    REQUIRE(bus.io_log[0].value == 0x12);
    REQUIRE_FALSE(bus.io_log[0].is_read);
}

TEST_CASE("on_m_cycle fires for every memory access", "[host][bus]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    bus.memory[0x0000] = 0x00;  // NOP

    int const before = bus.m_cycles;
    cpu.step();
    int const after = bus.m_cycles;

    REQUIRE(after > before);  // at least the fetch cycle was observed
}

TEST_CASE("snapshot round-trip restores full state", "[host][snapshot]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.a = 0x11;
    r.b = 0x22;
    r.c = 0x33;
    r.set_hl(0x4567);
    r.pc = 0x8000;
    r.sp = 0x9000;
    r.iff1 = true;
    r.im = 2;

    auto snap = cpu.save_snapshot();

    cpu.reset();
    REQUIRE(cpu.registers().a != 0x11);

    cpu.load_snapshot(snap);

    REQUIRE(cpu.registers().a == 0x11);
    REQUIRE(cpu.registers().bc() == 0x2233);
    REQUIRE(cpu.registers().hl() == 0x4567);
    REQUIRE(cpu.registers().pc == 0x8000);
    REQUIRE(cpu.registers().sp == 0x9000);
    REQUIRE(cpu.registers().iff1);
    REQUIRE(cpu.registers().im == 2);
}
