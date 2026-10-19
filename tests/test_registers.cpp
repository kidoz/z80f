#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include <cstdint>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("register file after reset", "[registers][reset]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();

    const auto& r = cpu.registers();
    REQUIRE(r.pc == 0x0000);
    REQUIRE(r.sp == 0xFFFF);
    REQUIRE(r.a == 0xFF);
    REQUIRE(r.flags.bits == 0xFF);
    REQUIRE(r.iff1 == false);
    REQUIRE(r.iff2 == false);
    REQUIRE(r.im == 0);
    REQUIRE(r.halted == false);
    REQUIRE(r.i == 0);
    REQUIRE(r.r == 0);
}

TEST_CASE("16-bit register composite accessors", "[registers]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);

    r.set_bc(0x1234);
    REQUIRE(r.b == 0x12);
    REQUIRE(r.c == 0x34);
    REQUIRE(r.bc() == 0x1234);

    r.set_de(0xDEAD);
    REQUIRE(r.de() == 0xDEAD);

    r.set_hl(0xBEEF);
    REQUIRE(r.hl() == 0xBEEF);

    r.set_af(0xCAFE);
    REQUIRE(r.a == 0xCA);
    REQUIRE(r.flags.bits == 0xFE);
}

TEST_CASE("R register increments on M1 only and preserves bit 7", "[registers][r]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).r = 0x80;  // bit 7 set, low 7 = 0

    bus.memory[0x0000] = 0x00;  // NOP
    bus.memory[0x0001] = 0x00;  // NOP
    bus.memory[0x0002] = 0x00;  // NOP

    cpu.step();
    cpu.step();
    cpu.step();

    REQUIRE((cpu.registers().r & 0x80) == 0x80);
    REQUIRE((cpu.registers().r & 0x7F) == 3);
}

TEST_CASE("LD R,A copies all eight bits before subsequent refresh increments", "[registers][r]") {
    for (std::uint8_t const value : {0x00, 0x7F, 0x80, 0xFF}) {
        CAPTURE(value);
        TestBus bus;
        Z80 cpu(bus);
        auto s = cpu.save_snapshot();
        s.registers.a = value;
        s.registers.r = static_cast<std::uint8_t>(value ^ 0x80);
        cpu.load_snapshot(s);
        bus.memory[0] = 0xED;
        bus.memory[1] = 0x4F;
        bus.memory[2] = 0xED;
        bus.memory[3] = 0x5F;
        REQUIRE(cpu.step() == 9);
        REQUIRE(cpu.registers().r == value);
        REQUIRE(cpu.registers().flags.bits == s.registers.flags.bits);
        REQUIRE(cpu.step() == 9);
        REQUIRE(cpu.registers().a ==
                static_cast<std::uint8_t>((value & 0x80) | ((value + 2) & 0x7F)));
    }
}
