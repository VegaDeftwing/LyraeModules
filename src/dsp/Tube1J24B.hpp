#pragma once
#include <rack.hpp>
namespace aladfar {
// Compact 1Zh24B rod-pentode approximation, in volts and milliamperes.
// Fitted by inspection to the published Ua curves at Ug2=45 V, Ug1=-1/0/+1 V.
// Source/provenance, curve checkpoints, and limitations: docs/Aladfar-model.md.
// The 3/2 current law is screen-controlled; the plate knee diverts current to G2.
// This is not a fitted full Koren parameter set or a model of a particular sample.
struct Tube1J24B {
    // Capacitances are in farads; all current-returning functions use mA.
    static constexpr float inputCapacitance = 3.6e-12f;
    static constexpr float outputCapacitance = 2.95e-12f;
    static constexpr float reverseCapacitance = .008e-12f;

    // Screen/control-grid emission before the plate-voltage knee.
    static rack::simd::float_4 emission(rack::simd::float_4 grid, rack::simd::float_4 screen) {
        namespace s = rack::simd;
        auto effective = grid + s::fmax(screen, 0.f) / (45.f / 1.65f);
        auto positive = s::fmax(effective, 0.f);
        auto current = (.88f / (1.65f * std::sqrt(1.65f))) * positive * s::sqrt(positive);
        // Smooth emission limiting near the 1.6 mA cathode rating, rather than
        // assuming that increasing positive grid voltage gives unlimited current.
        if (!s::movemask(current > 1.2f))
            return current;
        auto excess = s::clamp((current - 1.2f) / .4f, 0.f, 3.f);
        auto x2 = excess * excess;
        auto limited = 1.2f + .4f * excess * (27.f + x2) / (27.f + 9.f * x2);
        return s::ifelse(current > 1.2f, limited, current);
    }

    // Rational plate knee: current tends to zero as plate voltage falls.
    static rack::simd::float_4 plateCurrent(rack::simd::float_4 grid, rack::simd::float_4 plate,
                                            rack::simd::float_4 screen) {
        plate = rack::simd::fmax(plate, 0.f);
        return emission(grid, screen) * plate / (plate + 4.f);
    }

    // Approximate screen-current diversion, not a full measured surface fit.
    static rack::simd::float_4 screenCurrent(rack::simd::float_4 grid, rack::simd::float_4 plate,
                                             rack::simd::float_4 screen) {
        namespace s = rack::simd;
        auto emitted = emission(grid, screen);
        auto p = s::fmax(plate, 0.f);
        auto relativeScreen = s::fmax(screen, 0.f) / 45.f;
        return .04f * relativeScreen * s::sqrt(relativeScreen) +
               .45f * emitted * (4.f / (p + 4.f)) * (1.f + 1.2f * s::fmax(grid, 0.f));
    }

    // Exact positive solution of Va = B+ - Ra * Ia for our rational plate knee.
    // Ra is expressed in kilo-ohms because Ia is in milliamperes.
    static rack::simd::float_4 loadedPlate(rack::simd::float_4 grid, rack::simd::float_4 screen,
                                           float supply = 12.f, float resistanceK = 82.f) {
        auto b = 4.f + resistanceK * emission(grid, screen) - supply;
        return (8.f * supply) / (b + rack::simd::sqrt(b * b + 16.f * supply));
    }
};
} // namespace aladfar
