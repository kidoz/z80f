#include <catch2/catch_message.hpp>
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

TEST_CASE("Block I/O computes carry, half carry, parity and WZ in both directions",
          "[block][flags][host]") {
    for (std::uint8_t const opcode : {0xA2, 0xAA, 0xB2, 0xBA, 0xA3, 0xAB, 0xB3, 0xBB}) {
        const bool input = (opcode & 1) == 0;
        const bool decrement = (opcode & 8) != 0;
        const bool repeat = (opcode & 0x10) != 0;
        const int delta = decrement ? -1 : 1;
        for (bool const last : {false, true}) {
            CAPTURE(opcode, last);
            TestBus bus;
            Z80 cpu(bus);
            auto s = cpu.save_snapshot();
            s.registers.b = last ? 1 : 2;
            s.registers.c = decrement ? 0x00 : 0xFE;
            s.registers.set_hl(decrement ? 0x4000 : 0x40FE);
            s.registers.wz = 0x1234;
            s.registers.flags.bits = 0xFF;
            cpu.load_snapshot(s);
            bus.memory[0] = 0xED;
            bus.memory[1] = opcode;
            bus.memory[s.registers.hl()] = 0xFF;
            bus.io_in_value = 0xFF;
            const auto port = static_cast<std::uint16_t>(s.registers.bc() - (input ? 0 : 0x100));
            REQUIRE(cpu.step() == (repeat && !last ? 21 : 16));
            REQUIRE(cpu.registers().flags.bits == (last ? 0x57 : 0x13));
            REQUIRE(cpu.registers().wz == static_cast<std::uint16_t>(port + delta));
            REQUIRE(cpu.registers().hl() == static_cast<std::uint16_t>(s.registers.hl() + delta));
            REQUIRE(cpu.registers().b == (last ? 0 : 1));
            REQUIRE(cpu.registers().pc == (repeat && !last ? 0 : 2));
            REQUIRE(bus.io_log.size() == 1);
            REQUIRE(bus.io_log[0].port == port);
            REQUIRE(bus.io_log[0].value == 0xFF);
            REQUIRE(bus.io_log[0].is_read == input);
            REQUIRE(bus.memory[s.registers.hl()] == 0xFF);
        }
    }
}

TEST_CASE("Block I/O clears H and C and handles the zero and sign boundaries", "[block][flags]") {
    for (std::uint8_t const opcode : {0xA2, 0xAA, 0xA3, 0xAB}) {
        const bool input = (opcode & 1) == 0;
        const bool decrement = (opcode & 8) != 0;
        for (std::uint8_t const b : {1, 0x81}) {
            for (std::uint8_t const byte : {0, 0x80}) {
                TestBus bus;
                Z80 cpu(bus);
                auto s = cpu.save_snapshot();
                s.registers.b = b;
                s.registers.c = decrement ? 1 : 0xFF;
                s.registers.set_hl(decrement ? 0x4001 : 0x40FF);
                s.registers.flags.bits = 0xFF;
                cpu.load_snapshot(s);
                bus.memory[0] = 0xED;
                bus.memory[1] = opcode;
                bus.memory[s.registers.hl()] = byte;
                bus.io_in_value = byte;
                REQUIRE(cpu.step() == 16);
                const auto flags =
                    static_cast<std::uint8_t>((b == 1 ? 0x44 : 0x80) | (byte == 0x80 ? 2 : 0));
                REQUIRE(cpu.registers().flags.bits == flags);
                if (input) {
                    REQUIRE(bus.memory[s.registers.hl()] == byte);
                }
            }
        }
    }
}

TEST_CASE("Block transfer and search maintain WZ across repeats and terminal iterations",
          "[block][flags]") {
    for (std::uint8_t const opcode : {0xA0, 0xA8, 0xB0, 0xB8, 0xA1, 0xA9, 0xB1, 0xB9}) {
        const bool compare = (opcode & 1) != 0;
        const bool repeat = (opcode & 0x10) != 0;
        const int delta = (opcode & 8) != 0 ? -1 : 1;
        TestBus bus;
        Z80 cpu(bus);
        auto s = cpu.save_snapshot();
        s.registers.pc = 0x2800;
        s.registers.a = 1;
        s.registers.set_bc(2);
        s.registers.set_hl(0x4000);
        s.registers.set_de(0x5000);
        s.registers.wz = delta > 0 ? 0xFFFF : 0;
        cpu.load_snapshot(s);
        bus.memory[0x2800] = 0xED;
        bus.memory[0x2801] = opcode;
        REQUIRE(cpu.step() == (repeat ? 21 : 16));
        const auto next_wz =
            compare ? static_cast<std::uint16_t>(s.registers.wz + delta) : s.registers.wz;
        REQUIRE(cpu.registers().wz == (repeat ? 0x2801 : next_wz));
        if (repeat) {
            REQUIRE(cpu.step() == 16);
            REQUIRE(cpu.registers().wz == (compare ? 0x2801 + delta : 0x2801));
            REQUIRE(cpu.registers().pc == 0x2802);
        }
    }
}
