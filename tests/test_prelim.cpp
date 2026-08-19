#include <catch2/catch_test_macros.hpp>

#include "test_cpm.hpp"

TEST_CASE("Preliminary exerciser smoke test", "[prelim][compliance]") {
    const auto result = z80f::test::run_cpm_program("tests/roms/prelim.com", 1000000000ULL);
    if (!result.loaded) {
        SKIP("tests/roms/prelim.com not found or invalid, skipping smoke test");
    }
    REQUIRE_FALSE(result.timed_out);
    REQUIRE(result.output.contains("Preliminary tests complete"));
}
