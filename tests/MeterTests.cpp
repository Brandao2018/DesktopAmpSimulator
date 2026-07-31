// Meter math: dB conversion, peak/RMS measurement, and smoothing ballistics.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <vector>

#include "Shared/Constants.h"
#include "Shared/MeterProcessor.h"
#include "Shared/ThreadSafeBuffer.h"

TEST_CASE("gainToDb converts reference points correctly", "[meter]")
{
    REQUIRE(ampsim::gainToDb(1.0f)  == Catch::Approx(0.0f).margin(0.001f));
    REQUIRE(ampsim::gainToDb(0.5f)  == Catch::Approx(-6.0206f).margin(0.01f));
    REQUIRE(ampsim::gainToDb(2.0f)  == Catch::Approx(6.0206f).margin(0.01f));
    REQUIRE(ampsim::gainToDb(0.1f)  == Catch::Approx(-20.0f).margin(0.01f));
}

TEST_CASE("gainToDb clamps silence and negatives to the floor", "[meter]")
{
    REQUIRE(ampsim::gainToDb(0.0f)     == ampsim::kMeterFloorDb);
    REQUIRE(ampsim::gainToDb(-1.0f)    == ampsim::kMeterFloorDb);
    REQUIRE(ampsim::gainToDb(1.0e-30f) == ampsim::kMeterFloorDb);
}

TEST_CASE("channelPeak finds the largest absolute sample", "[meter]")
{
    const std::vector<float> samples { 0.1f, -0.8f, 0.3f, 0.5f };
    REQUIRE(ampsim::channelPeak(samples.data(), samples.size())
            == Catch::Approx(0.8f).margin(0.0001f));
}

TEST_CASE("channelRms of a DC signal equals its magnitude", "[meter]")
{
    const std::vector<float> dc(128, 0.25f);
    REQUIRE(ampsim::channelRms(dc.data(), dc.size())
            == Catch::Approx(0.25f).margin(0.0001f));
}

TEST_CASE("channelRms of empty input is zero", "[meter]")
{
    REQUIRE(ampsim::channelRms(nullptr, 0) == 0.0f);
}

TEST_CASE("Smoothed level attacks instantly and releases gradually", "[meter]")
{
    const float coeff = ampsim::smoothingCoefficient(ampsim::kMeterSmoothingMs, 48000.0, 256);
    REQUIRE(coeff > 0.0f);
    REQUIRE(coeff < 1.0f);

    // Attack: rising input jumps immediately.
    float level = -60.0f;
    level = ampsim::updateSmoothedLevel(level, -6.0f, coeff);
    REQUIRE(level == Catch::Approx(-6.0f));

    // Release: falling input decays smoothly, staying above the target.
    level = ampsim::updateSmoothedLevel(level, -60.0f, coeff);
    REQUIRE(level < -6.0f);
    REQUIRE(level > -60.0f);
}

TEST_CASE("MeterExchange stores and resets levels atomically", "[meter]")
{
    ampsim::MeterExchange exchange;

    exchange.input.left.store(-3.0f, -9.0f);
    REQUIRE(exchange.input.left.loadPeak() == Catch::Approx(-3.0f));
    REQUIRE(exchange.input.left.loadRms()  == Catch::Approx(-9.0f));

    exchange.reset();
    REQUIRE(exchange.input.left.loadPeak() == ampsim::kMeterFloorDb);
    REQUIRE(exchange.input.left.loadRms()  == ampsim::kMeterFloorDb);
}
