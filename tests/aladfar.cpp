#include "../src/Aladfar.cpp"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <fstream>
Plugin* pluginInstance = nullptr;
using aladfar::float_4;
void require(bool condition, const char* what) {
    if (!condition)
        throw std::runtime_error(what);
}
void near(float a, float b, float tolerance, const char* what) {
    require(std::isfinite(a) && std::fabs(a - b) <= tolerance, what);
}
void connect(Module& m) {
    for (auto& o : m.outputs)
        o.channels = 1;
}

// Independent device checkpoints and double-precision ADAA reference.
void tubeAndAntiderivative() {
    const float volts[] = {5, 10, 20, 40, 60, 80};
    const float milliamps[] = {.49f, .63f, .73f, .80f, .82f, .84f};
    for (int i = 0; i < 6; ++i)
        near(aladfar::Tube1J24B::plateCurrent(0.f, volts[i], 45.f)[0], milliamps[i], .025f,
             "1J24B model must follow published zero-grid plate curve");
    near(aladfar::Tube1J24B::plateCurrent(-1.f, 60.f, 45.f)[0], .20f, .03f,
         "1J24B negative-grid current checkpoint");
    near(aladfar::Tube1J24B::plateCurrent(1.f, 60.f, 45.f)[0], 1.5f, .08f,
         "1J24B positive-grid current checkpoint");
    float gm = (aladfar::Tube1J24B::plateCurrent(.001f, 60.f, 45.f)[0] -
                aladfar::Tube1J24B::plateCurrent(-.001f, 60.f, 45.f)[0]) /
               .002f;
    require(gm >= .6f && gm <= 1.2f, "modeled transconductance must lie in nominal 1J24B range");
    for (int i = 0; i < 100; ++i) {
        float grid = -2.f + i * .05f;
        auto plate = aladfar::Tube1J24B::loadedPlate(grid, 9.f);
        auto current = aladfar::Tube1J24B::plateCurrent(grid, plate, 9.f);
        near(plate[0] + 82.f * current[0], 12.f, .0001f,
             "third-stage plate solution must satisfy its load line");
    }
    auto primitive = [](double x) {
        double a = std::fabs(x);
        return a <= 3. ? x * x / 18. + 4. / 3. * std::log((x * x + 3.) / 3.)
                       : a - 3. + .5 + 4. / 3. * std::log(4.);
    };
    for (int i = 0; i < 150; ++i) {
        float x = -6.f + i * .08f;
        for (float delta : {0.f, 1e-5f, .01f, .7f, 6.f}) {
            float y = x + delta;
            double expected = delta == 0 ? aladfar::clip(x)[0]
                                         : (primitive(double(y)) - primitive(double(x))) /
                                               (double(y) - double(x));
            near(
                aladfar::clipAverage(x, y)[0], float(expected), 3e-6f,
                "ADAA must match independent double-precision integral, including clipping boundaries");
        }
    }
    std::cout
        << "Published 1J24B curves, transconductance, load-line solution and ADAA integral passed\n";
}

// Six waveform cases from the manual, before coupling/output conditioning.
void diagrams() {
    for (int i = 0; i < 100; ++i) {
        float p = (i + .1f) / 100.f;
        float ramp = std::fmod(2 * p, 1.f) - .5f;
        near(aladfar::core(p, 0.f, 0.f)[0], ramp, 1e-6f,
             "minimum offset/balance must give two rising ramps");
        near(aladfar::core(p, .5f, 0.f)[0], 2 * p - 1.f, 1e-6f,
             "middle offset and minimum balance must give a continuous saw");
        float v = p < .5f ? -2 * p : 2 * p - 2;
        near(aladfar::core(p, 0.f, 1.f)[0], v, 1e-6f,
             "minimum offset and maximum balance must give the manual's V shape");
    }
    near(aladfar::core(.125f, .5f, .5f)[0], -.5f, 1e-6f,
         "balance midpoint must turn the first half into a triangle");
    near(aladfar::core(.375f, .5f, .5f)[0], -.5f, 1e-6f,
         "first-half triangle must descend after its peak");
    near(aladfar::core(.75f, .5f, .5f)[0], .25f, 1e-6f, "second half must retain its rising ramp");
    std::cout << "Manual waveform diagrams passed\n";
}

