#pragma once

#include <z80f/flags.hpp>

#include <cstdint>

namespace z80f {

struct Registers {
    // Main set (A and F kept separate; flags wraps F bits)
    std::uint8_t a = 0;
    Flags flags{};

    std::uint8_t b = 0;
    std::uint8_t c = 0;
    std::uint8_t d = 0;
    std::uint8_t e = 0;
    std::uint8_t h = 0;
    std::uint8_t l = 0;

    // Shadow set (AF'/BC'/DE'/HL') stored as raw bytes
    std::uint8_t a_alt = 0;
    std::uint8_t f_alt = 0;
    std::uint8_t b_alt = 0;
    std::uint8_t c_alt = 0;
    std::uint8_t d_alt = 0;
    std::uint8_t e_alt = 0;
    std::uint8_t h_alt = 0;
    std::uint8_t l_alt = 0;

    // Index registers
    std::uint16_t ix = 0;
    std::uint16_t iy = 0;

    // Special purpose
    std::uint16_t sp = 0;
    std::uint16_t pc = 0;
    std::uint16_t wz = 0;  // MEMPTR
    std::uint8_t i = 0;
    std::uint8_t r = 0;

    // Interrupt state
    bool iff1 = false;
    bool iff2 = false;
    std::uint8_t im = 0;  // 0, 1, or 2
    bool halted = false;
    bool ei_pending = false;  // EI delays interrupts by one instruction

    // 16-bit composite accessors
    constexpr std::uint16_t af() const noexcept {
        return static_cast<std::uint16_t>((a << 8) | flags.bits);
    }
    constexpr std::uint16_t bc() const noexcept { return static_cast<std::uint16_t>((b << 8) | c); }
    constexpr std::uint16_t de() const noexcept { return static_cast<std::uint16_t>((d << 8) | e); }
    constexpr std::uint16_t hl() const noexcept { return static_cast<std::uint16_t>((h << 8) | l); }

    constexpr void set_af(std::uint16_t v) noexcept {
        a = static_cast<std::uint8_t>(v >> 8);
        flags.bits = static_cast<std::uint8_t>(v & 0xFF);
    }
    constexpr void set_bc(std::uint16_t v) noexcept {
        b = static_cast<std::uint8_t>(v >> 8);
        c = static_cast<std::uint8_t>(v & 0xFF);
    }
    constexpr void set_de(std::uint16_t v) noexcept {
        d = static_cast<std::uint8_t>(v >> 8);
        e = static_cast<std::uint8_t>(v & 0xFF);
    }
    constexpr void set_hl(std::uint16_t v) noexcept {
        h = static_cast<std::uint8_t>(v >> 8);
        l = static_cast<std::uint8_t>(v & 0xFF);
    }

    constexpr std::uint8_t ixh() const noexcept { return static_cast<std::uint8_t>(ix >> 8); }
    constexpr std::uint8_t ixl() const noexcept { return static_cast<std::uint8_t>(ix & 0xFF); }
    constexpr std::uint8_t iyh() const noexcept { return static_cast<std::uint8_t>(iy >> 8); }
    constexpr std::uint8_t iyl() const noexcept { return static_cast<std::uint8_t>(iy & 0xFF); }
};

}  // namespace z80f
