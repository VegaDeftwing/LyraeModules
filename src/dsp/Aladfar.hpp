#pragma once
#include <rack.hpp>
#include <array>
#include "Tube1J24B.hpp"

namespace aladfar {
using rack::simd::float_4;
namespace simd = rack::simd;

// Behavioral model of alternating ramp cores and an asymmetric, AC-coupled
// saturation path. No claim of matching undisclosed hardware component values.
inline float_4 clip(float_4 x) {
    x = simd::clamp(x, -3.f, 3.f);
    auto x2 = x * x;
    return x * (27.f + x2) / (27.f + 9.f * x2);
}

// Output-buffer rail compression is performed before decimation. A final
// host-rate hard safety clamp must not be the source of aggressive harmonics.
inline float_4 outputBuffer(float_4 volts) {
    auto a = simd::abs(volts);
    auto compressed = 8.f + 1.5f * clip((a - 8.f) / 1.5f);
    auto limited = simd::ifelse(volts < 0.f, -compressed, compressed);
    return simd::ifelse(a > 8.f, limited, volts);
}

// First-order antiderivative antialiasing for the rational soft clip, including
// its flat outer branches. The folding stages integrate their complete transfer
// over each input interval, rather than merely evaluating a clipped sample.
inline float_4 clipAverage(float_4 x, float_4 y) {
    auto a = simd::clamp(x, -3.f, 3.f), b = simd::clamp(y, -3.f, 3.f);
    auto sum = a + b;
    auto r = (a - b) * sum / (b * b + 3.f);
    auto z = r / (2.f + r);
    auto z2 = z * z;
    // log(1+r)/r via atanh's even series. |z|<=.6 on this domain;
    // twelve terms avoid cancellation when the two inputs almost coincide.
    float_4 polynomial = 1.f / 23.f;
    for (int d = 21; d >= 3; d -= 2)
        polynomial = 1.f / d + z2 * polynomial;
    polynomial = 1.f + z2 * polynomial;
    auto average = sum / 18.f + (4.f / 3.f) * sum / (b * b + 3.f) * (2.f / (2.f + r)) * polynomial;
    if (!simd::movemask((simd::abs(x) > 3.f) | (simd::abs(y) > 3.f)))
        return average;
    auto low = simd::fmin(x, y), high = simd::fmax(x, y);
    auto width = high - low;
    auto left = simd::fmax(simd::fmin(high, -3.f) - low, 0.f);
    auto right = simd::fmax(high - simd::fmax(low, 3.f), 0.f);
    return simd::ifelse(width < 1e-6f, clip(.5f * (x + y)),
                        (average * simd::abs(a - b) + right - left) / simd::fmax(width, 1e-6f));
}

struct FoldStage {
    float_4 previous = 0.f;
    bool initialized = false;
    void reset() {
        previous = 0.f;
        initialized = false;
    }
    float_4 process(float_4 x) {
        if (!initialized) {
            previous = x;
            initialized = true;
        }
        auto out = .7f * (x + previous) - 2.2f * clipAverage(1.6f * x, 1.6f * previous);
        previous = x;
        return out;
    }
};

inline float_4 wrap(float_4 p) {
    return p - simd::floor(p);
}

inline float_4 blep(float_4 p, float_4 dt) {
    auto left = p / dt;
    auto right = (p - 1.f) / dt;
    return simd::ifelse(p < dt, 2.f * left - left * left - 1.f,
                        simd::ifelse(p > 1.f - dt, right * right + 2.f * right + 1.f, 0.f));
}

inline float_4 blamp(float_4 p, float_4 dt) {
    auto distance = simd::fmin(p, 1.f - p);
    auto t = simd::fmax(1.f - distance / dt, 0.f);
    return dt * t * t * t / 6.f;
}

// One host-frame target per lane. Frequency is Hz; sync is the level-scaled
// input in volts. Offset/balance/resonance/drive are normalized 0..1, while
// pwm/shape are -1..1. Bank::process interpolates these targets internally.
struct Controls {
    float_4 frequency = 261.625565f, offset = .5f, balance = 0.f;
    float_4 pwm = 0.f, resonance = 0.f, drive = .35f, shape = 0.f, sync = 0.f;
};

inline Controls interpolate(const Controls& a, const Controls& b, float t) {
    Controls c;
#define ALADFAR_LERP(field) c.field = a.field + t * (b.field - a.field)
    ALADFAR_LERP(frequency);
    ALADFAR_LERP(offset);
    ALADFAR_LERP(balance);
    ALADFAR_LERP(pwm);
    ALADFAR_LERP(resonance);
    ALADFAR_LERP(drive);
    ALADFAR_LERP(shape);
    ALADFAR_LERP(sync);
#undef ALADFAR_LERP
    return c;
}

inline float_4 core(float_4 phase, float_4 offset, float_4 balance) {
    auto u = wrap(2.f * phase);
    auto shift = offset - .5f * balance;
    auto first = u - 2.f * simd::fmax(u - (1.f - balance), 0.f);
    return simd::ifelse(phase < .5f, first - shift, u + shift) - .5f;
}

inline float_4 bandlimitedCore(float_4 phase, float_4 dt, float_4 offset, float_4 balance) {
    auto half = wrap(phase + .5f);
    auto turn = wrap(phase - .5f * (1.f - balance));
    return core(phase, offset, balance) + .5f * (-1.f - 2.f * offset + balance) * blep(phase, dt) +
           .5f * (2.f * offset - 1.f + balance) * blep(half, dt) - 4.f * blamp(turn, dt) +
           4.f * blamp(half, dt);
}

// Four voices share SIMD arithmetic, but every state and correction is lane-local.
// Feedback, comparator and saturation run at Oversample times the host rate;
// folding runs at twice that rate. Defaults are 8x/16x with 256-tap output FIRs.
// Antialias=false is used only by the naive reference in the spectral tests.
// BLEP/BLAMP handle core steps/corners; minBLEP handles sync/comparator edges.
template <int Oversample = 8, int Quality = 32, bool Antialias = true> struct Bank {
    // phase is the active timing capacitor charge plus the core-selection bit:
    // [0,.5) is core A charging, [.5,1) is core B. It is integrated from
    // conduction current, not advanced by an independent ideal phase clock.
    float_4 phase = 0.f;
    // Double-precision phase is used only when float increments would stall.
    double lowPhase[4] = {};
    bool lowActive[4] = {};
    // Per-lane pitch compensation. Integrals/debt are measured in cycles;
    // fullCycle excludes the partial cycle immediately following external sync.
    float_4 tuningGain = 1.f;
    double referenceCycles[4] = {}, observedCycles[4] = {}, phaseDebt[4] = {};
    bool fullCycle[4] = {true, true, true, true};
    // Coupling-capacitor states use normalized return-signal units. Screen
    // voltage is in volts; recovery is normalized inactive-core charge.
    float_4 gridCapA = 0.f, gridCapB = 0.f, screenVoltage = 10.f;
    float_4 recoveryA = 0.f, recoveryB = 0.f;
    // Last conduction branch, relative to the nominal plate-current reference.
    float_4 conductionA = 1.f, conductionB = 1.f;
    // Conditioning, tube input, and DC-removal histories.
    float_4 colorLow = 0.f, tubeGrid = 0.f, dcRaw = 0.f, dcSmooth = 0.f, dcFold = 0.f;
    float_4 previousComparator = 0.f;
    // Schmitt latches are numeric 0/1, not SIMD all-bits-set masks.
    float_4 positive = 0.f, negative = 0.f;
    // Folder histories run at twice the main internal rate.
    std::array<FoldStage, 4> foldStages;
    rack::dsp::Decimator<2, 8, float_4> foldDecimator;
    float_4 previousSmooth = 0.f;
    // Previous host-frame target for interpolation; seeded on first process.
    Controls previous;
    bool initialized = false;
    bool pulseWasConnected = false;
    rack::dsp::MinBlepGenerator<16, 16, float_4> syncBlep, pulseBlep;
    std::array<rack::dsp::Decimator<Oversample, Quality, float_4>, 4> decimators;
    // Output order matches the Rack adapter: Raw, Pulse, Smooth, Fold.
    std::array<std::array<float_4, Oversample>, 4> buffer;

    void reset() {
        for (auto& active : lowActive)
            active = false;
        tuningGain = 1.f;
        for (int lane = 0; lane < 4; ++lane) {
            referenceCycles[lane] = observedCycles[lane] = phaseDebt[lane] = 0.;
            fullCycle[lane] = true;
        }
        phase = colorLow = tubeGrid = gridCapA = gridCapB = recoveryA = recoveryB = 0.f;
        screenVoltage = 10.f;
        conductionA = conductionB = 1.f;
        dcRaw = dcSmooth = dcFold = previousComparator = positive = negative = 0.f;
        initialized = false;
        pulseWasConnected = false;
        for (auto& d : decimators)
            d.reset();
        for (auto& stage : foldStages)
            stage.reset();
        foldDecimator.reset();
        previousSmooth = 0.f;
        for (auto* b : {&syncBlep, &pulseBlep}) {
            b->pos = 0;
            for (auto& x : b->buf)
                x = 0.f;
        }
    }

    // Charging scale/output plus return-path values needed by the RC solver.
    struct CircuitResult {
        float_4 current, output, sourceA, sourceB, gridA, gridB, screenLoad, conduction;
    };

    CircuitResult circuit(float_4 p, float_4 capA, float_4 capB, float_4 screen, float_4 recoverA,
                          float_4 recoverB, const Controls& c, bool cross) const {
        auto first = p < .5f;
        if (!simd::movemask(c.resonance > 0.f)) {
            CircuitResult result;
            result.current = 1.f;
            result.output = core(p, c.offset, c.balance);
            auto q = wrap(p * 2.f);
            result.sourceA = cross ? simd::ifelse(first, recoverB, q) : result.output;
            result.sourceB = cross ? simd::ifelse(first, q, recoverA) : result.output;
            result.gridA = result.gridB = result.screenLoad = 0.f;
            result.conduction = 1.f;
            return result;
        }
        auto q = wrap(p * 2.f);
        auto qa = simd::ifelse(first, q, recoverA);
        auto qb = simd::ifelse(first, recoverB, q);
        // Plate rails and component values below are explicit estimates, not
        // inferred from the panel graphics. Device current itself uses volts/mA.
        auto plateA = 12.f - 7.f * qa;
        auto plateB = 12.f - 7.f * qb;
        auto plate = simd::ifelse(first, plateA, plateB);
        // Reference Ia at G1=0 V, G2=10 V, plate=9 V, in mA.
        // 0.471404521 approximates sqrt(10/45); retain the expression/order.
        constexpr float nominal = .88f * (10.f / 45.f) * .471404521f * (9.f / 13.f);
        auto base = core(p, c.offset, c.balance);
        auto r = c.resonance;
        // The upper resonance range enters regenerative grid feedback. Returning an
        // inverted plate voltage with inverted grid injection makes the loop
        // positive; the coupling capacitors and screen loading limit/recover it.
        auto feedbackGain = .28f * r + 1.8f * r * r * r * r;
        auto plateLoad = .22f + .18f * r * r;
        float polarity = cross ? 1.f : -1.f;
        auto sourceA = cross ? (12.f - plateB) / 7.f : base;
        auto sourceB = cross ? (12.f - plateA) / 7.f : base;
        auto cap = simd::ifelse(first, capA, capB);
        auto selectedSource = simd::ifelse(first, sourceA, sourceB);
        auto grid = polarity * feedbackGain * (selectedSource - cap);
        // The plate load lies inside the returned signal path. Continue from the
        // previous conduction branch to retain regenerative hysteresis; bounded
        // iteration follows its cutoff/saturation transitions.
        float_4 ratio = simd::ifelse(first, conductionA, conductionB);
        for (int iteration = 0; iteration < 4; ++iteration) {
            auto loadedReturn =
                selectedSource + (cross ? .8f : -1.f) * plateLoad * r * (ratio - 1.f);
            grid = polarity * feedbackGain * (loadedReturn - cap);
            auto next = Tube1J24B::plateCurrent(grid, plate, screen) / simd::fmax(nominal, 1e-6f);
            ratio += .7f * (simd::clamp(next, 0.f, 5.f) - ratio);
        }
        CircuitResult result;
        result.conduction = simd::ifelse(r > 0.f, ratio, 1.f);
        // The programmed charging source supplies the DC current; a bounded
        // fraction of plate-current variation reaches the timing capacitor.
        // Strong grid regeneration can then ring without latching its clock.
        result.current = 1.f + .6f * r * clip(.8f * (ratio - 1.f));
        result.output = base - plateLoad * r * (ratio - 1.f);
        result.sourceA = cross ? sourceA + .8f * plateLoad * r * (ratio - 1.f) : result.output;
        result.sourceB = cross ? sourceB + .8f * plateLoad * r * (ratio - 1.f) : result.output;
        result.gridA = polarity * feedbackGain * (result.sourceA - capA);
        result.gridB = polarity * feedbackGain * (result.sourceB - capB);
        result.screenLoad = Tube1J24B::screenCurrent(grid, plate, screen);
        return result;
    }

    std::array<float_4, 4> process(const Controls& target, float sampleTime, bool cross,
                                   unsigned outputMask = 15) {
        const bool reconnectPulse = initialized && (outputMask & 2) && !pulseWasConnected;
        if (!initialized) {
            previous = target;
            initialized = true;
        }
        float dt = sampleTime / Oversample;
        float dcAlpha = 1.f - std::exp(-2.f * float(M_PI) * 5.f * dt);
        float resetDecay = std::exp(-dt / .000025f);
        float shelf = 1.f - std::exp(-2.f * float(M_PI) * 1000.f * dt);
        float tau = 220000.f * (Tube1J24B::inputCapacitance + 11.f * Tube1J24B::reverseCapacitance);
        float gridPole = 1.f - std::exp(-dt / tau);
        float_4 lastSync = previous.sync;
        for (int s = 0; s < Oversample; ++s) {
            // 1. Interpolate controls and integrate coupling/screen states.
            auto c = interpolate(previous, target, float(s + 1) / Oversample);
            for (int lane = 0; lane < 4; ++lane)
                if (c.resonance[lane] <= 0.f) {
                    tuningGain[lane] = 1.f;
                    phaseDebt[lane] = 0.;
                }
            auto original =
                circuit(phase, gridCapA, gridCapB, screenVoltage, recoveryA, recoveryB, c, cross);
            // RC coupling/grid-leak network, integrated with a midpoint step.
            // Positive-grid conduction charges the coupling capacitors; screen
            // current pulls the shared supply down and it subsequently recovers.
            auto capDerivative = [cross](float_4 source, float_4 cap, float_4 grid) {
                return (source - cap + (cross ? .4f : -.4f) * simd::fmax(grid - .05f, 0.f)) /
                       .00022f;
            };
            auto da = capDerivative(original.sourceA, gridCapA, original.gridA);
            auto db = capDerivative(original.sourceB, gridCapB, original.gridB);
            auto screenTarget =
                simd::clamp(10.f - 22.f * c.resonance * original.screenLoad, 6.f, 10.f);
            auto ds = (screenTarget - screenVoltage) / .003f;
            auto midpointPhase =
                wrap(phase + .5f * dt * c.frequency * original.current * tuningGain);
            auto midpoint =
                circuit(midpointPhase, gridCapA + .5f * dt * da, gridCapB + .5f * dt * db,
                        screenVoltage + .5f * dt * ds, recoveryA, recoveryB, c, cross);
            gridCapA +=
                dt * capDerivative(midpoint.sourceA, gridCapA + .5f * dt * da, midpoint.gridA);
            gridCapB +=
                dt * capDerivative(midpoint.sourceB, gridCapB + .5f * dt * db, midpoint.gridB);
            auto midScreenTarget =
                simd::clamp(10.f - 22.f * c.resonance * midpoint.screenLoad, 6.f, 10.f);
            screenVoltage += dt * (midScreenTarget - (screenVoltage + .5f * dt * ds)) / .003f;

            // 2. Advance the active timing charge, then handle external sync.
            auto increment = simd::clamp(c.frequency * midpoint.current * tuningGain * dt, 1e-10f,
                                         .45f / Oversample);
            auto oldPhase = phase;
            double preciseStart[4] = {};
            bool preciseStep[4] = {};
            if (simd::movemask(increment < 1e-6f)) {
                // At very low rates, a float increment can be smaller than a
                // phase ULP. Use double accumulators just for these lanes.
                for (int lane = 0; lane < 4; ++lane) {
                    if (increment[lane] < 1e-6f) {
                        if (!lowActive[lane])
                            lowPhase[lane] = phase[lane];
                        preciseStart[lane] = lowPhase[lane];
                        preciseStep[lane] = true;
                        lowPhase[lane] += double(increment[lane]);
                        lowPhase[lane] -= std::floor(lowPhase[lane]);
                        phase[lane] = std::min(float(lowPhase[lane]), .99999994f);
                        lowActive[lane] = true;
                    } else {
                        phase[lane] = rack::math::eucMod(phase[lane] + increment[lane], 1.f);
                        lowActive[lane] = false;
                    }
                }
            } else {
                phase = wrap(phase + increment);
                for (auto& active : lowActive)
                    active = false;
            }
            auto rising = (positive == 0.f) & (c.sync >= 1.f);
            auto falling = (negative == 0.f) & (c.sync <= -1.f);
            positive = simd::ifelse(c.sync <= .1f, 0.f, simd::ifelse(rising, 1.f, positive));
            negative = simd::ifelse(c.sync >= -.1f, 0.f, simd::ifelse(falling, 1.f, negative));

            bool synced[4] = {};
            for (int lane = 0; lane < 4; ++lane) {
                bool up = rising[lane] != 0.f, down = falling[lane] != 0.f;
                // Weak sync resets the corresponding half only; stronger drive
                // can take over either core. Bipolar sync can address both halves.
                if (!(up || down))
                    continue;
                bool strong = std::fabs(target.sync[lane]) >= 2.f;
                if (!strong && ((up && oldPhase[lane] >= .5f) || (down && oldPhase[lane] < .5f)))
                    continue;
                float delta = c.sync[lane] - lastSync[lane];
                float fraction =
                    std::fabs(delta) > 1e-12f
                        ? rack::clamp(((up ? 1.f : -1.f) - lastSync[lane]) / delta, 0.f, 1.f)
                        : 1.f;
                float_4 before = wrap(oldPhase + increment * fraction);
                float_4 resetPhase = up ? 0.f : .5f;
                auto beforeValue = circuit(before, gridCapA, gridCapB, screenVoltage, recoveryA,
                                           recoveryB, c, cross);
                auto afterValue = circuit(resetPhase, gridCapA, gridCapB, screenVoltage, recoveryA,
                                          recoveryB, c, cross);
                float_4 masked = 0.f;
                masked[lane] = afterValue.output[lane] - beforeValue.output[lane];
                if (Antialias)
                    syncBlep.insertDiscontinuity(std::max(-.999999f, fraction - 1.f), masked);
                synced[lane] = true;
                lowActive[lane] = false;
                phase[lane] = resetPhase[lane] + increment[lane] * (1.f - fraction);
            }

            // 3. Update the tuning loop from completed free-running cycles.
            // Automatic tuning trims the charging-current scale from full
            // free-running cycles. The reference integrates requested pitch/FM,
            // so compensation does not oppose intentional frequency modulation.
            // Forced sync resets the estimator instead of entering its error.
            for (int lane = 0; lane < 4; ++lane) {
                if (synced[lane]) {
                    referenceCycles[lane] = observedCycles[lane] = phaseDebt[lane] = 0.;
                    fullCycle[lane] = false;
                    continue;
                }
                double desired = double(c.frequency[lane]) * dt;
                double actual = increment[lane];
                // Use the accumulator that actually advanced this lane. The
                // capped float view cannot detect every very slow double wrap.
                double start = preciseStep[lane] ? preciseStart[lane] : double(oldPhase[lane]);
                bool wrapped = start + actual >= 1.;
                if (wrapped) {
                    double before = std::min(1., std::max(0., (1. - start) / actual));
                    referenceCycles[lane] += desired * before;
                    observedCycles[lane] += actual * before;
                    if (fullCycle[lane] && c.resonance[lane] > 0.f && observedCycles[lane] > .5) {
                        phaseDebt[lane] =
                            std::max(-.5, std::min(.5, phaseDebt[lane] + referenceCycles[lane] -
                                                           observedCycles[lane]));
                        double ratio =
                            referenceCycles[lane] / observedCycles[lane] + .4 * phaseDebt[lane];
                        ratio = std::max(.25, std::min(4., ratio));
                        tuningGain[lane] =
                            rack::clamp(tuningGain[lane] * float(std::sqrt(ratio)), .2f, 8.f);
                    }
                    referenceCycles[lane] = desired * (1. - before);
                    observedCycles[lane] = actual * (1. - before);
                    fullCycle[lane] = true;
                } else {
                    referenceCycles[lane] += desired;
                    observedCycles[lane] += actual;
                }
            }
            lastSync = c.sync;

            // 4. Recover inactive cores and read the loaded raw waveform.
            auto q = wrap(2.f * oldPhase);
            recoveryA = simd::ifelse(oldPhase < .5f, q, recoveryA * resetDecay);
            recoveryB = simd::ifelse(oldPhase >= .5f, q, recoveryB * resetDecay);
            auto output =
                circuit(phase, gridCapA, gridCapB, screenVoltage, recoveryA, recoveryB, c, cross);
            conductionA = simd::ifelse(phase < .5f, output.conduction, conductionA);
            conductionB = simd::ifelse(phase >= .5f, output.conduction, conductionB);
            auto raw = output.output;
            if (Antialias)
                raw += syncBlep.process() +
                       bandlimitedCore(phase, increment * Oversample, c.offset, c.balance) -
                       core(phase, c.offset, c.balance);

            // 5. The Pulse path compares Raw against its own threshold.
            if (outputMask & 2) {
                auto comparator = raw - c.pwm;
                auto pulse = simd::ifelse(comparator >= 0.f, 1.f, -1.f);
                if (reconnectPulse && s == 0) {
                    // Discard the disconnected path's old edge/FIR history.
                    // Seed the current level, rather than inventing a comparator
                    // transition or fading through an unrelated old waveform.
                    pulseBlep.pos = 0;
                    for (auto& sample : pulseBlep.buf)
                        sample = 0.f;
                    previousComparator = comparator;
                    decimators[1].reset();
                    for (auto& sample : decimators[1].inBuffer)
                        sample = 5.f * pulse;
                }
                auto oldPulse = simd::ifelse(previousComparator >= 0.f, 1.f, -1.f);
                for (int lane = 0; lane < 4; ++lane)
                    if (Antialias && pulse[lane] != oldPulse[lane]) {
                        float fraction = previousComparator[lane] /
                                         (previousComparator[lane] - comparator[lane]);
                        float_4 jump = 0.f;
                        jump[lane] = pulse[lane] - oldPulse[lane];
                        pulseBlep.insertDiscontinuity(std::max(-.999999f, fraction - 1.f), jump);
                    }
                previousComparator = comparator;
                if (Antialias)
                    pulse += pulseBlep.process();

                buffer[1][s] = 5.f * pulse;
            }

            if (outputMask & 12) {
                // 6. Shared Smooth/Fold drive and bias stage. Shape's tilt is an
                // estimated fixed RC shelf, not pitch tracking.
                colorLow += shelf * (raw - colorLow);
                auto colored = raw + c.shape * (colorLow - raw) * .75f;
                auto bias = -.08f + c.shape * .35f;
                auto gridDrive = colored * (3.f * c.drive * c.drive);
                // Published input/reverse capacitance, estimated 220k grid resistor.
                tubeGrid += gridPole * (gridDrive - tubeGrid);
                auto quietPlate = Tube1J24B::loadedPlate(bias, 9.f);
                auto drivenPlate = Tube1J24B::loadedPlate(bias + tubeGrid, 9.f);
                auto smooth = (quietPlate - drivenPlate) * .25f * simd::fmin(c.drive * 32.f, 1.f);
                // Only Fold runs the additional 2x interpolation/decimation stage.
                if (outputMask & 8) {
                    float_4 folded;
                    if (Antialias) {
                        float_4 sub[2];
                        for (int j = 0; j < 2; ++j) {
                            auto x = (previousSmooth +
                                      (smooth - previousSmooth) * (float(j + 1) * .5f)) *
                                     2.3f;
                            for (auto& stage : foldStages)
                                x = stage.process(x);
                            sub[j] = clip(x * 1.2f);
                        }
                        folded = foldDecimator.process(sub);
                    } else {
                        folded = smooth * 2.3f;
                        for (int i = 0; i < 4; ++i)
                            folded = 1.4f * folded - 2.2f * clip(1.6f * folded);
                        folded = clip(folded * 1.2f);
                    }
                    previousSmooth = smooth;
                    folded *= simd::fmin(c.drive * 32.f, 1.f);
                    dcFold += dcAlpha * (folded - dcFold);
                    buffer[3][s] = outputBuffer(5.f * (folded - dcFold));
                }
                dcSmooth += dcAlpha * (smooth - dcSmooth);
                buffer[2][s] = outputBuffer(5.f * (smooth - dcSmooth));
            }
            // Raw remains independent of the optional output paths above.
            dcRaw += dcAlpha * (raw - dcRaw);
            buffer[0][s] = outputBuffer(5.f * (raw - dcRaw));
        }
        previous = target;
        pulseWasConnected = (outputMask & 2) != 0;

        // 7. Decimate connected outputs; the final clamp is emergency headroom.
        std::array<float_4, 4> out;
        for (int o = 0; o < 4; ++o)
            out[o] = (outputMask & (1u << o))
                         ? simd::clamp(decimators[o].process(buffer[o].data()), -12.f, 12.f)
                         : float_4(0.f);
        return out;
    }
};
} // namespace aladfar
