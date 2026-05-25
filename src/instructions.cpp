#include <z80f/z80.hpp>

#include <cstdint>

#include "internal.hpp"
#include "z80f/flags.hpp"

namespace z80f {

using detail::parity_of;
using detail::sz53_of;
using detail::sz53p_of;

namespace {

constexpr std::uint8_t sf = Flags::S;
constexpr std::uint8_t zf = Flags::Z;
constexpr std::uint8_t yf = Flags::Y;
constexpr std::uint8_t hf = Flags::H;
constexpr std::uint8_t xf = Flags::X;
constexpr std::uint8_t pf = Flags::P;
constexpr std::uint8_t nf = Flags::N;
constexpr std::uint8_t cf = Flags::C;

}  // namespace

void Z80::alu_add(std::uint8_t value, bool with_carry) {
    std::uint16_t c = with_carry && regs_.flags.carry() ? 1 : 0;
    std::uint16_t a = regs_.a;
    std::uint16_t sum = a + value + c;
    auto result = static_cast<std::uint8_t>(sum & 0xFF);
    std::uint8_t f = sz53_of(result);
    if (sum & 0x100) {
        f |= cf;
    }
    if (((a ^ value ^ sum) & 0x10) != 0) {
        f |= hf;
    }
    if ((~(a ^ value) & (a ^ sum) & 0x80) != 0) {
        f |= pf;
    }
    regs_.flags.bits = f;
    regs_.a = result;
}

void Z80::alu_sub(std::uint8_t value, bool with_carry) {
    std::uint16_t c = with_carry && regs_.flags.carry() ? 1 : 0;
    std::uint16_t a = regs_.a;
    std::uint16_t diff = a - value - c;
    auto result = static_cast<std::uint8_t>(diff & 0xFF);
    auto f = static_cast<std::uint8_t>(sz53_of(result) | nf);
    if (diff & 0x100) {
        f |= cf;
    }
    if (((a ^ value ^ diff) & 0x10) != 0) {
        f |= hf;
    }
    if (((a ^ value) & (a ^ diff) & 0x80) != 0) {
        f |= pf;
    }
    regs_.flags.bits = f;
    regs_.a = result;
}

void Z80::alu_cp(std::uint8_t value) {
    std::uint16_t a = regs_.a;
    std::uint16_t diff = a - value;
    auto result = static_cast<std::uint8_t>(diff & 0xFF);
    std::uint8_t f = nf;
    if (result & 0x80) {
        f |= sf;
    }
    if (result == 0) {
        f |= zf;
    }
    // CP uses operand bits (not result) for X/Y, per documented Z80 behavior.
    f |= static_cast<std::uint8_t>(value & (yf | xf));
    if (diff & 0x100) {
        f |= cf;
    }
    if (((a ^ value ^ diff) & 0x10) != 0) {
        f |= hf;
    }
    if (((a ^ value) & (a ^ diff) & 0x80) != 0) {
        f |= pf;
    }
    regs_.flags.bits = f;
}

void Z80::alu_and(std::uint8_t value) {
    regs_.a &= value;
    regs_.flags.bits = static_cast<std::uint8_t>(sz53p_of(regs_.a) | hf);
}

void Z80::alu_or(std::uint8_t value) {
    regs_.a |= value;
    regs_.flags.bits = sz53p_of(regs_.a);
}

void Z80::alu_xor(std::uint8_t value) {
    regs_.a ^= value;
    regs_.flags.bits = sz53p_of(regs_.a);
}

std::uint8_t Z80::alu_inc(std::uint8_t value) {
    auto result = static_cast<std::uint8_t>(value + 1);
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
    f |= sz53_of(result);
    if ((result & 0x0F) == 0) {
        f |= hf;
    }
    if (result == 0x80) {
        f |= pf;
    }
    regs_.flags.bits = f;
    return result;
}

std::uint8_t Z80::alu_dec(std::uint8_t value) {
    auto result = static_cast<std::uint8_t>(value - 1);
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
    f |= static_cast<std::uint8_t>(sz53_of(result) | nf);
    if ((result & 0x0F) == 0x0F) {
        f |= hf;
    }
    if (result == 0x7F) {
        f |= pf;
    }
    regs_.flags.bits = f;
    return result;
}

void Z80::alu_add16(std::uint16_t& dest, std::uint16_t value) {
    std::uint32_t a = dest;
    std::uint32_t sum = a + value;
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
    if (((a ^ value ^ sum) >> 8) & 0x10) {
        f |= hf;
    }
    if (sum & 0x10000) {
        f |= cf;
    }
    auto result = static_cast<std::uint16_t>(sum & 0xFFFF);
    f |= static_cast<std::uint8_t>((result >> 8) & (yf | xf));
    regs_.flags.bits = f;
    regs_.wz = static_cast<std::uint16_t>(a + 1);
    dest = result;
}

void Z80::alu_adc16(std::uint16_t value) {
    std::uint32_t hl = regs_.hl();
    std::uint32_t c = regs_.flags.carry() ? 1U : 0U;
    std::uint32_t sum = hl + value + c;
    auto result = static_cast<std::uint16_t>(sum & 0xFFFF);
    std::uint8_t f = 0;
    if (result & 0x8000) {
        f |= sf;
    }
    if (result == 0) {
        f |= zf;
    }
    if (((hl ^ value ^ sum) >> 8) & 0x10) {
        f |= hf;
    }
    if (((~(hl ^ value)) & (hl ^ sum) & 0x8000) != 0) {
        f |= pf;
    }
    if (sum & 0x10000) {
        f |= cf;
    }
    f |= static_cast<std::uint8_t>((result >> 8) & (yf | xf));
    regs_.flags.bits = f;
    regs_.wz = static_cast<std::uint16_t>(hl + 1);
    regs_.set_hl(result);
}

void Z80::alu_sbc16(std::uint16_t value) {
    std::uint32_t hl = regs_.hl();
    std::uint32_t c = regs_.flags.carry() ? 1U : 0U;
    std::uint32_t diff = hl - value - c;
    auto result = static_cast<std::uint16_t>(diff & 0xFFFF);
    std::uint8_t f = nf;
    if (result & 0x8000) {
        f |= sf;
    }
    if (result == 0) {
        f |= zf;
    }
    if (((hl ^ value ^ diff) >> 8) & 0x10) {
        f |= hf;
    }
    if (((hl ^ value) & (hl ^ diff) & 0x8000) != 0) {
        f |= pf;
    }
    if (diff & 0x10000) {
        f |= cf;
    }
    f |= static_cast<std::uint8_t>((result >> 8) & (yf | xf));
    regs_.flags.bits = f;
    regs_.wz = static_cast<std::uint16_t>(hl + 1);
    regs_.set_hl(result);
}

void Z80::alu_daa() {
    std::uint8_t a = regs_.a;
    std::uint8_t correction = 0;
    bool const carry = regs_.flags.carry();
    bool const half = regs_.flags.half_carry();
    bool const sub = regs_.flags.add_sub();

    if (half || (a & 0x0F) > 9) {
        correction |= 0x06;
    }
    if (carry || a > 0x99) {
        correction |= 0x60;
    }

    auto new_a = static_cast<std::uint8_t>(sub ? (a - correction) : (a + correction));
    std::uint8_t f = sz53p_of(new_a);
    if (sub) {
        f |= nf;
    }
    if (carry || a > 0x99) {
        f |= cf;
    }

    // H flag is bit 4 of (a ^ new_a)
    if ((a ^ new_a) & 0x10) {
        f |= hf;
    }

    regs_.flags.bits = f;
    regs_.a = new_a;
}

void Z80::alu_rlca() {
    std::uint8_t a = regs_.a;
    auto c = static_cast<std::uint8_t>((a >> 7) & 1);
    a = static_cast<std::uint8_t>((a << 1) | c);
    regs_.a = a;
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
    if (c) {
        f |= cf;
    }
    f |= static_cast<std::uint8_t>(a & (yf | xf));
    regs_.flags.bits = f;
}

void Z80::alu_rrca() {
    std::uint8_t a = regs_.a;
    auto c = static_cast<std::uint8_t>(a & 1);
    a = static_cast<std::uint8_t>((a >> 1) | (c << 7));
    regs_.a = a;
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
    if (c) {
        f |= cf;
    }
    f |= static_cast<std::uint8_t>(a & (yf | xf));
    regs_.flags.bits = f;
}

void Z80::alu_rla() {
    std::uint8_t a = regs_.a;
    std::uint8_t old_c = regs_.flags.carry() ? 1 : 0;
    auto new_c = static_cast<std::uint8_t>((a >> 7) & 1);
    a = static_cast<std::uint8_t>((a << 1) | old_c);
    regs_.a = a;
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
    if (new_c) {
        f |= cf;
    }
    f |= static_cast<std::uint8_t>(a & (yf | xf));
    regs_.flags.bits = f;
}

void Z80::alu_rra() {
    std::uint8_t a = regs_.a;
    std::uint8_t old_c = regs_.flags.carry() ? 1 : 0;
    auto new_c = static_cast<std::uint8_t>(a & 1);
    a = static_cast<std::uint8_t>((a >> 1) | (old_c << 7));
    regs_.a = a;
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
    if (new_c) {
        f |= cf;
    }
    f |= static_cast<std::uint8_t>(a & (yf | xf));
    regs_.flags.bits = f;
}

// ---- CB-prefix rotations/shifts ----

std::uint8_t Z80::cb_rlc(std::uint8_t v) {
    auto c = static_cast<std::uint8_t>((v >> 7) & 1);
    auto r = static_cast<std::uint8_t>((v << 1) | c);
    std::uint8_t f = sz53p_of(r);
    if (c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_rrc(std::uint8_t v) {
    auto c = static_cast<std::uint8_t>(v & 1);
    auto r = static_cast<std::uint8_t>((v >> 1) | (c << 7));
    std::uint8_t f = sz53p_of(r);
    if (c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_rl(std::uint8_t v) {
    std::uint8_t old_c = regs_.flags.carry() ? 1 : 0;
    auto new_c = static_cast<std::uint8_t>((v >> 7) & 1);
    auto r = static_cast<std::uint8_t>((v << 1) | old_c);
    std::uint8_t f = sz53p_of(r);
    if (new_c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_rr(std::uint8_t v) {
    std::uint8_t old_c = regs_.flags.carry() ? 1 : 0;
    auto new_c = static_cast<std::uint8_t>(v & 1);
    auto r = static_cast<std::uint8_t>((v >> 1) | (old_c << 7));
    std::uint8_t f = sz53p_of(r);
    if (new_c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_sla(std::uint8_t v) {
    auto c = static_cast<std::uint8_t>((v >> 7) & 1);
    auto r = static_cast<std::uint8_t>(v << 1);
    std::uint8_t f = sz53p_of(r);
    if (c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_sra(std::uint8_t v) {
    auto c = static_cast<std::uint8_t>(v & 1);
    auto r = static_cast<std::uint8_t>((v >> 1) | (v & 0x80));
    std::uint8_t f = sz53p_of(r);
    if (c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_sll(std::uint8_t v) {
    // Undocumented SLL/SL1: like SLA but sets bit 0.
    auto c = static_cast<std::uint8_t>((v >> 7) & 1);
    auto r = static_cast<std::uint8_t>((v << 1) | 1);
    std::uint8_t f = sz53p_of(r);
    if (c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

std::uint8_t Z80::cb_srl(std::uint8_t v) {
    auto c = static_cast<std::uint8_t>(v & 1);
    auto r = static_cast<std::uint8_t>(v >> 1);
    std::uint8_t f = sz53p_of(r);
    if (c) {
        f |= cf;
    }
    regs_.flags.bits = f;
    return r;
}

void Z80::cb_bit(std::uint8_t bit, std::uint8_t v, std::uint16_t internal_addr) {
    auto mask = static_cast<std::uint8_t>(1 << bit);
    auto result = static_cast<std::uint8_t>(v & mask);
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
    f |= hf;
    if (result == 0) {
        f |= (zf | pf);
    }
    if (bit == 7 && (result & 0x80)) {
        f |= sf;
    }
    // Y and X come from the high byte of the internal address for BIT n,(HL)
    // and BIT n,(IX+d) — and from the operand itself for register form.
    if (internal_addr != 0xFFFF) {
        f |= static_cast<std::uint8_t>((internal_addr >> 8) & (yf | xf));
    } else {
        f |= static_cast<std::uint8_t>(v & (yf | xf));
    }
    regs_.flags.bits = f;
}

std::uint8_t Z80::cb_set(std::uint8_t bit, std::uint8_t v) {
    return static_cast<std::uint8_t>(v | (1 << bit));
}

std::uint8_t Z80::cb_res(std::uint8_t bit, std::uint8_t v) {
    return static_cast<std::uint8_t>(v & ~(1 << bit));
}

// ---- Block instructions ----

void Z80::block_ld(int delta, bool repeat) {
    std::uint8_t byte = read8(regs_.hl());
    write8(regs_.de(), byte);
    cycles_ += bus_.on_m_cycle(regs_.de(), 2);  // 2 extra T-states
    regs_.set_hl(static_cast<std::uint16_t>(regs_.hl() + delta));
    regs_.set_de(static_cast<std::uint16_t>(regs_.de() + delta));
    regs_.set_bc(static_cast<std::uint16_t>(regs_.bc() - 1));
    auto n = static_cast<std::uint8_t>(byte + regs_.a);
    auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | cf));
    if (n & 0x02) {
        f |= yf;
    }
    if (n & 0x08) {
        f |= xf;
    }
    if (regs_.bc() != 0) {
        f |= pf;
    }
    regs_.flags.bits = f;
    if (repeat && regs_.bc() != 0) {
        regs_.pc = static_cast<std::uint16_t>(regs_.pc - 2);
        cycles_ += bus_.on_m_cycle(regs_.de(), 5);
        cycles_ += 5;
    }
}

void Z80::block_cp(int delta, bool repeat) {
    std::uint8_t byte = read8(regs_.hl());
    cycles_ += bus_.on_m_cycle(regs_.hl(), 5);
    std::uint16_t a = regs_.a;
    std::uint16_t diff = a - byte;
    auto result = static_cast<std::uint8_t>(diff & 0xFF);
    bool const half = ((a ^ byte ^ diff) & 0x10) != 0;
    regs_.set_hl(static_cast<std::uint16_t>(regs_.hl() + delta));
    regs_.set_bc(static_cast<std::uint16_t>(regs_.bc() - 1));
    std::uint8_t f = nf;
    f |= static_cast<std::uint8_t>(regs_.flags.bits & cf);
    if (result & 0x80) {
        f |= sf;
    }
    if (result == 0) {
        f |= zf;
    }
    if (half) {
        f |= hf;
    }
    if (regs_.bc() != 0) {
        f |= pf;
    }
    auto n = static_cast<std::uint8_t>(result - (half ? 1 : 0));
    if (n & 0x02) {
        f |= yf;
    }
    if (n & 0x08) {
        f |= xf;
    }
    regs_.flags.bits = f;
    if (repeat && regs_.bc() != 0 && result != 0) {
        regs_.pc = static_cast<std::uint16_t>(regs_.pc - 2);
        cycles_ += bus_.on_m_cycle(regs_.hl(), 5);
        cycles_ += 5;
    }
}

void Z80::block_in(int delta, bool repeat) {
    std::uint8_t byte = io_in(regs_.bc());
    regs_.b = static_cast<std::uint8_t>(regs_.b - 1);
    write8(regs_.hl(), byte);
    regs_.set_hl(static_cast<std::uint16_t>(regs_.hl() + delta));
    std::uint8_t f = sz53_of(regs_.b);
    if (byte & 0x80) {
        f |= nf;
    }
    regs_.flags.bits = f;
    if (repeat && regs_.b != 0) {
        regs_.pc = static_cast<std::uint16_t>(regs_.pc - 2);
        cycles_ += bus_.on_m_cycle(regs_.hl(), 5);
        cycles_ += 5;
    }
}

void Z80::block_out(int delta, bool repeat) {
    std::uint8_t byte = read8(regs_.hl());
    regs_.b = static_cast<std::uint8_t>(regs_.b - 1);
    io_out(regs_.bc(), byte);
    regs_.set_hl(static_cast<std::uint16_t>(regs_.hl() + delta));
    std::uint8_t f = sz53_of(regs_.b);
    if (byte & 0x80) {
        f |= nf;
    }
    regs_.flags.bits = f;
    if (repeat && regs_.b != 0) {
        regs_.pc = static_cast<std::uint16_t>(regs_.pc - 2);
        cycles_ += bus_.on_m_cycle(regs_.bc(), 5);
        cycles_ += 5;
    }
}

// ---- Branch helpers ----

void Z80::jr_cond(bool cond) {
    auto d = static_cast<std::int8_t>(fetch_immediate());
    if (cond) {
        regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
        regs_.wz = regs_.pc;
    }
}

void Z80::jp_cond(bool cond) {
    std::uint16_t target = fetch_immediate16();
    regs_.wz = target;
    if (cond) {
        regs_.pc = target;
    }
}

void Z80::call_cond(bool cond) {
    std::uint16_t target = fetch_immediate16();
    regs_.wz = target;
    if (cond) {
        push16(regs_.pc);
        regs_.pc = target;
    }
}

void Z80::ret_cond(bool cond) {
    if (cond) {
        regs_.pc = pop16();
        regs_.wz = regs_.pc;
    }
}

}  // namespace z80f
