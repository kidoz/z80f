#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <z80f/bus.hpp>
#include <z80f/z80.hpp>

#include <array>
#include <cstdint>

#include "test_bus.hpp"

using z80f::Z80;
using z80f::test::TestBus;

namespace {
// Bus with a configurable interrupt-acknowledge data-bus byte for IM 0 / IM 2.
class AckBus final : public z80f::Bus {
public:
    std::array<std::uint8_t, 0x10000> memory{};
    std::uint8_t ack_value = 0xFF;

    std::uint8_t read_memory(std::uint16_t address) override { return memory[address]; }
    void write_memory(std::uint16_t address, std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint8_t read_io(std::uint16_t /*port*/) override { return 0xFF; }
    void write_io(std::uint16_t /*port*/, std::uint8_t /*value*/) override {}
    int on_m_cycle(std::uint16_t /*address*/, int /*t_states*/) override { return 0; }
    std::uint8_t acknowledge_interrupt() override { return ack_value; }
};
}  // namespace

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

TEST_CASE("IM 0 executes the data-bus RST instruction", "[interrupts][im0]") {
    AckBus bus;
    bus.ack_value = 0xD7;  // RST 10h
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0x1000;
    r.iff1 = true;
    r.im = 0;
    r.sp = 0xFFF0;

    cpu.set_int_line(true);
    int const t = cpu.step();

    REQUIRE(cpu.registers().pc == 0x0010);
    REQUIRE(t == 13);
    REQUIRE(cpu.registers().sp == 0xFFEE);
    REQUIRE(bus.memory[0xFFEE] == 0x00);  // pushed return address 0x1000
    REQUIRE(bus.memory[0xFFEF] == 0x10);
    REQUIRE_FALSE(cpu.registers().iff1);
}

TEST_CASE("IM 0 falls back to RST 38h without a bus vector", "[interrupts][im0]") {
    TestBus bus;  // default acknowledge_interrupt() returns 0xFF (RST 38h)
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0x1000;
    r.iff1 = true;
    r.im = 0;
    r.sp = 0xFFF0;

    cpu.set_int_line(true);
    int const t = cpu.step();

    REQUIRE(cpu.registers().pc == 0x0038);
    REQUIRE(t == 13);
}

TEST_CASE("IM 2 vectors through the I register table", "[interrupts][im2]") {
    AckBus bus;
    bus.ack_value = 0x22;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0x1000;
    r.iff1 = true;
    r.im = 2;
    r.i = 0x40;
    r.sp = 0xFFF0;

    bus.memory[0x4022] = 0x34;  // vector table entry -> 0x1234
    bus.memory[0x4023] = 0x12;

    cpu.set_int_line(true);
    int const t = cpu.step();

    REQUIRE(cpu.registers().pc == 0x1234);
    REQUIRE(t == 19);
    REQUIRE(cpu.registers().sp == 0xFFEE);
    REQUIRE(bus.memory[0xFFEE] == 0x00);  // pushed return address 0x1000
    REQUIRE(bus.memory[0xFFEF] == 0x10);
    REQUIRE_FALSE(cpu.registers().iff1);
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

TEST_CASE("Pulse INT clears itself if not accepted", "[interrupts][pulse]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.iff1 = false;
    r.im = 1;
    bus.memory[0x0000] = 0x00;  // NOP
    bus.memory[0x0001] = 0xFB;  // EI
    bus.memory[0x0002] = 0x00;  // NOP
    bus.memory[0x0003] = 0x00;  // NOP

    cpu.pulse_int_line();
    cpu.step();  // NOP at PC 0. Pulse should be consumed here and ignored since IFF1 is false.

    REQUIRE(cpu.registers().pc == 0x0001);

    cpu.step();  // EI
    REQUIRE(cpu.registers().iff1);

    cpu.step();  // NOP at PC 2. Wait 1 instruction after EI.
    cpu.step();  // NOP at PC 3. Should NOT jump to 0x38 because pulse was lost!

    REQUIRE(cpu.registers().pc == 0x0004);
}

TEST_CASE("Pulse INT vectors if accepted", "[interrupts][pulse]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();
    auto& r = z80f::detail_z80_access::mutable_registers(cpu);
    r.pc = 0;
    r.iff1 = true;
    r.im = 1;
    r.sp = 0xFFF0;
    bus.memory[0x0000] = 0x00;  // NOP

    cpu.pulse_int_line();
    cpu.step();  // Takes INT immediately

    REQUIRE(cpu.registers().pc == 0x0038);
    REQUIRE_FALSE(cpu.registers().iff1);

    // Step again to verify the pulse didn't magically re-trigger
    bus.memory[0x0038] = 0xFB;  // EI
    bus.memory[0x0039] = 0x00;  // NOP
    bus.memory[0x003A] = 0x00;  // NOP
    cpu.step();                 // EI
    cpu.step();                 // NOP
    cpu.step();                 // NOP
    REQUIRE(cpu.registers().pc == 0x003B);
}

TEST_CASE("Cycles since reset deltas", "[timing]") {
    TestBus bus;
    Z80 cpu(bus);
    cpu.reset();

    // NOP is 4 cycles
    bus.memory[0x0000] = 0x00;
    bus.memory[0x0001] = 0x00;
    bus.memory[0x0002] = 0x00;

    cpu.step();
    REQUIRE(cpu.cycle_counter() == 4);
    REQUIRE(cpu.cycles_since_reset() == 4);

    cpu.reset_cycle_counter();
    REQUIRE(cpu.cycle_counter() == 4);
    REQUIRE(cpu.cycles_since_reset() == 0);

    cpu.step();
    REQUIRE(cpu.cycle_counter() == 8);
    REQUIRE(cpu.cycles_since_reset() == 4);

    cpu.reset();
    REQUIRE(cpu.cycle_counter() == 0);
    REQUIRE(cpu.cycles_since_reset() == 0);
}

TEST_CASE("Interrupt return resumes after HALT, including address wrap", "[interrupts][halt]") {
    for (bool const nmi : {false, true}) {
        for (std::uint16_t const pc : {std::uint16_t{0}, std::uint16_t{0xFFFF}}) {
            for (std::uint8_t const mode : {0, 1, 2}) {
                CAPTURE(nmi, pc, mode);
                TestBus bus;
                Z80 cpu(bus);
                auto s = cpu.save_snapshot();
                s.registers.pc = pc;
                s.registers.sp = 0x9000;
                s.registers.im = mode;
                s.registers.i = 0x40;
                s.registers.iff1 = true;
                s.registers.iff2 = true;
                cpu.load_snapshot(s);
                bus.memory[pc] = 0x76;
                bus.memory[0x40FF] = 0x38;
                bus.memory[0x4100] = 0;
                bus.memory[nmi ? 0x66 : 0x38] = 0xED;
                bus.memory[nmi ? 0x67 : 0x39] = 0x45;
                REQUIRE(cpu.step() == 4);
                if (nmi) {
                    cpu.set_nmi_line(true);
                } else {
                    cpu.set_int_line(true);
                }
                REQUIRE(cpu.step() == (nmi ? 11 : (mode == 2 ? 19 : 13)));
                REQUIRE_FALSE(cpu.registers().halted);
                const auto next = static_cast<std::uint16_t>(pc + 1);
                REQUIRE(bus.memory[0x8FFE] == static_cast<std::uint8_t>(next));
                REQUIRE(bus.memory[0x8FFF] == static_cast<std::uint8_t>(next >> 8));
                cpu.set_int_line(false);
                REQUIRE(cpu.step() == 14);
                REQUIRE(cpu.registers().pc == next);
                REQUIRE(cpu.registers().sp == 0x9000);
            }
        }
    }
}

TEST_CASE("Nested NMIs preserve IFF2 until both RETNs complete", "[interrupts][nmi]") {
    for (bool const enabled : {false, true}) {
        TestBus bus;
        Z80 cpu(bus);
        auto s = cpu.save_snapshot();
        s.registers.pc = 0x2000;
        s.registers.sp = 0x9000;
        s.registers.iff1 = enabled;
        s.registers.iff2 = enabled;
        cpu.load_snapshot(s);
        bus.memory[0x66] = 0xED;
        bus.memory[0x67] = 0x45;
        cpu.set_nmi_line(true);
        REQUIRE(cpu.step() == 11);
        cpu.set_nmi_line(false);
        cpu.set_nmi_line(true);
        REQUIRE(cpu.step() == 11);
        REQUIRE(cpu.registers().iff2 == enabled);
        REQUIRE(cpu.step() == 14);
        REQUIRE(cpu.step() == 14);
        REQUIRE(cpu.registers().pc == 0x2000);
        REQUIRE(cpu.registers().sp == 0x9000);
        REQUIRE(cpu.registers().iff1 == enabled);
        REQUIRE(cpu.registers().iff2 == enabled);
    }
}
