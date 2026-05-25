#include <catch2/catch_test_macros.hpp>
#include <z80f/z80.hpp>

#include <cstdint>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

TEST_CASE("LDIR copies a block", "[block][ldir]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_hl(0x1000);
    r.set_de(0x2000);
    r.set_bc(4);

    for (int i = 0; i < 4; ++i) {
        bus.memory[0x1000 + i] = static_cast<std::uint8_t>(0xA0 + i);
    }

    bus.memory[0x0000] = 0xED;
    bus.memory[0x0001] = 0xB0;  // LDIR

    // LDIR repeats internally by re-decoding from PC-2 until BC=0.
    cpu.run_for(200);

    REQUIRE(bus.memory[0x2000] == 0xA0);
    REQUIRE(bus.memory[0x2001] == 0xA1);
    REQUIRE(bus.memory[0x2002] == 0xA2);
    REQUIRE(bus.memory[0x2003] == 0xA3);
    REQUIRE(cpu.registers().bc() == 0);
    REQUIRE(cpu.registers().hl() == 0x1004);
    REQUIRE(cpu.registers().de() == 0x2004);
}

TEST_CASE("CPIR scans for a match", "[block][cpir]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.a = 0x42;
    r.set_hl(0x1000);
    r.set_bc(8);

    bus.memory[0x1000] = 0x10;
    bus.memory[0x1001] = 0x20;
    bus.memory[0x1002] = 0x42;
    bus.memory[0x1003] = 0x55;

    bus.memory[0x0000] = 0xED;
    bus.memory[0x0001] = 0xB1;  // CPIR

    cpu.run_for(200);

    REQUIRE(cpu.registers().hl() == 0x1003);  // stops one past match
    REQUIRE(cpu.registers().flags.zero());
}

TEST_CASE("LDDR copies a block backwards", "[block][lddr]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.set_hl(0x1003);
    r.set_de(0x2003);
    r.set_bc(4);

    for (int i = 0; i < 4; ++i) {
        bus.memory[0x1000 + i] = static_cast<std::uint8_t>(0x10 + i);
    }

    bus.memory[0x0000] = 0xED;
    bus.memory[0x0001] = 0xB8;  // LDDR

    cpu.run_for(200);

    REQUIRE(bus.memory[0x2000] == 0x10);
    REQUIRE(bus.memory[0x2003] == 0x13);
    REQUIRE(cpu.registers().bc() == 0);
}
