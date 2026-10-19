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

TEST_CASE("SCF and CCF retain flag history across unchanged results and snapshots",
          "[flags][host][snapshot]") {
    for (std::uint8_t const opcode : {0x37, 0x3F}) {
        TestBus bus;
        Z80 cpu(bus);
        auto s = cpu.save_snapshot();
        s.registers.a = 0;
        s.registers.b = 0x27;
        s.registers.flags.bits = 0x28;
        cpu.load_snapshot(s);
        bus.memory[0] = 0x04;  // INC B writes F=28 even though F already equals 28.
        bus.memory[1] = opcode;
        REQUIRE(cpu.step() == 4);
        REQUIRE(cpu.registers().flags.bits == 0x28);
        REQUIRE(cpu.registers().q == 0x28);
        const auto after_inc = cpu.save_snapshot();
        REQUIRE(cpu.step() == 4);
        REQUIRE(cpu.registers().flags.bits == 0x01);
        cpu.reset();
        cpu.load_snapshot(after_inc);
        REQUIRE(cpu.step() == 4);
        REQUIRE(cpu.registers().flags.bits == 0x01);

        cpu.load_snapshot(after_inc);
        bus.memory[1] = 0x00;
        bus.memory[2] = opcode;
        REQUIRE(cpu.step() == 4);
        REQUIRE(cpu.registers().q == 0);
        REQUIRE(cpu.step() == 4);
        REQUIRE(cpu.registers().flags.bits == 0x29);

        cpu.load_snapshot(after_inc);
        bus.memory[1] = 0xDD;
        bus.memory[2] = opcode;
        REQUIRE(cpu.step() == 8);
        REQUIRE(cpu.registers().flags.bits == 0x29);
    }
}

TEST_CASE("Memory BIT accepts FFFF as an address while register BIT uses operand flags",
          "[flags][cb][indexed]") {
    for (std::uint8_t const prefix : {0x00, 0xDD, 0xFD}) {
        TestBus bus;
        Z80 cpu(bus);
        auto s = cpu.save_snapshot();
        s.registers.wz = 0xFFFF;
        s.registers.set_hl(0x4000);
        s.registers.ix = 0xFFFF;
        s.registers.iy = 0x0000;
        s.registers.b = 0;
        s.registers.flags.bits = 0;
        cpu.load_snapshot(s);
        if (prefix == 0) {
            load_program(bus, 0, {0xCB, 0x46, 0xCB, 0x40});
        } else {
            load_program(bus, 0,
                         {prefix, 0xCB, static_cast<std::uint8_t>(prefix == 0xFD ? 0xFF : 0), 0x46,
                          0xCB, 0x40});
        }
        REQUIRE(cpu.step() == (prefix == 0 ? 12 : 20));
        REQUIRE(cpu.registers().flags.bits == 0x7C);
        REQUIRE(cpu.step() == 8);
        REQUIRE(cpu.registers().flags.bits == 0x54);
    }
}