// Compare SIMD lanes to separate mono modules, including zero-to-nonzero CV.
void routing() {
    Aladfar poly;
    std::array<std::unique_ptr<Aladfar>, 16> mono;
    connect(poly);
    for (auto& m : mono) {
        m.reset(new Aladfar);
        connect(*m);
    }
    poly.params[Aladfar::OFFSET_AMOUNT_PARAM].setValue(.7f);
    poly.params[Aladfar::RESONANCE_PARAM].setValue(0.f);
    poly.params[Aladfar::RESONANCE_AMOUNT_PARAM].setValue(1.f);
    for (auto& m : mono)
        for (size_t p = 0; p < poly.params.size(); ++p)
            m->params[p].setValue(poly.params[p].getValue());
    for (int frame = 0; frame < 3500; ++frame) {
        poly.inputs[Aladfar::PITCH_INPUT].channels = 16;
        poly.inputs[Aladfar::OFFSET_INPUT].channels = 3;
        poly.inputs[Aladfar::DRIVE_INPUT].channels = 1;
        poly.inputs[Aladfar::RESONANCE_INPUT].channels = 16;
        for (int c = 0; c < 16; ++c)
            poly.inputs[Aladfar::RESONANCE_INPUT].setVoltage(
                (c % 4 == 0 && frame < 1000) ? 0.f : float((c % 4 + 1) * 2.5f), c);
        poly.inputs[Aladfar::DRIVE_INPUT].setVoltage(.5f * std::sin(frame * .01f));
        for (int c = 0; c < 16; ++c)
            poly.inputs[Aladfar::PITCH_INPUT].setVoltage((c - 8) * .15f, c);
        for (int c = 0; c < 3; ++c)
            poly.inputs[Aladfar::OFFSET_INPUT].setVoltage(std::sin(frame * .02f + c), c);
        Module::ProcessArgs args{48000.f, 1.f / 48000.f, frame};
        poly.process(args);
        for (int c = 0; c < 16; ++c) {
            auto& m = *mono[c];
            for (size_t i = 0; i < poly.inputs.size(); ++i) {
                m.inputs[i].channels = 1;
                m.inputs[i].setVoltage(poly.inputs[i].getPolyVoltage(c));
            }
            m.process(args);
            for (int o = 0; o < 4; ++o)
                near(poly.outputs[o].getVoltage(c), m.outputs[o].getVoltage(), 2e-5f,
                     "SIMD poly lane must match its independent mono oscillator");
        }
    }
    poly.inputs[Aladfar::RESONANCE_INPUT].channels = 0;
    for (int count : {4, 1, 2, 8, 16}) {
        poly.inputs[Aladfar::PITCH_INPUT].channels = count;
        poly.process({96000.f, 1.f / 96000.f, 0});
        for (auto& o : poly.outputs)
            require(o.getChannels() == std::max(3, count),
                    "widest input must determine every output width");
    }
    Aladfar a, b;
    connect(a);
    connect(b);
    b.params[Aladfar::PWM_PARAM].setValue(.8f);
    for (int frame = 0; frame < 5000; ++frame) {
        Module::ProcessArgs args{48000.f, 1.f / 48000.f, frame};
        a.process(args);
        b.process(args);
        for (int o : {0, 2, 3})
            near(a.outputs[o].getVoltage(), b.outputs[o].getVoltage(), 0.f,
                 "PWM must affect only the Pulse output");
    }
    std::cout
        << "16-voice SIMD isolation, mono broadcast, short cables, channel changes and PWM independence passed\n";
}

