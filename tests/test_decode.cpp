#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include <cstdint>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("LD r,r' between B and C", "[decode][ld]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.b = 0xAB;

    bus.memory[0x0000] = 0x48;  // LD C,B

    cpu.step();

    REQUIRE(cpu.registers().c == 0xAB);
}

TEST_CASE("LD r,(HL) and LD (HL),r", "[decode][ld]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_hl(0x2000);
    bus.memory[0x2000] = 0x99;

    bus.memory[0x0000] = 0x7E;  // LD A,(HL)
    bus.memory[0x0001] = 0x06;  // LD B,n
    bus.memory[0x0002] = 0x77;
    bus.memory[0x0003] = 0x70;  // LD (HL),B

    cpu.step();
    REQUIRE(cpu.registers().a == 0x99);

    cpu.step();  // LD B,0x77
    cpu.step();  // LD (HL),B
    REQUIRE(bus.memory[0x2000] == 0x77);
}

TEST_CASE("LD rr,nn and LD (nn),HL", "[decode][ld16]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    bus.memory[0x0000] = 0x21;
    bus.memory[0x0001] = 0x34;
    bus.memory[0x0002] = 0x12;  // LD HL,0x1234
    bus.memory[0x0003] = 0x22;
    bus.memory[0x0004] = 0x00;
    bus.memory[0x0005] = 0x30;  // LD (0x3000),HL

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().hl() == 0x1234);
    REQUIRE(bus.memory[0x3000] == 0x34);
    REQUIRE(bus.memory[0x3001] == 0x12);
}

TEST_CASE("EX DE,HL swaps", "[decode][exchange]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_de(0xAABB);
    r.set_hl(0xCCDD);

    bus.memory[0x0000] = 0xEB;

    cpu.step();

    REQUIRE(cpu.registers().de() == 0xCCDD);
    REQUIRE(cpu.registers().hl() == 0xAABB);
}

TEST_CASE("EXX swaps general-purpose pairs", "[decode][exchange]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_bc(0x1111);
    r.set_de(0x2222);
    r.set_hl(0x3333);
    r.b_alt = 0xAA;
    r.c_alt = 0xAA;
    r.d_alt = 0xBB;
    r.e_alt = 0xBB;
    r.h_alt = 0xCC;
    r.l_alt = 0xCC;

    bus.memory[0x0000] = 0xD9;

    cpu.step();

    REQUIRE(cpu.registers().bc() == 0xAAAA);
    REQUIRE(cpu.registers().de() == 0xBBBB);
    REQUIRE(cpu.registers().hl() == 0xCCCC);
    REQUIRE(cpu.registers().b_alt == 0x11);
}

TEST_CASE("JP nn and JR e", "[decode][jump]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    bus.memory[0x0000] = 0xC3;
    bus.memory[0x0001] = 0x10;
    bus.memory[0x0002] = 0x00;  // JP 0x0010
    bus.memory[0x0010] = 0x18;
    bus.memory[0x0011] = static_cast<std::uint8_t>(-4);  // JR -4 -> 0x000E

    cpu.step();  // JP
    REQUIRE(cpu.registers().pc == 0x0010);

    cpu.step();  // JR -4 -> 0x0012 + (-4) = 0x000E
    REQUIRE(cpu.registers().pc == 0x000E);
}

TEST_CASE("CB SET and RES", "[decode][cb]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.b = 0x00;

    bus.memory[0x0000] = 0xCB;
    bus.memory[0x0001] = 0xC0;  // SET 0,B
    bus.memory[0x0002] = 0xCB;
    bus.memory[0x0003] = 0xF8;  // SET 7,B
    bus.memory[0x0004] = 0xCB;
    bus.memory[0x0005] = 0x80;  // RES 0,B

    cpu.step();
    REQUIRE(cpu.registers().b == 0x01);
    cpu.step();
    REQUIRE(cpu.registers().b == 0x81);
    cpu.step();
    REQUIRE(cpu.registers().b == 0x80);
}

TEST_CASE("CB BIT 7 of 0x80 clears Z", "[decode][cb][bit]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x80;

    bus.memory[0x0000] = 0xCB;
    bus.memory[0x0001] = 0x7F;  // BIT 7,A

    cpu.step();
    REQUIRE_FALSE(cpu.registers().flags.zero());
    REQUIRE(cpu.registers().flags.sign());
    REQUIRE(cpu.registers().flags.half_carry());
}
