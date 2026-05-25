#include <z80f/flags.hpp>

#include <array>
#include <cstdint>

#include "internal.hpp"

namespace z80f::detail {

namespace {

constexpr std::array<std::uint8_t, 256> build_parity_table() {
    std::array<std::uint8_t, 256> t{};
    for (int i = 0; i < 256; ++i) {
        int p = 1;
        for (int b = 0; b < 8; ++b) {
            if (i & (1 << b)) {
                p ^= 1;
            }
        }
        t[i] = static_cast<std::uint8_t>(p ? Flags::P : 0);
    }
    return t;
}

constexpr std::array<std::uint8_t, 256> build_sz_table() {
    std::array<std::uint8_t, 256> t{};
    for (int i = 0; i < 256; ++i) {
        std::uint8_t f = 0;
        if (i == 0) {
            f |= Flags::Z;
        }
        if (i & 0x80) {
            f |= Flags::S;
        }
        f |= static_cast<std::uint8_t>(i) & (Flags::Y | Flags::X);
        t[i] = f;
    }
    return t;
}

}  // namespace

constexpr auto parity_table = build_parity_table();
constexpr auto sz_table = build_sz_table();

std::uint8_t parity_of(std::uint8_t v) {
    return parity_table[v];
}

std::uint8_t sz53_of(std::uint8_t v) {
    return sz_table[v];
}

std::uint8_t sz53p_of(std::uint8_t v) {
    return static_cast<std::uint8_t>(sz_table[v] | parity_table[v]);
}

}  // namespace z80f::detail
