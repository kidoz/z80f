#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("ADC adds carry", "[alu]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x10;
    r.flags.set_carry(true);

    bus.memory[0x0000] = 0xCE;  // ADC A,n
    bus.memory[0x0001] = 0x20;

    cpu.step();

    REQUIRE(cpu.registers().a == 0x31);
    REQUIRE_FALSE(cpu.registers().flags.carry());
}

TEST_CASE("SBC subtracts with borrow", "[alu]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x10;
    r.flags.set_carry(true);

    bus.memory[0x0000] = 0xDE;  // SBC A,n
    bus.memory[0x0001] = 0x01;

    cpu.step();

    REQUIRE(cpu.registers().a == 0x0E);
    REQUIRE(cpu.registers().flags.add_sub());
    REQUIRE_FALSE(cpu.registers().flags.carry());
}

TEST_CASE("ADD HL,DE", "[alu][16bit]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_hl(0x1000);
    r.set_de(0x0234);

    bus.memory[0x0000] = 0x19;  // ADD HL,DE

    cpu.step();

    REQUIRE(cpu.registers().hl() == 0x1234);
    REQUIRE_FALSE(cpu.registers().flags.carry());
    REQUIRE_FALSE(cpu.registers().flags.add_sub());
}

TEST_CASE("ADD HL,HL overflows into carry", "[alu][16bit]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_hl(0x8000);

    bus.memory[0x0000] = 0x29;  // ADD HL,HL

    cpu.step();

    REQUIRE(cpu.registers().hl() == 0x0000);
    REQUIRE(cpu.registers().flags.carry());
}

TEST_CASE("SBC HL,BC sets PF on signed overflow", "[alu][16bit]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_hl(0x8000);
    r.set_bc(0x0001);
    r.flags.set_carry(false);

    bus.memory[0x0000] = 0xED;  // SBC HL,BC
    bus.memory[0x0001] = 0x42;

    cpu.step();

    REQUIRE(cpu.registers().hl() == 0x7FFF);
    REQUIRE(cpu.registers().flags.parity_overflow());
    REQUIRE(cpu.registers().flags.add_sub());
}

TEST_CASE("DAA after BCD addition", "[alu][daa]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x15;

    bus.memory[0x0000] = 0xC6;  // ADD A,0x27
    bus.memory[0x0001] = 0x27;
    bus.memory[0x0002] = 0x27;  // DAA

    cpu.step();  // A = 0x3C, H set because 5+7=12
    cpu.step();  // DAA -> should adjust low nibble +6 = 0x42

    REQUIRE(cpu.registers().a == 0x42);
}

TEST_CASE("NEG of 0x01 = 0xFF, carry set", "[alu]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x01;

    bus.memory[0x0000] = 0xED;  // NEG
    bus.memory[0x0001] = 0x44;

    cpu.step();

    REQUIRE(cpu.registers().a == 0xFF);
    REQUIRE(cpu.registers().flags.carry());
    REQUIRE(cpu.registers().flags.add_sub());
    REQUIRE_FALSE(cpu.registers().flags.parity_overflow());
}
