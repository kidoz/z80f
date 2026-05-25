#include <z80f/z80.hpp>

#include <cstdint>

#include "internal.hpp"
#include "z80f/flags.hpp"
#include "z80f/registers.hpp"

namespace z80f {

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

// Read register r where r selects 0=B 1=C 2=D 3=E 4=H 5=L 6=(HL) 7=A
static inline std::uint8_t* reg8_ptr(Registers& r, int idx) {
    switch (idx) {
    case 0:
        return &r.b;
    case 1:
        return &r.c;
    case 2:
        return &r.d;
    case 3:
        return &r.e;
    case 4:
        return &r.h;
    case 5:
        return &r.l;
    case 7:
        return &r.a;
    default:
        return nullptr;  // 6 = (HL) is special-cased by callers
    }
}

int Z80::dispatch_unprefixed(std::uint8_t op) {
    int t = detail::base_t_for(op);

    switch (op) {
    // --- 0x00..0x3F: assorted ---
    case 0x00: /* NOP */
        break;
    case 0x01:
        regs_.set_bc(fetch_immediate16());
        break;
    case 0x02:
        write8(regs_.bc(), regs_.a);
        regs_.wz = static_cast<std::uint16_t>(((regs_.bc() + 1) & 0xFF) | (regs_.a << 8));
        break;
    case 0x03:
        regs_.set_bc(static_cast<std::uint16_t>(regs_.bc() + 1));
        break;
    case 0x04:
        regs_.b = alu_inc(regs_.b);
        break;
    case 0x05:
        regs_.b = alu_dec(regs_.b);
        break;
    case 0x06:
        regs_.b = fetch_immediate();
        break;
    case 0x07:
        alu_rlca();
        break;
    case 0x08: {
        std::uint16_t af = regs_.af();
        auto af2 = static_cast<std::uint16_t>((regs_.a_alt << 8) | regs_.f_alt);
        regs_.set_af(af2);
        regs_.a_alt = static_cast<std::uint8_t>(af >> 8);
        regs_.f_alt = static_cast<std::uint8_t>(af & 0xFF);
        break;
    }
    case 0x09: {
        std::uint16_t hl = regs_.hl();
        alu_add16(hl, regs_.bc());
        regs_.set_hl(hl);
        break;
    }
    case 0x0A:
        regs_.a = read8(regs_.bc());
        regs_.wz = static_cast<std::uint16_t>(regs_.bc() + 1);
        break;
    case 0x0B:
        regs_.set_bc(static_cast<std::uint16_t>(regs_.bc() - 1));
        break;
    case 0x0C:
        regs_.c = alu_inc(regs_.c);
        break;
    case 0x0D:
        regs_.c = alu_dec(regs_.c);
        break;
    case 0x0E:
        regs_.c = fetch_immediate();
        break;
    case 0x0F:
        alu_rrca();
        break;

    case 0x10: {  // DJNZ
        auto d = static_cast<std::int8_t>(fetch_immediate());
        regs_.b = static_cast<std::uint8_t>(regs_.b - 1);
        if (regs_.b != 0) {
            regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
            regs_.wz = regs_.pc;
            t = 13;
        } else {
            t = 8;
        }
        break;
    }
    case 0x11:
        regs_.set_de(fetch_immediate16());
        break;
    case 0x12:
        write8(regs_.de(), regs_.a);
        regs_.wz = static_cast<std::uint16_t>(((regs_.de() + 1) & 0xFF) | (regs_.a << 8));
        break;
    case 0x13:
        regs_.set_de(static_cast<std::uint16_t>(regs_.de() + 1));
        break;
    case 0x14:
        regs_.d = alu_inc(regs_.d);
        break;
    case 0x15:
        regs_.d = alu_dec(regs_.d);
        break;
    case 0x16:
        regs_.d = fetch_immediate();
        break;
    case 0x17:
        alu_rla();
        break;
    case 0x18: {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
        regs_.wz = regs_.pc;
        break;
    }
    case 0x19: {
        std::uint16_t hl = regs_.hl();
        alu_add16(hl, regs_.de());
        regs_.set_hl(hl);
        break;
    }
    case 0x1A:
        regs_.a = read8(regs_.de());
        regs_.wz = static_cast<std::uint16_t>(regs_.de() + 1);
        break;
    case 0x1B:
        regs_.set_de(static_cast<std::uint16_t>(regs_.de() - 1));
        break;
    case 0x1C:
        regs_.e = alu_inc(regs_.e);
        break;
    case 0x1D:
        regs_.e = alu_dec(regs_.e);
        break;
    case 0x1E:
        regs_.e = fetch_immediate();
        break;
    case 0x1F:
        alu_rra();
        break;

    case 0x20: {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        if (!regs_.flags.zero()) {
            regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
            regs_.wz = regs_.pc;
            t = 12;
        } else {
            t = 7;
        }
        break;
    }
    case 0x21:
        regs_.set_hl(fetch_immediate16());
        break;
    case 0x22: {
        std::uint16_t addr = fetch_immediate16();
        write8(addr, regs_.l);
        write8(static_cast<std::uint16_t>(addr + 1), regs_.h);
        regs_.wz = static_cast<std::uint16_t>(addr + 1);
        break;
    }
    case 0x23:
        regs_.set_hl(static_cast<std::uint16_t>(regs_.hl() + 1));
        break;
    case 0x24:
        regs_.h = alu_inc(regs_.h);
        break;
    case 0x25:
        regs_.h = alu_dec(regs_.h);
        break;
    case 0x26:
        regs_.h = fetch_immediate();
        break;
    case 0x27:
        alu_daa();
        break;
    case 0x28: {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        if (regs_.flags.zero()) {
            regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
            regs_.wz = regs_.pc;
            t = 12;
        } else {
            t = 7;
        }
        break;
    }
    case 0x29: {
        std::uint16_t hl = regs_.hl();
        alu_add16(hl, regs_.hl());
        regs_.set_hl(hl);
        break;
    }
    case 0x2A: {
        std::uint16_t addr = fetch_immediate16();
        regs_.l = read8(addr);
        regs_.h = read8(static_cast<std::uint16_t>(addr + 1));
        regs_.wz = static_cast<std::uint16_t>(addr + 1);
        break;
    }
    case 0x2B:
        regs_.set_hl(static_cast<std::uint16_t>(regs_.hl() - 1));
        break;
    case 0x2C:
        regs_.l = alu_inc(regs_.l);
        break;
    case 0x2D:
        regs_.l = alu_dec(regs_.l);
        break;
    case 0x2E:
        regs_.l = fetch_immediate();
        break;
    case 0x2F: {  // CPL
        regs_.a = static_cast<std::uint8_t>(~regs_.a);
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf | cf));
        f |= static_cast<std::uint8_t>(hf | nf);
        f |= static_cast<std::uint8_t>(regs_.a & (yf | xf));
        regs_.flags.bits = f;
        break;
    }

