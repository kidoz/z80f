#include <catch2/catch_test_macros.hpp>

#include "test_cpm.hpp"

TEST_CASE("ZEXALL compliance test", "[zexall][compliance]") {
    const auto result = z80f::test::run_cpm_program("tests/roms/zexall.cim", 100000000000ULL);
    if (!result.loaded) {
        SKIP("tests/roms/zexall.cim not found or invalid, skipping compliance test");
    }
    REQUIRE_FALSE(result.timed_out);
    REQUIRE(!result.output.contains("ERROR"));
    REQUIRE(result.output.contains("Tests complete"));
}
