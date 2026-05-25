#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include <cstdint>
#include <initializer_list>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

namespace {

void load_program(TestBus& bus, std::uint16_t at, std::initializer_list<std::uint8_t> bytes) {
    std::uint16_t a = at;
    for (auto b : bytes) {
        bus.memory[a++] = b;
    }
}

}  // namespace

TEST_CASE("ADD A,n sets flags correctly", "[alu][flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    load_program(bus, 0x0000,
                 {
                     0x3E,
                     0x7F,  // LD A,0x7F
                     0xC6,
                     0x01,  // ADD A,0x01
                 });

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().a == 0x80);
    REQUIRE(cpu.registers().flags.sign());
    REQUIRE(cpu.registers().flags.parity_overflow());
    REQUIRE(cpu.registers().flags.half_carry());
    REQUIRE_FALSE(cpu.registers().flags.zero());
    REQUIRE_FALSE(cpu.registers().flags.carry());
    REQUIRE_FALSE(cpu.registers().flags.add_sub());
}

TEST_CASE("SUB sets zero and N flags on equal operands", "[alu][flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    load_program(bus, 0x0000,
                 {
                     0x3E,
                     0x42,  // LD A,0x42
                     0xD6,
                     0x42,  // SUB 0x42
                 });

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().a == 0x00);
    REQUIRE(cpu.registers().flags.zero());
    REQUIRE(cpu.registers().flags.add_sub());
    REQUIRE_FALSE(cpu.registers().flags.carry());
}

TEST_CASE("AND sets H, clears N and C, sets parity", "[alu][flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    load_program(bus, 0x0000,
                 {
                     0x3E,
                     0xF0,  // LD A,0xF0
                     0xE6,
                     0x0F,  // AND 0x0F
                 });

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().a == 0x00);
    REQUIRE(cpu.registers().flags.zero());
    REQUIRE(cpu.registers().flags.half_carry());
    REQUIRE(cpu.registers().flags.parity());
    REQUIRE_FALSE(cpu.registers().flags.carry());
    REQUIRE_FALSE(cpu.registers().flags.add_sub());
}

TEST_CASE("INC sets half-carry on nibble boundary", "[alu][flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;
    z80f::detail_z80_access::mutable_registers(cpu).flags.bits = 0;

    load_program(bus, 0x0000,
                 {
                     0x3E,
                     0x0F,  // LD A,0x0F
                     0x3C,  // INC A
                 });

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().a == 0x10);
    REQUIRE(cpu.registers().flags.half_carry());
    REQUIRE_FALSE(cpu.registers().flags.zero());
}

TEST_CASE("DEC of 0x01 yields zero, not overflow", "[alu][flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    load_program(bus, 0x0000,
                 {
                     0x3E,
                     0x01,
                     0x3D,  // DEC A
                 });

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().a == 0x00);
    REQUIRE(cpu.registers().flags.zero());
    REQUIRE(cpu.registers().flags.add_sub());
    REQUIRE_FALSE(cpu.registers().flags.parity_overflow());
}

TEST_CASE("CPL inverts A and sets H, N", "[flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;

    load_program(bus, 0x0000,
                 {
                     0x3E,
                     0x55,  // LD A,0x55
                     0x2F,  // CPL
                 });

    cpu.step();
    cpu.step();

    REQUIRE(cpu.registers().a == 0xAA);
    REQUIRE(cpu.registers().flags.half_carry());
    REQUIRE(cpu.registers().flags.add_sub());
}

TEST_CASE("SCF and CCF toggle carry correctly", "[flags]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    z80f::detail_z80_access::mutable_registers(cpu).pc = 0;
    z80f::detail_z80_access::mutable_registers(cpu).flags.bits = 0;

    load_program(bus, 0x0000,
                 {
                     0x37,  // SCF
                     0x3F,  // CCF
                 });

    cpu.step();
    REQUIRE(cpu.registers().flags.carry());

    cpu.step();
    REQUIRE_FALSE(cpu.registers().flags.carry());
    REQUIRE(cpu.registers().flags.half_carry());  // CCF: H = old C
}
