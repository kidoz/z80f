#include <z80f/z80.hpp>

#include <cstdint>

#include "z80f/bus.hpp"
#include "z80f/snapshot.hpp"

namespace z80f {

Z80::Z80(Bus& bus) : bus_(&bus) {
    reset();
}

void Z80::reset() {
    regs_ = Registers{};
    regs_.pc = 0x0000;
    regs_.sp = 0xFFFF;
    regs_.a = 0xFF;
    regs_.flags.bits = 0xFF;
    regs_.set_bc(0xFFFF);
    regs_.set_de(0xFFFF);
    regs_.set_hl(0xFFFF);
    regs_.ix = 0xFFFF;
    regs_.iy = 0xFFFF;
    regs_.i = 0;
    regs_.r = 0;
    regs_.iff1 = false;
    regs_.iff2 = false;
    regs_.im = 0;
    regs_.halted = false;
    regs_.ei_pending = false;
    regs_.wz = 0;
    cycles_ = 0;
    nmi_pending_ = false;
    int_pulse_pending_ = false;
}

void Z80::inc_r() {
    // R bit 7 is preserved; only bits 0..6 increment.
    regs_.r = static_cast<std::uint8_t>((regs_.r & 0x80) | ((regs_.r + 1) & 0x7F));
}

std::uint8_t Z80::read8(std::uint16_t addr) {
    cycles_ += bus_->on_m_cycle(addr, 3);
    return bus_->read_memory(addr);
}

void Z80::write8(std::uint16_t addr, std::uint8_t value) {
    cycles_ += bus_->on_m_cycle(addr, 3);
    bus_->write_memory(addr, value);
}

std::uint8_t Z80::fetch_opcode() {
    std::uint16_t pc = regs_.pc;
    cycles_ += bus_->on_m_cycle(pc, 4);
    std::uint8_t op = bus_->read_memory(pc);
    regs_.pc = static_cast<std::uint16_t>(pc + 1);
    inc_r();
    return op;
}

std::uint8_t Z80::fetch_immediate() {
    std::uint8_t v = read8(regs_.pc);
    regs_.pc = static_cast<std::uint16_t>(regs_.pc + 1);
    return v;
}

std::uint16_t Z80::fetch_immediate16() {
    std::uint8_t lo = fetch_immediate();
    std::uint8_t hi = fetch_immediate();
    return static_cast<std::uint16_t>((hi << 8) | lo);
}

std::uint8_t Z80::io_in(std::uint16_t port) {
    cycles_ += bus_->on_m_cycle(port, 4);
    return bus_->read_io(port);
}

void Z80::io_out(std::uint16_t port, std::uint8_t value) {
    cycles_ += bus_->on_m_cycle(port, 4);
    bus_->write_io(port, value);
}

void Z80::push16(std::uint16_t value) {
    regs_.sp = static_cast<std::uint16_t>(regs_.sp - 1);
    write8(regs_.sp, static_cast<std::uint8_t>(value >> 8));
    regs_.sp = static_cast<std::uint16_t>(regs_.sp - 1);
    write8(regs_.sp, static_cast<std::uint8_t>(value & 0xFF));
}

std::uint16_t Z80::pop16() {
    std::uint8_t lo = read8(regs_.sp);
    regs_.sp = static_cast<std::uint16_t>(regs_.sp + 1);
    std::uint8_t hi = read8(regs_.sp);
    regs_.sp = static_cast<std::uint16_t>(regs_.sp + 1);
    return static_cast<std::uint16_t>((hi << 8) | lo);
}

void Z80::set_nmi_line(bool active) {
    if (active && !nmi_line_) {
        nmi_pending_ = true;
    }
    nmi_line_ = active;
}

void Z80::set_int_line(bool active) {
    int_line_ = active;
}

void Z80::pulse_int_line() noexcept {
    int_pulse_pending_ = true;
}

bool Z80::handle_nmi() {
    if (!nmi_pending_) {
        return false;
    }
    nmi_pending_ = false;
    regs_.halted = false;
    regs_.iff2 = regs_.iff1;
    regs_.iff1 = false;
    inc_r();
    cycles_ += bus_->on_m_cycle(regs_.pc, 5);
    push16(regs_.pc);
    regs_.pc = 0x0066;
    regs_.wz = 0x0066;
    cycles_ += 11;
    return true;
}

bool Z80::handle_int(bool int_active) {
    if (!regs_.iff1 || !int_active) {
        return false;
    }
    regs_.halted = false;
    regs_.iff1 = false;
    regs_.iff2 = false;
    inc_r();
    cycles_ += bus_->on_m_cycle(regs_.pc, 7);
    std::uint8_t databus = bus_->acknowledge_interrupt();
    push16(regs_.pc);
    int t = 0;
    switch (regs_.im) {
    case 0: {
        // Execute the data-bus opcode. Most often RST nn → vector to it.
        std::uint16_t target = ((databus & 0xC7) == 0xC7)
                                   ? static_cast<std::uint16_t>(databus & 0x38)
                                   : static_cast<std::uint16_t>(0x38);
        regs_.pc = target;
        regs_.wz = target;
        t = 13;
        break;
    }
    case 1:
        regs_.pc = 0x0038;
        regs_.wz = 0x0038;
        t = 13;
        break;
    case 2: {
        auto vector_addr = static_cast<std::uint16_t>((regs_.i << 8) | databus);
        std::uint8_t lo = read8(vector_addr);
        std::uint8_t hi = read8(static_cast<std::uint16_t>(vector_addr + 1));
        regs_.pc = static_cast<std::uint16_t>((hi << 8) | lo);
        regs_.wz = regs_.pc;
        t = 19;
        break;
    }
    default:
        t = 13;
        break;
    }
    cycles_ += static_cast<std::uint64_t>(t);
    return true;
}

void Z80::rst(std::uint16_t target) {
    push16(regs_.pc);
    regs_.pc = target;
    regs_.wz = target;
}

int Z80::step() {
    std::uint64_t start = cycles_;

    // NMI is edge-triggered and always serviced first if pending.
    if (nmi_pending_) {
        handle_nmi();
        return static_cast<int>(cycles_ - start);
    }

    // INT is serviced only when IFF1 is set and not directly after EI.
    const bool int_active = int_line_ || int_pulse_pending_;
    if (int_pulse_pending_) {
        int_pulse_pending_ = false;
    }

    if (int_active && regs_.iff1 && !regs_.ei_pending) {
        if (handle_int(int_active)) {
            return static_cast<int>(cycles_ - start);
        }
    }

    bool const was_ei_pending = regs_.ei_pending;
    regs_.ei_pending = false;

    if (regs_.halted) {
        // Single 4-T NOP-like cycle until interrupted.
        cycles_ += bus_->on_m_cycle(regs_.pc, 4);
        inc_r();
        cycles_ += 4;
        // EI right before HALT still clears the one-instruction delay.
        (void)was_ei_pending;
        return 4;
    }

    std::uint8_t op = fetch_opcode();
    int const t = dispatch_unprefixed(op);
    cycles_ += static_cast<std::uint64_t>(t);
    return static_cast<int>(cycles_ - start);
}

int Z80::run_for(int t_states) {
    int total = 0;
    while (total < t_states) {
        total += step();
    }
    return total;
}

Snapshot Z80::save_snapshot() const {
    Snapshot s;
    s.registers = regs_;
    s.cycle_counter = cycles_;
    s.nmi_line = nmi_line_;
    s.nmi_pending = nmi_pending_;
    s.int_line = int_line_;
    s.int_pulse_pending = int_pulse_pending_;
    return s;
}

void Z80::load_snapshot(const Snapshot& snapshot) {
    regs_ = snapshot.registers;
    cycles_ = snapshot.cycle_counter;
    nmi_line_ = snapshot.nmi_line;
    nmi_pending_ = snapshot.nmi_pending;
    int_line_ = snapshot.int_line;
    int_pulse_pending_ = snapshot.int_pulse_pending;
}

}  // namespace z80f