// Bounds, output independence, sync semantics and state-reset coverage.
void behavior() {
    for (float rate : {22050.f, 44100.f, 48000.f, 96000.f, 192000.f}) {
        Aladfar m;
        connect(m);
        m.params[Aladfar::DRIVE_PARAM].setValue(0.f);
        for (int frame = 0; frame < 1000; ++frame) {
            m.process({rate, 1.f / rate, frame});
            for (int o : {2, 3})
                near(m.outputs[o].getVoltage(), 0.f, 0.f,
                     "zero drive must silence Smooth and Fold");
        }
        m.params[Aladfar::DRIVE_PARAM].setValue(1.f);
        m.params[Aladfar::RESONANCE_PARAM].setValue(1.f);
        m.params[Aladfar::FM_AMOUNT_PARAM].setValue(1.f);
        m.params[Aladfar::SYNC_AMOUNT_PARAM].setValue(1.f);
        for (auto& i : m.inputs)
            i.channels = 16;
        for (int frame = 0; frame < 2000; ++frame) {
            m.params[Aladfar::MODE_PARAM].setValue((frame / 300) % 2);
            m.params[Aladfar::SHAPE_PARAM].setValue(frame % 2 ? 1.f : -1.f);
            for (size_t i = 0; i < m.inputs.size(); ++i)
                for (int c = 0; c < 16; ++c)
                    m.inputs[i].setVoltage(25.f * std::sin(frame * .27f + c + i), c);
            if (frame % 17 == 0)
                m.inputs[0].setVoltage(std::numeric_limits<float>::quiet_NaN(), 3);
            if (frame % 23 == 0)
                m.inputs[1].setVoltage(std::numeric_limits<float>::infinity(), 2);
            m.process({rate, 1.f / rate, frame});
            for (auto& o : m.outputs)
                for (int c = 0; c < 16; ++c)
                    require(std::isfinite(o.getVoltage(c)) && std::fabs(o.getVoltage(c)) <= 12.f,
                            "extreme modulation must remain finite and bounded");
        }
        for (auto& bank : m.banks)
            bank.reset();
    }
    aladfar::Bank<> direct, cross, plain;
    aladfar::Controls c;
    c.resonance = .95f;
    c.frequency = 180.f;
    double modeDifference = 0., resDifference = 0., foldDifference = 0.;
    for (int i = 0; i < 10000; ++i) {
        auto a = direct.process(c, 1.f / 48000.f, false);
        auto b = cross.process(c, 1.f / 48000.f, true);
        auto d = c;
        d.resonance = 0.f;
        auto z = plain.process(d, 1.f / 48000.f, false);
        if (i > 5000) {
            modeDifference += std::fabs(a[0][0] - b[0][0]);
            resDifference += std::fabs(a[0][0] - z[0][0]);
            foldDifference += std::fabs(a[2][0] - a[3][0]);
        }
    }
    require(modeDifference > 100. && resDifference > 100. && foldDifference > 100.,
            "resonance routes and four-stage fold must produce distinct audible waveforms");
    // Sync at zero level is completely disconnected from oscillator behavior.
    Aladfar silentSync, unpatched;
    connect(silentSync);
    connect(unpatched);
    silentSync.inputs[Aladfar::SYNC_INPUT].channels = 1;
    for (int i = 0; i < 1000; ++i) {
        silentSync.inputs[Aladfar::SYNC_INPUT].setVoltage(i % 50 < 25 ? 10.f : -10.f);
        Module::ProcessArgs a{48000.f, 1.f / 48000.f, i};
        silentSync.process(a);
        unpatched.process(a);
        near(silentSync.outputs[0].getVoltage(), unpatched.outputs[0].getVoltage(), 0.f,
             "zero sync level must disable synchronization");
    }
    aladfar::Bank<> locked;
    c = aladfar::Controls{};
    c.frequency = 731.f;
    for (int frame = 0; frame < 1000; ++frame) {
        c.sync = frame % 100 < 50 ? 5.f : -5.f;
        locked.process(c, 1.f / 48000.f, false);
        if (frame % 100 == 0)
            require(locked.phase[0] < .02f, "positive sync must reset first half");
        if (frame % 100 == 50)
            require(locked.phase[0] >= .5f && locked.phase[0] < .52f,
                    "negative sync must reset second half");
    }
    aladfar::Bank<> low;
    low.phase = .25f;
    c = aladfar::Controls{};
    c.frequency = .01f;
    for (int i = 0; i < 1000; ++i)
        low.process(c, 1.f / 192000.f, false);
    near(low.phase[0], .25f + .01f * 1000 / 192000.f, 3e-7f,
         "sub-audio phase accumulation must not stall at high sample rates");
    aladfar::Bank<> allOutputs, rawOnly;
    c.resonance = .8f;
    for (int i = 0; i < 1000; ++i) {
        auto all = allOutputs.process(c, 1.f / 48000.f, false);
        auto raw = rawOnly.process(c, 1.f / 48000.f, false, 1);
        near(all[0][0], raw[0][0], 0.f, "skipping unused output paths must not change Raw");
    }
    std::cout
        << "Drive mute, resonance routes, fold, sync levels/polarities, malformed CV and five sample rates passed\n";
}

