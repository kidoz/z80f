#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("EI then INT is delayed by one instruction", "[interrupts][ei]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.im = 1;
    bus.memory[0x0038] = 0xC9;  // RET at IM 1 handler

    bus.memory[0x0000] = 0xFB;  // EI
    bus.memory[0x0001] = 0x00;  // NOP
    bus.memory[0x0002] = 0x00;  // NOP

    cpu.set_int_line(true);

    cpu.step();  // EI — IFF1=1 but ei_pending blocks INT this step
    REQUIRE(cpu.registers().iff1);
    REQUIRE(cpu.registers().pc == 0x0001);

    cpu.step();  // NOP — ei_pending is cleared, NOP completes; INT not yet acked
    REQUIRE(cpu.registers().iff1);
    REQUIRE(cpu.registers().pc == 0x0002);

    cpu.step();  // Next step recognizes INT before executing PC=2
    REQUIRE_FALSE(cpu.registers().iff1);
    REQUIRE(cpu.registers().pc == 0x0038);
}

TEST_CASE("IM 1 jumps to 0x0038", "[interrupts][im1]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0x1000;
    r.iff1 = true;
    r.im = 1;
    r.sp = 0xFFF0;

    bus.memory[0x1000] = 0x00;  // NOP, but should be preempted

    cpu.set_int_line(true);
    cpu.step();

    REQUIRE(cpu.registers().pc == 0x0038);
    REQUIRE_FALSE(cpu.registers().iff1);
    REQUIRE(cpu.registers().sp == 0xFFEE);
}

TEST_CASE("NMI vectors to 0x0066 and preserves IFF2", "[interrupts][nmi]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0x2000;
    r.iff1 = true;
    r.iff2 = true;
    r.sp = 0xFFF0;

    cpu.set_nmi_line(true);
    cpu.step();

    REQUIRE(cpu.registers().pc == 0x0066);
    REQUIRE_FALSE(cpu.registers().iff1);
    REQUIRE(cpu.registers().iff2);  // preserved
}

TEST_CASE("RETN restores IFF1 from IFF2", "[interrupts]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0x0066;
    r.iff1 = false;
    r.iff2 = true;
    r.sp = 0xFFFE;
    bus.memory[0xFFFE] = 0x00;
    bus.memory[0xFFFF] = 0x20;  // return address 0x2000

    bus.memory[0x0066] = 0xED;
    bus.memory[0x0067] = 0x45;  // RETN

    cpu.step();

    REQUIRE(cpu.registers().pc == 0x2000);
    REQUIRE(cpu.registers().iff1);  // restored from IFF2
}

TEST_CASE("HALT loops until interrupt", "[interrupts][halt]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.iff1 = true;
    r.im = 1;
    r.sp = 0xFFF0;

    bus.memory[0x0000] = 0x76;  // HALT

    cpu.step();
    REQUIRE(cpu.registers().halted);
    REQUIRE(cpu.registers().pc == 0x0000);

    cpu.step();
    REQUIRE(cpu.registers().halted);  // still halted

    cpu.set_int_line(true);
    cpu.step();

    REQUIRE_FALSE(cpu.registers().halted);
    REQUIRE(cpu.registers().pc == 0x0038);  // IM 1 vector
}
