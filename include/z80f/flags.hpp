#pragma once

#include <cstdint>

namespace z80f {

// Z80 F register layout:
//   bit 7  S  - sign
//   bit 6  Z  - zero
//   bit 5  Y  - undocumented copy of result bit 5
//   bit 4  H  - half-carry
//   bit 3  X  - undocumented copy of result bit 3
//   bit 2  P/V- parity / overflow
//   bit 1  N  - add/sub
//   bit 0  C  - carry
struct Flags {
    std::uint8_t bits = 0;

    static constexpr std::uint8_t S = 0x80;
    static constexpr std::uint8_t Z = 0x40;
    static constexpr std::uint8_t Y = 0x20;
    static constexpr std::uint8_t H = 0x10;
    static constexpr std::uint8_t X = 0x08;
    static constexpr std::uint8_t P = 0x04;
    static constexpr std::uint8_t V = 0x04;
    static constexpr std::uint8_t N = 0x02;
    static constexpr std::uint8_t C = 0x01;

    constexpr bool sign() const noexcept { return (bits & S) != 0; }
    constexpr bool zero() const noexcept { return (bits & Z) != 0; }
    constexpr bool y() const noexcept { return (bits & Y) != 0; }
    constexpr bool half_carry() const noexcept { return (bits & H) != 0; }
    constexpr bool x() const noexcept { return (bits & X) != 0; }
    constexpr bool parity_overflow() const noexcept { return (bits & P) != 0; }
    constexpr bool parity() const noexcept { return (bits & P) != 0; }
    constexpr bool overflow() const noexcept { return (bits & V) != 0; }
    constexpr bool add_sub() const noexcept { return (bits & N) != 0; }
    constexpr bool carry() const noexcept { return (bits & C) != 0; }

    constexpr void set_sign(bool v) noexcept { set_bit(S, v); }
    constexpr void set_zero(bool v) noexcept { set_bit(Z, v); }
    constexpr void set_y(bool v) noexcept { set_bit(Y, v); }
    constexpr void set_half_carry(bool v) noexcept { set_bit(H, v); }
    constexpr void set_x(bool v) noexcept { set_bit(X, v); }
    constexpr void set_parity_overflow(bool v) noexcept { set_bit(P, v); }
    constexpr void set_add_sub(bool v) noexcept { set_bit(N, v); }
    constexpr void set_carry(bool v) noexcept { set_bit(C, v); }

    constexpr void set_bit(std::uint8_t mask, bool v) noexcept {
        bits = static_cast<std::uint8_t>(v ? (bits | mask) : (bits & ~mask));
    }
};

}  // namespace z80f