double aliasRatio(const std::vector<float>& samples, int fundamental) {
    int n = samples.size();
    std::vector<float> spectrum(n * 2);
    rack::dsp::RealFFT fft(n);
    fft.rfft(samples.data(), spectrum.data());
    double wanted = 0., unwanted = 0.;
    for (int bin = 1; bin < n / 2; ++bin) {
        double power = double(spectrum[2 * bin]) * spectrum[2 * bin] +
                       double(spectrum[2 * bin + 1]) * spectrum[2 * bin + 1];
        if (bin % fundamental == 0)
            wanted += power;
        else
            unwanted += power;
    }
    return unwanted / std::max(wanted, 1e-30);
}

// Coherent FFT bins distinguish legal harmonics from folded-back energy.
void spectrum() {
    constexpr int n = 8192, k = 431;
    aladfar::Bank<> bank;
    aladfar::Bank<1, 16, false> uncorrected;
    aladfar::Controls c;
    c.frequency = 48000.f * k / n;
    c.balance = .31f;
    c.offset = .77f;
    c.drive = .95f;
    c.shape = .3f;
    std::array<std::vector<float>, 4> clean, naive;
    for (auto& v : clean)
        v.reserve(n);
    for (auto& v : naive)
        v.reserve(n);
    for (int frame = 0; frame < n * 5; ++frame) {
        auto out = bank.process(c, 1.f / 48000.f, false);
        auto ref = uncorrected.process(c, 1.f / 48000.f, false);
        if (frame < n * 4)
            continue;
        for (int o = 0; o < 4; ++o)
            clean[o].push_back(out[o][0]);
        for (int o = 0; o < 4; ++o)
            naive[o].push_back(ref[o][0]);
    }
    for (int o = 0; o < 4; ++o) {
        double actual = aliasRatio(clean[o], k), baseline = aliasRatio(naive[o], k);
        double improvement = 10. * std::log10(baseline / actual);
        std::cout << "Output " << o << " alias: " << 10. * std::log10(actual) << " dBc; "
                  << improvement << " dB improvement over naive synthesis\n";
        require(improvement > 15.,
                "antialias path must substantially suppress inharmonic folded-back energy");
    }
}

// Measure actual output crossings rather than trusting the phase increment.
void tuning() {
    for (float rate : {44100.f, 48000.f, 96000.f}) {
        Aladfar m;
        connect(m);
        m.inputs[Aladfar::PITCH_INPUT].channels = 4;
        for (int c = 0; c < 4; ++c)
            m.inputs[0].setVoltage(c - 2.f, c);
        int crossings[4] = {}, last[4] = {};
        double periods[4] = {};
        float previous[4] = {};
        for (int f = 0; f < int(rate * .5f); ++f) {
            m.process({rate, 1.f / rate, f});
            if (f < 2000)
                continue;
            for (int c = 0; c < 4; ++c) {
                float x = m.outputs[0].getVoltage(c);
                if (x >= 0 && previous[c] < 0) {
                    if (last[c]) {
                        periods[c] += f - last[c];
                        ++crossings[c];
                    }
                    last[c] = f;
                }
                previous[c] = x;
            }
        }
        for (int c = 0; c < 4; ++c) {
            float actual = rate * crossings[c] / periods[c];
            float expected = dsp::FREQ_C4 * std::pow(2.f, c - 2.f);
            require(std::fabs(actual / expected - 1.f) < .002f,
                    "oscillator must accurately track 1 V/oct across sample rates");
        }
    }
    std::cout << "1 V/oct pitch tracking passed\n";
}
void resonanceTuning() {
    for (float rate : {48000.f, 96000.f})
        for (bool cross : {false, true})
            for (float resonance : {.25f, .6f, 1.f}) {
                aladfar::Bank<> bank;
                aladfar::Controls control;
                control.frequency = float_4(65.40639f, 261.62556f, 1046.5022f, 2093.0044f);
                control.resonance = resonance;
                control.balance = .3f;
                control.offset = .7f;
                int first[4] = {}, last[4] = {}, cycles[4] = {};
                int warmup = int(rate * .6f);
                // Regenerative bursts vary successive periods. Average the carrier
                // over a longer window at the maximum setting, not individual cycles.
                int measure = int(rate * (resonance == 1.f ? 4.f : .6f));
                for (int frame = 0; frame < warmup + measure; ++frame) {
                    auto previous = bank.phase;
                    bank.process(control, 1.f / rate, cross, 1);
                    if (frame < warmup)
                        continue;
                    for (int lane = 0; lane < 4; ++lane)
                        if (bank.phase[lane] < previous[lane]) {
                            if (!cycles[lane])
                                first[lane] = frame;
                            last[lane] = frame;
                            ++cycles[lane];
                        }
                }
                for (int lane = 0; lane < 4; ++lane) {
                    float actual = rate * (cycles[lane] - 1) / float(last[lane] - first[lane]);
                    if (!(cycles[lane] > 10 &&
                          std::fabs(actual / control.frequency[lane] - 1.f) < .001f))
                        std::cerr << "Tuning: rate=" << rate << " cross=" << cross
                                  << " r=" << resonance << " target=" << control.frequency[lane]
                                  << " actual=" << actual << " gain=" << bank.tuningGain[lane]
                                  << "\n";
                    require(
                        cycles[lane] > 10 &&
                            std::fabs(actual / control.frequency[lane] - 1.f) < .001f,
                        "resonance compensation must retain pitch in both routes across sample rates");
                }
            }
    std::cout
        << "Resonance pitch compensation across levels, five octaves and both routes passed\n";
}

