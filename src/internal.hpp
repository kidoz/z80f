#pragma once

#include <cstdint>

namespace z80f::detail {

std::uint8_t parity_of(std::uint8_t v);
std::uint8_t sz53_of(std::uint8_t v);
std::uint8_t sz53p_of(std::uint8_t v);

std::uint8_t base_t_for(std::uint8_t op);

}  // namespace z80f::detail