    case 0x30: {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        if (!regs_.flags.carry()) {
            regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
            regs_.wz = regs_.pc;
            t = 12;
        } else {
            t = 7;
        }
        break;
    }
    case 0x31:
        regs_.sp = fetch_immediate16();
        break;
    case 0x32: {
        std::uint16_t addr = fetch_immediate16();
        write8(addr, regs_.a);
        regs_.wz = static_cast<std::uint16_t>(((addr + 1) & 0xFF) | (regs_.a << 8));
        break;
    }
    case 0x33:
        regs_.sp = static_cast<std::uint16_t>(regs_.sp + 1);
        break;
    case 0x34: {
        std::uint8_t v = read8(regs_.hl());
        v = alu_inc(v);
        write8(regs_.hl(), v);
        break;
    }
    case 0x35: {
        std::uint8_t v = read8(regs_.hl());
        v = alu_dec(v);
        write8(regs_.hl(), v);
        break;
    }
    case 0x36: {
        std::uint8_t n = fetch_immediate();
        write8(regs_.hl(), n);
        break;
    }
    case 0x37: {  // SCF
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
        f |= cf;
        f |= static_cast<std::uint8_t>(regs_.a & (yf | xf));
        regs_.flags.bits = f;
        break;
    }
    case 0x38: {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        if (regs_.flags.carry()) {
            regs_.pc = static_cast<std::uint16_t>(regs_.pc + d);
            regs_.wz = regs_.pc;
            t = 12;
        } else {
            t = 7;
        }
        break;
    }
    case 0x39: {
        std::uint16_t hl = regs_.hl();
        alu_add16(hl, regs_.sp);
        regs_.set_hl(hl);
        break;
    }
    case 0x3A: {
        std::uint16_t addr = fetch_immediate16();
        regs_.a = read8(addr);
        regs_.wz = static_cast<std::uint16_t>(addr + 1);
        break;
    }
    case 0x3B:
        regs_.sp = static_cast<std::uint16_t>(regs_.sp - 1);
        break;
    case 0x3C:
        regs_.a = alu_inc(regs_.a);
        break;
    case 0x3D:
        regs_.a = alu_dec(regs_.a);
        break;
    case 0x3E:
        regs_.a = fetch_immediate();
        break;
    case 0x3F: {  // CCF
        bool const old_c = regs_.flags.carry();
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & (sf | zf | pf));
        if (old_c) {
            f |= hf;
        }
        if (!old_c) {
            f |= cf;
        }
        f |= static_cast<std::uint8_t>(regs_.a & (yf | xf));
        regs_.flags.bits = f;
        break;
    }

    // --- 0x40..0x7F: LD r,r' / LD r,(HL) / LD (HL),r / HALT ---
    case 0x76:
        regs_.halted = true;
        regs_.pc = static_cast<std::uint16_t>(regs_.pc - 1);
        break;
    default:
        if (op >= 0x40 && op <= 0x7F) {
            int const dst = (op >> 3) & 7;
            int const src = op & 7;
            std::uint8_t value = (src == 6) ? read8(regs_.hl()) : *reg8_ptr(regs_, src);
            if (dst == 6) {
                write8(regs_.hl(), value);
            } else {
                *reg8_ptr(regs_, dst) = value;
            }
        } else if (op >= 0x80 && op <= 0xBF) {
            int const alu = (op >> 3) & 7;
            int const src = op & 7;
            std::uint8_t v = (src == 6) ? read8(regs_.hl()) : *reg8_ptr(regs_, src);
            switch (alu) {
            case 0:
                alu_add(v, false);
                break;
            case 1:
                alu_add(v, true);
                break;
            case 2:
                alu_sub(v, false);
                break;
            case 3:
                alu_sub(v, true);
                break;
            case 4:
                alu_and(v);
                break;
            case 5:
                alu_xor(v);
                break;
            case 6:
                alu_or(v);
                break;
            case 7:
                alu_cp(v);
                break;
            }
        } else {
            // 0xC0..0xFF block
            switch (op) {
            case 0xC0:
                if (!regs_.flags.zero()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xC1:
                regs_.set_bc(pop16());
                break;
            case 0xC2:
                jp_cond(!regs_.flags.zero());
                break;
            case 0xC3:
                regs_.pc = fetch_immediate16();
                regs_.wz = regs_.pc;
                break;
            case 0xC4:
                if (!regs_.flags.zero()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xC5:
                push16(regs_.bc());
                break;
            case 0xC6:
                alu_add(fetch_immediate(), false);
                break;
            case 0xC7:
                rst(0x00);
                break;
            case 0xC8:
                if (regs_.flags.zero()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xC9:
                regs_.pc = pop16();
                regs_.wz = regs_.pc;
                break;
            case 0xCA:
                jp_cond(regs_.flags.zero());
                break;
            case 0xCB:
                t = dispatch_cb();
                break;
            case 0xCC:
                if (regs_.flags.zero()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xCD: {
                std::uint16_t target = fetch_immediate16();
                regs_.wz = target;
                push16(regs_.pc);
                regs_.pc = target;
                break;
            }
            case 0xCE:
                alu_add(fetch_immediate(), true);
                break;
            case 0xCF:
                rst(0x08);
                break;

            case 0xD0:
                if (!regs_.flags.carry()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xD1:
                regs_.set_de(pop16());
                break;
            case 0xD2:
                jp_cond(!regs_.flags.carry());
                break;
            case 0xD3: {
                std::uint8_t n = fetch_immediate();
                auto port = static_cast<std::uint16_t>((regs_.a << 8) | n);
                io_out(port, regs_.a);
                regs_.wz = static_cast<std::uint16_t>(((port + 1) & 0xFF) | (regs_.a << 8));
                break;
            }
            case 0xD4:
                if (!regs_.flags.carry()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xD5:
                push16(regs_.de());
                break;
            case 0xD6:
                alu_sub(fetch_immediate(), false);
                break;
            case 0xD7:
                rst(0x10);
                break;
            case 0xD8:
                if (regs_.flags.carry()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xD9: {  // EXX
                std::uint8_t tb = regs_.b;
                std::uint8_t tc = regs_.c;
                std::uint8_t td = regs_.d;
                std::uint8_t te = regs_.e;
                std::uint8_t th = regs_.h;
                std::uint8_t tl = regs_.l;
                regs_.b = regs_.b_alt;
                regs_.c = regs_.c_alt;
                regs_.d = regs_.d_alt;
                regs_.e = regs_.e_alt;
                regs_.h = regs_.h_alt;
                regs_.l = regs_.l_alt;
                regs_.b_alt = tb;
                regs_.c_alt = tc;
                regs_.d_alt = td;
                regs_.e_alt = te;
                regs_.h_alt = th;
                regs_.l_alt = tl;
                break;
            }
            case 0xDA:
                jp_cond(regs_.flags.carry());
                break;
            case 0xDB: {
                std::uint8_t n = fetch_immediate();
                auto port = static_cast<std::uint16_t>((regs_.a << 8) | n);
                regs_.a = io_in(port);
                regs_.wz = static_cast<std::uint16_t>(port + 1);
                break;
            }
            case 0xDC:
                if (regs_.flags.carry()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xDD:
                t = dispatch_index(regs_.ix, true);
                break;
            case 0xDE:
                alu_sub(fetch_immediate(), true);
                break;
            case 0xDF:
                rst(0x18);
                break;

            case 0xE0:
                if (!regs_.flags.parity_overflow()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xE1:
                regs_.set_hl(pop16());
                break;
            case 0xE2:
                jp_cond(!regs_.flags.parity_overflow());
                break;
            case 0xE3: {  // EX (SP),HL
                std::uint8_t lo = read8(regs_.sp);
                std::uint8_t hi = read8(static_cast<std::uint16_t>(regs_.sp + 1));
                write8(static_cast<std::uint16_t>(regs_.sp + 1), regs_.h);
                write8(regs_.sp, regs_.l);
                regs_.l = lo;
                regs_.h = hi;
                regs_.wz = regs_.hl();
                break;
            }
            case 0xE4:
                if (!regs_.flags.parity_overflow()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xE5:
                push16(regs_.hl());
                break;
            case 0xE6:
                alu_and(fetch_immediate());
                break;
            case 0xE7:
                rst(0x20);
                break;
            case 0xE8:
                if (regs_.flags.parity_overflow()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xE9:
                regs_.pc = regs_.hl();
                break;  // JP (HL)
            case 0xEA:
                jp_cond(regs_.flags.parity_overflow());
                break;
            case 0xEB: {  // EX DE,HL
                std::uint8_t td = regs_.d;
                std::uint8_t te = regs_.e;
                regs_.d = regs_.h;
                regs_.e = regs_.l;
                regs_.h = td;
                regs_.l = te;
                break;
            }
            case 0xEC:
                if (regs_.flags.parity_overflow()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xED:
                t = dispatch_ed();
                break;
            case 0xEE:
                alu_xor(fetch_immediate());
                break;
            case 0xEF:
                rst(0x28);
                break;

            case 0xF0:
                if (!regs_.flags.sign()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xF1:
                regs_.set_af(pop16());
                break;
            case 0xF2:
                jp_cond(!regs_.flags.sign());
                break;
            case 0xF3:
                regs_.iff1 = false;
                regs_.iff2 = false;
                break;  // DI
            case 0xF4:
                if (!regs_.flags.sign()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xF5:
                push16(regs_.af());
                break;
            case 0xF6:
                alu_or(fetch_immediate());
                break;
            case 0xF7:
                rst(0x30);
                break;
            case 0xF8:
                if (regs_.flags.sign()) {
                    regs_.pc = pop16();
                    regs_.wz = regs_.pc;
                    t = 11;
                } else {
                    t = 5;
                }
                break;
            case 0xF9:
                regs_.sp = regs_.hl();
                break;
            case 0xFA:
                jp_cond(regs_.flags.sign());
                break;
            case 0xFB:
                regs_.iff1 = true;
                regs_.iff2 = true;
                regs_.ei_pending = true;
                break;  // EI
            case 0xFC:
                if (regs_.flags.sign()) {
                    call_cond(true);
                    t = 17;
                } else {
                    (void)fetch_immediate16();
                    t = 10;
                }
                break;
            case 0xFD:
                t = dispatch_index(regs_.iy, false);
                break;
            case 0xFE:
                alu_cp(fetch_immediate());
                break;
            case 0xFF:
                rst(0x38);
                break;
            }
        }
        break;
    }
    return t;
}

// ---- CB prefix ----

int Z80::dispatch_cb() {
    std::uint8_t op = fetch_opcode();
    int const reg = op & 7;
    int const kind = (op >> 6) & 3;
    int const bit_or_op = (op >> 3) & 7;

    int t = 8;
    std::uint8_t value = 0;
    bool const is_hl = (reg == 6);

    if (is_hl) {
        value = read8(regs_.hl());
    } else {
        value = *reg8_ptr(regs_, reg);
    }

    if (kind == 0) {
        // rotate / shift
        switch (bit_or_op) {
        case 0:
            value = cb_rlc(value);
            break;
        case 1:
            value = cb_rrc(value);
            break;
        case 2:
            value = cb_rl(value);
            break;
        case 3:
            value = cb_rr(value);
            break;
        case 4:
            value = cb_sla(value);
            break;
        case 5:
            value = cb_sra(value);
            break;
        case 6:
            value = cb_sll(value);
            break;
        case 7:
            value = cb_srl(value);
            break;
        }
        if (is_hl) {
            write8(regs_.hl(), value);
            t = 15;
        } else {
            *reg8_ptr(regs_, reg) = value;
        }
    } else if (kind == 1) {
        // BIT
        cb_bit(static_cast<std::uint8_t>(bit_or_op), value,
               is_hl ? regs_.wz : static_cast<std::uint16_t>(0xFFFF));
        if (is_hl) {
            t = 12;
        }
    } else if (kind == 2) {
        // RES
        value = cb_res(static_cast<std::uint8_t>(bit_or_op), value);
        if (is_hl) {
            write8(regs_.hl(), value);
            t = 15;
        } else {
            *reg8_ptr(regs_, reg) = value;
        }
    } else {
        // SET
        value = cb_set(static_cast<std::uint8_t>(bit_or_op), value);
        if (is_hl) {
            write8(regs_.hl(), value);
            t = 15;
        } else {
            *reg8_ptr(regs_, reg) = value;
        }
    }
    return t;
}

// ---- ED prefix ----

int Z80::dispatch_ed() {
    std::uint8_t op = fetch_opcode();
    int t = 8;

    switch (op) {
    // IN r,(C) and OUT (C),r family — 0x40..0x7F
    case 0x40:
    case 0x48:
    case 0x50:
    case 0x58:
    case 0x60:
    case 0x68:
    case 0x70:
    case 0x78: {
        int const dst = (op >> 3) & 7;
        std::uint8_t v = io_in(regs_.bc());
        regs_.wz = static_cast<std::uint16_t>(regs_.bc() + 1);
        std::uint8_t f = detail::sz53p_of(v);
        f |= static_cast<std::uint8_t>(regs_.flags.bits & cf);
        regs_.flags.bits = f;
        if (dst != 6) {
            *reg8_ptr(regs_, dst) = v;
        }
        t = 12;
        break;
    }
    case 0x41:
    case 0x49:
    case 0x51:
    case 0x59:
    case 0x61:
    case 0x69:
    case 0x71:
    case 0x79: {
        int const src = (op >> 3) & 7;
        std::uint8_t v = (src == 6) ? 0 : *reg8_ptr(regs_, src);
        io_out(regs_.bc(), v);
        regs_.wz = static_cast<std::uint16_t>(regs_.bc() + 1);
        t = 12;
        break;
    }
    // SBC HL,rr / ADC HL,rr
    case 0x42:
        alu_sbc16(regs_.bc());
        t = 15;
        break;
    case 0x52:
        alu_sbc16(regs_.de());
        t = 15;
        break;
    case 0x62:
        alu_sbc16(regs_.hl());
        t = 15;
        break;
    case 0x72:
        alu_sbc16(regs_.sp);
        t = 15;
        break;
    case 0x4A:
        alu_adc16(regs_.bc());
        t = 15;
        break;
    case 0x5A:
        alu_adc16(regs_.de());
        t = 15;
        break;
    case 0x6A:
        alu_adc16(regs_.hl());
        t = 15;
        break;
    case 0x7A:
        alu_adc16(regs_.sp);
        t = 15;
        break;

    // LD (nn),rr / LD rr,(nn)
    case 0x43: {
        std::uint16_t a = fetch_immediate16();
        write8(a, regs_.c);
        write8(static_cast<std::uint16_t>(a + 1), regs_.b);
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x53: {
        std::uint16_t a = fetch_immediate16();
        write8(a, regs_.e);
        write8(static_cast<std::uint16_t>(a + 1), regs_.d);
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x63: {
        std::uint16_t a = fetch_immediate16();
        write8(a, regs_.l);
        write8(static_cast<std::uint16_t>(a + 1), regs_.h);
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x73: {
        std::uint16_t a = fetch_immediate16();
        write8(a, static_cast<std::uint8_t>(regs_.sp & 0xFF));
        write8(static_cast<std::uint16_t>(a + 1), static_cast<std::uint8_t>(regs_.sp >> 8));
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x4B: {
        std::uint16_t a = fetch_immediate16();
        regs_.c = read8(a);
        regs_.b = read8(static_cast<std::uint16_t>(a + 1));
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x5B: {
        std::uint16_t a = fetch_immediate16();
        regs_.e = read8(a);
        regs_.d = read8(static_cast<std::uint16_t>(a + 1));
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x6B: {
        std::uint16_t a = fetch_immediate16();
        regs_.l = read8(a);
        regs_.h = read8(static_cast<std::uint16_t>(a + 1));
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }
    case 0x7B: {
        std::uint16_t a = fetch_immediate16();
        std::uint8_t lo = read8(a);
        std::uint8_t hi = read8(static_cast<std::uint16_t>(a + 1));
        regs_.sp = static_cast<std::uint16_t>((hi << 8) | lo);
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t = 20;
        break;
    }

    // NEG
    case 0x44:
    case 0x4C:
    case 0x54:
    case 0x5C:
    case 0x64:
    case 0x6C:
    case 0x74:
    case 0x7C: {
        std::uint8_t a = regs_.a;
        auto r = static_cast<std::uint8_t>(0 - a);
        std::uint8_t f = nf;
        f |= detail::sz53_of(r);
        if (a != 0) {
            f |= cf;
        }
        if ((a & 0x0F) != 0) {
            f |= hf;
        }
        if (a == 0x80) {
            f |= pf;
        }
        regs_.a = r;
        regs_.flags.bits = f;
        break;
    }

    // RETN / RETI
    case 0x45:
    case 0x55:
    case 0x65:
    case 0x75:
    case 0x4D:
    case 0x5D:
    case 0x6D:
    case 0x7D:
        regs_.pc = pop16();
        regs_.wz = regs_.pc;
        regs_.iff1 = regs_.iff2;
        t = 14;
        break;

    // IM 0 / IM 1 / IM 2
    case 0x46:
    case 0x4E:
    case 0x66:
    case 0x6E:
        regs_.im = 0;
        break;
    case 0x56:
    case 0x76:
        regs_.im = 1;
        break;
    case 0x5E:
    case 0x7E:
        regs_.im = 2;
        break;

    // LD I,A / LD R,A / LD A,I / LD A,R
    case 0x47:
        regs_.i = regs_.a;
        t = 9;
        break;
    case 0x4F:
        regs_.r = static_cast<std::uint8_t>((regs_.r & 0x80) | (regs_.a & 0x7F));
        t = 9;
        break;
    case 0x57: {
        regs_.a = regs_.i;
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
        f |= detail::sz53_of(regs_.a);
        if (regs_.iff2) {
            f |= pf;
        }
        regs_.flags.bits = f;
        t = 9;
        break;
    }
    case 0x5F: {
        regs_.a = regs_.r;
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
        f |= detail::sz53_of(regs_.a);
        if (regs_.iff2) {
            f |= pf;
        }
        regs_.flags.bits = f;
        t = 9;
        break;
    }

    // RRD / RLD
    case 0x67: {  // RRD
        std::uint8_t m = read8(regs_.hl());
        auto new_m = static_cast<std::uint8_t>(((regs_.a & 0x0F) << 4) | (m >> 4));
        auto new_a = static_cast<std::uint8_t>((regs_.a & 0xF0) | (m & 0x0F));
        write8(regs_.hl(), new_m);
        regs_.a = new_a;
        regs_.wz = static_cast<std::uint16_t>(regs_.hl() + 1);
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
        f |= detail::sz53p_of(regs_.a);
        regs_.flags.bits = f;
        t = 18;
        break;
    }
    case 0x6F: {  // RLD
        std::uint8_t m = read8(regs_.hl());
        auto new_m = static_cast<std::uint8_t>(((m & 0x0F) << 4) | (regs_.a & 0x0F));
        auto new_a = static_cast<std::uint8_t>((regs_.a & 0xF0) | (m >> 4));
        write8(regs_.hl(), new_m);
        regs_.a = new_a;
        regs_.wz = static_cast<std::uint16_t>(regs_.hl() + 1);
        auto f = static_cast<std::uint8_t>(regs_.flags.bits & cf);
        f |= detail::sz53p_of(regs_.a);
        regs_.flags.bits = f;
        t = 18;
        break;
    }

    // Block ops
    case 0xA0:
        block_ld(+1, false);
        t = 16;
        break;
    case 0xA8:
        block_ld(-1, false);
        t = 16;
        break;
    case 0xB0:
        block_ld(+1, true);
        t = 16;
        break;
    case 0xB8:
        block_ld(-1, true);
        t = 16;
        break;
    case 0xA1:
        block_cp(+1, false);
        t = 16;
        break;
    case 0xA9:
        block_cp(-1, false);
        t = 16;
        break;
    case 0xB1:
        block_cp(+1, true);
        t = 16;
        break;
    case 0xB9:
        block_cp(-1, true);
        t = 16;
        break;
    case 0xA2:
        block_in(+1, false);
        t = 16;
        break;
    case 0xAA:
        block_in(-1, false);
        t = 16;
        break;
    case 0xB2:
        block_in(+1, true);
        t = 16;
        break;
    case 0xBA:
        block_in(-1, true);
        t = 16;
        break;
    case 0xA3:
        block_out(+1, false);
        t = 16;
        break;
    case 0xAB:
        block_out(-1, false);
        t = 16;
        break;
    case 0xB3:
        block_out(+1, true);
        t = 16;
        break;
    case 0xBB:
        block_out(-1, true);
        t = 16;
        break;

    default:
        // Unimplemented ED: 8 T (NOP-like).
        break;
    }

    return t;
}

// ---- DD/FD prefix ----

int Z80::dispatch_index(std::uint16_t& idx_ref, bool is_ix_ref) {
    std::uint16_t* current_idx = &idx_ref;
    bool is_ix = is_ix_ref;
    std::uint8_t op = fetch_opcode();
    int t = 4;  // DD/FD prefix cost

    // Consume consecutive DD/FD prefixes without recursion
    while (op == 0xDD || op == 0xFD) {
        is_ix = (op == 0xDD);
        current_idx = is_ix ? &regs_.ix : &regs_.iy;
        op = fetch_opcode();
        t += 4;
    }

    // We bind a local reference so the rest of the function operates on the final targeted index
    // register.
    std::uint16_t& idx = *current_idx;

    // Helpers for IX/IY high/low bytes
    auto ixh = [&] { return static_cast<std::uint8_t>(idx >> 8); };
    auto ixl = [&] { return static_cast<std::uint8_t>(idx & 0xFF); };
    auto set_ixh = [&](std::uint8_t v) {
        idx = static_cast<std::uint16_t>((v << 8) | (idx & 0xFF));
    };
    auto set_ixl = [&](std::uint8_t v) { idx = static_cast<std::uint16_t>((idx & 0xFF00) | v); };
    auto idx_addr = [&]() -> std::uint16_t {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        auto a = static_cast<std::uint16_t>(idx + d);
        regs_.wz = a;
        cycles_ += bus_->on_m_cycle(idx, 5);
        return a;
    };

    auto read_r = [&](int r) -> std::uint8_t {
        switch (r) {
        case 0:
            return regs_.b;
        case 1:
            return regs_.c;
        case 2:
            return regs_.d;
        case 3:
            return regs_.e;
        case 4:
            return ixh();
        case 5:
            return ixl();
        case 7:
            return regs_.a;
        default:
            return 0;
        }
    };
    auto write_r = [&](int r, std::uint8_t v) {
        switch (r) {
        case 0:
            regs_.b = v;
            break;
        case 1:
            regs_.c = v;
            break;
        case 2:
            regs_.d = v;
            break;
        case 3:
            regs_.e = v;
            break;
        case 4:
            set_ixh(v);
            break;
        case 5:
            set_ixl(v);
            break;
        case 7:
            regs_.a = v;
            break;
        }
    };

    switch (op) {
    case 0x09:
        alu_add16(idx, regs_.bc());
        t += 11;
        break;
    case 0x19:
        alu_add16(idx, regs_.de());
        t += 11;
        break;
    case 0x29:
        alu_add16(idx, idx);
        t += 11;
        break;
    case 0x39:
        alu_add16(idx, regs_.sp);
        t += 11;
        break;

    case 0x21:
        idx = fetch_immediate16();
        t += 10;
        break;
    case 0x22: {
        std::uint16_t a = fetch_immediate16();
        write8(a, static_cast<std::uint8_t>(idx & 0xFF));
        write8(static_cast<std::uint16_t>(a + 1), static_cast<std::uint8_t>(idx >> 8));
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t += 16;
        break;
    }
    case 0x2A: {
        std::uint16_t a = fetch_immediate16();
        std::uint8_t lo = read8(a);
        std::uint8_t hi = read8(static_cast<std::uint16_t>(a + 1));
        idx = static_cast<std::uint16_t>((hi << 8) | lo);
        regs_.wz = static_cast<std::uint16_t>(a + 1);
        t += 16;
        break;
    }
    case 0x23:
        idx = static_cast<std::uint16_t>(idx + 1);
        t += 6;
        break;
    case 0x2B:
        idx = static_cast<std::uint16_t>(idx - 1);
        t += 6;
        break;

    case 0x24:
        set_ixh(alu_inc(ixh()));
        t += 4;
        break;
    case 0x25:
        set_ixh(alu_dec(ixh()));
        t += 4;
        break;
    case 0x26:
        set_ixh(fetch_immediate());
        t += 7;
        break;
    case 0x2C:
        set_ixl(alu_inc(ixl()));
        t += 4;
        break;
    case 0x2D:
        set_ixl(alu_dec(ixl()));
        t += 4;
        break;
    case 0x2E:
        set_ixl(fetch_immediate());
        t += 7;
        break;

    case 0x34: {
        std::uint16_t a = idx_addr();
        std::uint8_t v = read8(a);
        v = alu_inc(v);
        write8(a, v);
        t += 19;
        break;
    }
    case 0x35: {
        std::uint16_t a = idx_addr();
        std::uint8_t v = read8(a);
        v = alu_dec(v);
        write8(a, v);
        t += 19;
        break;
    }
    case 0x36: {
        auto d = static_cast<std::int8_t>(fetch_immediate());
        std::uint8_t n = fetch_immediate();
        auto a = static_cast<std::uint16_t>(idx + d);
        regs_.wz = a;
        cycles_ += bus_->on_m_cycle(idx, 2);
        write8(a, n);
        t += 15;
        break;
    }

    // LD r,(IX+d) and LD (IX+d),r — pattern 0x46/0x4E/.../0x7E and 0x70..0x77
    case 0x46:
    case 0x4E:
    case 0x56:
    case 0x5E:
    case 0x66:
    case 0x6E:
    case 0x7E: {
        int const dst = (op >> 3) & 7;
        std::uint16_t a = idx_addr();
        std::uint8_t v = read8(a);
        // 0x66 / 0x6E load into H or L not IXH/IXL — they target real H/L
        switch (dst) {
        case 0:
            regs_.b = v;
            break;
        case 1:
            regs_.c = v;
            break;
        case 2:
            regs_.d = v;
            break;
        case 3:
            regs_.e = v;
            break;
        case 4:
            regs_.h = v;
            break;
        case 5:
            regs_.l = v;
            break;
        case 7:
            regs_.a = v;
            break;
        }
        t += 15;
        break;
    }
    case 0x70:
    case 0x71:
    case 0x72:
    case 0x73:
    case 0x74:
    case 0x75:
    case 0x77: {
        int const src = op & 7;
        std::uint16_t a = idx_addr();
        std::uint8_t v = 0;
        switch (src) {
        case 0:
            v = regs_.b;
            break;
        case 1:
            v = regs_.c;
            break;
        case 2:
            v = regs_.d;
            break;
        case 3:
            v = regs_.e;
            break;
        case 4:
            v = regs_.h;
            break;
        case 5:
            v = regs_.l;
            break;
        case 7:
            v = regs_.a;
            break;
        default:
            v = 0;
            break;
        }
        write8(a, v);
        t += 15;
        break;
    }

    // ALU A,(IX+d): 0x86, 0x8E, 0x96, 0x9E, 0xA6, 0xAE, 0xB6, 0xBE
    case 0x86: {
        std::uint16_t a = idx_addr();
        alu_add(read8(a), false);
        t += 15;
        break;
    }
    case 0x8E: {
        std::uint16_t a = idx_addr();
        alu_add(read8(a), true);
        t += 15;
        break;
    }
    case 0x96: {
        std::uint16_t a = idx_addr();
        alu_sub(read8(a), false);
        t += 15;
        break;
    }
    case 0x9E: {
        std::uint16_t a = idx_addr();
        alu_sub(read8(a), true);
        t += 15;
        break;
    }
    case 0xA6: {
        std::uint16_t a = idx_addr();
        alu_and(read8(a));
        t += 15;
        break;
    }
    case 0xAE: {
        std::uint16_t a = idx_addr();
        alu_xor(read8(a));
        t += 15;
        break;
    }
    case 0xB6: {
        std::uint16_t a = idx_addr();
        alu_or(read8(a));
        t += 15;
        break;
    }
    case 0xBE: {
        std::uint16_t a = idx_addr();
        alu_cp(read8(a));
        t += 15;
        break;
    }

    case 0xCB:
        t += dispatch_index_cb(idx);
        break;

    case 0xE1:
        idx = pop16();
        t += 10;
        break;
    case 0xE5:
        push16(idx);
        t += 11;
        break;
    case 0xE3: {  // EX (SP),IX
        std::uint8_t lo = read8(regs_.sp);
        std::uint8_t hi = read8(static_cast<std::uint16_t>(regs_.sp + 1));
        write8(static_cast<std::uint16_t>(regs_.sp + 1), static_cast<std::uint8_t>(idx >> 8));
        write8(regs_.sp, static_cast<std::uint8_t>(idx & 0xFF));
        idx = static_cast<std::uint16_t>((hi << 8) | lo);
        regs_.wz = idx;
        t += 19;
        break;
    }
    case 0xE9:
        regs_.pc = idx;
        t += 4;
        break;
    case 0xF9:
        regs_.sp = idx;
        t += 6;
        break;

    // LD r,r' with IXH/IXL substituting for H/L — common patterns 0x40..0x6F (excluding (HL)),
    // 0x60..0x6F replace H/L bytes. We handle a few useful cases:
    case 0x60:
        set_ixh(regs_.b);
        t += 4;
        break;
    case 0x61:
        set_ixh(regs_.c);
        t += 4;
        break;
    case 0x62:
        set_ixh(regs_.d);
        t += 4;
        break;
    case 0x63:
        set_ixh(regs_.e);
        t += 4;
        break;
    case 0x64:
        set_ixh(ixh());
        t += 4;
        break;
    case 0x65:
        set_ixh(ixl());
        t += 4;
        break;
    case 0x67:
        set_ixh(regs_.a);
        t += 4;
        break;
    case 0x68:
        set_ixl(regs_.b);
        t += 4;
        break;
    case 0x69:
        set_ixl(regs_.c);
        t += 4;
        break;
    case 0x6A:
        set_ixl(regs_.d);
        t += 4;
        break;
    case 0x6B:
        set_ixl(regs_.e);
        t += 4;
        break;
    case 0x6C:
        set_ixl(ixh());
        t += 4;
        break;
    case 0x6D:
        set_ixl(ixl());
        t += 4;
        break;
    case 0x6F:
        set_ixl(regs_.a);
        t += 4;
        break;

    // LD r,IXH/IXL for r ∈ {B,C,D,E,A}: opcodes 0x44/0x45/0x4C/0x4D/0x54/0x55/0x5C/0x5D/0x7C/0x7D
    case 0x44:
        regs_.b = ixh();
        t += 4;
        break;
    case 0x45:
        regs_.b = ixl();
        t += 4;
        break;
    case 0x4C:
        regs_.c = ixh();
        t += 4;
        break;
    case 0x4D:
        regs_.c = ixl();
        t += 4;
        break;
    case 0x54:
        regs_.d = ixh();
        t += 4;
        break;
    case 0x55:
        regs_.d = ixl();
        t += 4;
        break;
    case 0x5C:
        regs_.e = ixh();
        t += 4;
        break;
    case 0x5D:
        regs_.e = ixl();
        t += 4;
        break;
    case 0x7C:
        regs_.a = ixh();
        t += 4;
        break;
    case 0x7D:
        regs_.a = ixl();
        t += 4;
        break;

    // ALU with IXH/IXL — opcodes 0x84/0x85, 0x8C/0x8D, 0x94/0x95, 0x9C/0x9D, 0xA4/0xA5, 0xAC/0xAD,
    // 0xB4/0xB5, 0xBC/0xBD
    case 0x84:
        alu_add(ixh(), false);
        t += 4;
        break;
    case 0x85:
        alu_add(ixl(), false);
        t += 4;
        break;
    case 0x8C:
        alu_add(ixh(), true);
        t += 4;
        break;
    case 0x8D:
        alu_add(ixl(), true);
        t += 4;
        break;
    case 0x94:
        alu_sub(ixh(), false);
        t += 4;
        break;
    case 0x95:
        alu_sub(ixl(), false);
        t += 4;
        break;
    case 0x9C:
        alu_sub(ixh(), true);
        t += 4;
        break;
    case 0x9D:
        alu_sub(ixl(), true);
        t += 4;
        break;
    case 0xA4:
        alu_and(ixh());
        t += 4;
        break;
    case 0xA5:
        alu_and(ixl());
        t += 4;
        break;
    case 0xAC:
        alu_xor(ixh());
        t += 4;
        break;
    case 0xAD:
        alu_xor(ixl());
        t += 4;
        break;
    case 0xB4:
        alu_or(ixh());
        t += 4;
        break;
    case 0xB5:
        alu_or(ixl());
        t += 4;
        break;
    case 0xBC:
        alu_cp(ixh());
        t += 4;
        break;
    case 0xBD:
        alu_cp(ixl());
        t += 4;
        break;

    default:
        // Unhandled DD/FD: behaves like an unprefixed instruction with the
        // prefix essentially wasted. Re-dispatch as unprefixed.
        (void)read_r;
        (void)write_r;
        // Decrement R because the prefixed second opcode already inc'd it but
        // dispatch_unprefixed will also inc it; for now we leave as-is and
        // execute the same byte unprefixed at PC+0 — but we already consumed
        // it, so synthesize.
        t += dispatch_unprefixed(op);
        break;
    }

    return t;
}

// ---- DDCB / FDCB prefix ----

int Z80::dispatch_index_cb(std::uint16_t& idx) {
    auto d = static_cast<std::int8_t>(fetch_immediate());
    std::uint8_t op = fetch_immediate();  // not a opcode fetch — does NOT inc R
    auto a = static_cast<std::uint16_t>(idx + d);
    regs_.wz = a;
    cycles_ += bus_->on_m_cycle(idx, 2);

    int const reg = op & 7;
    int const kind = (op >> 6) & 3;
    int const bit_or_op = (op >> 3) & 7;

    std::uint8_t value = read8(a);

    if (kind == 0) {
        switch (bit_or_op) {
        case 0:
            value = cb_rlc(value);
            break;
        case 1:
            value = cb_rrc(value);
            break;
        case 2:
            value = cb_rl(value);
            break;
        case 3:
            value = cb_rr(value);
            break;
        case 4:
            value = cb_sla(value);
            break;
        case 5:
            value = cb_sra(value);
            break;
        case 6:
            value = cb_sll(value);
            break;
        case 7:
            value = cb_srl(value);
            break;
        }
        write8(a, value);
        if (reg != 6) {
            // Undocumented: also store the result in the named register.
            switch (reg) {
            case 0:
                regs_.b = value;
                break;
            case 1:
                regs_.c = value;
                break;
            case 2:
                regs_.d = value;
                break;
            case 3:
                regs_.e = value;
                break;
            case 4:
                regs_.h = value;
                break;
            case 5:
                regs_.l = value;
                break;
            case 7:
                regs_.a = value;
                break;
            }
        }
    } else if (kind == 1) {
        cb_bit(static_cast<std::uint8_t>(bit_or_op), value, a);
    } else if (kind == 2) {
        value = cb_res(static_cast<std::uint8_t>(bit_or_op), value);
        write8(a, value);
        if (reg != 6) {
            switch (reg) {
            case 0:
                regs_.b = value;
                break;
            case 1:
                regs_.c = value;
                break;
            case 2:
                regs_.d = value;
                break;
            case 3:
                regs_.e = value;
                break;
            case 4:
                regs_.h = value;
                break;
            case 5:
                regs_.l = value;
                break;
            case 7:
                regs_.a = value;
                break;
            }
        }
    } else {
        value = cb_set(static_cast<std::uint8_t>(bit_or_op), value);
        write8(a, value);
        if (reg != 6) {
            switch (reg) {
            case 0:
                regs_.b = value;
                break;
            case 1:
                regs_.c = value;
                break;
            case 2:
                regs_.d = value;
                break;
            case 3:
                regs_.e = value;
                break;
            case 4:
                regs_.h = value;
                break;
            case 5:
                regs_.l = value;
                break;
            case 7:
                regs_.a = value;
                break;
            }
        }
    }

    return (kind == 1) ? 16 : 19;
}

}  // namespace z80f