// Normalize high-frequency energy so level gain alone cannot pass this test.
void resonanceTexture() {
    constexpr int n = 8192;
    for (bool cross : {false, true}) {
        aladfar::Bank<> bank;
        aladfar::Controls c;
        c.frequency = 110.f;
        c.resonance = float_4(0.f, .4f, .75f, 1.f);
        std::array<std::vector<float>, 4> waves;
        for (auto& w : waves)
            w.resize(n);
        for (int frame = 0; frame < 48000 + n; ++frame) {
            auto out = bank.process(c, 1.f / 48000.f, cross, 1);
            if (frame >= 48000)
                for (int lane = 0; lane < 4; ++lane)
                    waves[lane][frame - 48000] = out[0][lane];
        }
        dsp::RealFFT fft(n);
        std::vector<float> windowed(n), bins(n * 2);
        double fractions[4] = {};
        for (int lane = 0; lane < 4; ++lane) {
            for (int i = 0; i < n; ++i)
                windowed[i] = waves[lane][i] * float(.5 - .5 * std::cos(2 * M_PI * i / n));
            fft.rfft(windowed.data(), bins.data());
            double total = 0., high = 0.;
            for (int bin = 1; bin < n / 2; ++bin) {
                double power = double(bins[bin * 2]) * bins[bin * 2] +
                               double(bins[bin * 2 + 1]) * bins[bin * 2 + 1];
                total += power;
                if (bin * 48000.f / n >= 3000.f)
                    high += power;
            }
            fractions[lane] = high / total;
            float peak = 0.f;
            int clipped = 0;
            for (float v : waves[lane]) {
                peak = std::max(peak, std::fabs(v));
                clipped += std::fabs(v) >= 11.999f;
            }
            require(clipped == 0,
                    "resonant texture must not be produced by the host-rate safety clamp");
            std::cout << "Texture mode=" << cross << " r=" << c.resonance[lane]
                      << " HF=" << 10 * std::log10(high / total) << " dB peak=" << peak
                      << " clipped=" << clipped << " gain=" << bank.tuningGain[lane] << "\n";
        }
        require(fractions[3] > 4. * fractions[0],
                "maximum resonance must add high-frequency texture, not merely gain");
    }
}
void benchmark() {
    for (int channels : {1, 16}) {
        Aladfar m;
        connect(m);
        m.inputs[0].channels = channels;
        m.params[Aladfar::RESONANCE_PARAM].setValue(.8f);
        auto t = std::chrono::steady_clock::now();
        float total = 0;
        for (int frame = 0; frame < 24000; ++frame) {
            m.process({48000.f, 1.f / 48000.f, frame});
            total += m.outputs[0].getVoltage();
        }
        auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t).count();
        std::cout << channels << " voices: " << seconds / 24000. * 1e6 << " us/sample ("
                  << seconds / .5 * 100 << "% of one core at 48 kHz), checksum " << total << "\n";
    }
}
int main(int argc, char** argv) {
    std::unique_ptr<Model> owner(modelAladfar);
    try {
        if (argc > 1 && std::string(argv[1]) == "--resonance-report") {
            resonanceTexture();
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--benchmark") {
            benchmark();
            return 0;
        }
        tubeAndAntiderivative();
        diagrams();
        routing();
        behavior();
        tuning();
        resonanceTuning();
        resonanceTexture();
        spectrum();
        std::cout << "Aladfar tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
