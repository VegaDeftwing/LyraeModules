#include "plugin.hpp"
#include "dsp/Aladfar.hpp"

// Rack adapter: keep parameter/port IDs stable for saved patches. DSP state is
// preallocated in four independent banks of four SIMD voices.
struct Aladfar : Module {
    enum ParamIds {
        FREQUENCY_PARAM,
        FINE_PARAM,
        OFFSET_PARAM,
        BALANCE_PARAM,
        PWM_PARAM,
        RESONANCE_PARAM,
        DRIVE_PARAM,
        SHAPE_PARAM,
        MODE_PARAM,
        FM_AMOUNT_PARAM,
        SYNC_AMOUNT_PARAM,
        OFFSET_AMOUNT_PARAM,
        BALANCE_AMOUNT_PARAM,
        DRIVE_AMOUNT_PARAM,
        RESONANCE_AMOUNT_PARAM,
        PWM_AMOUNT_PARAM,
        SHAPE_AMOUNT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        FM_INPUT,
        SYNC_INPUT,
        OFFSET_INPUT,
        BALANCE_INPUT,
        DRIVE_INPUT,
        RESONANCE_INPUT,
        PWM_INPUT,
        SHAPE_INPUT,
        NUM_INPUTS
    };
    enum OutputIds { RAW_OUTPUT, PULSE_OUTPUT, SMOOTH_OUTPUT, FOLD_OUTPUT, NUM_OUTPUTS };
    std::array<aladfar::Bank<>, 4> banks;

    Aladfar() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0);
        configParam(FREQUENCY_PARAM, -8.f, 6.f, 0.f, "Frequency", " Hz", 2.f, dsp::FREQ_C4);
        configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine tune", " semitones");
        configParam(OFFSET_PARAM, 0.f, 1.f, .5f, "Core offset", "%", 0.f, 100.f);
        configParam(BALANCE_PARAM, 0.f, 1.f, 0.f, "Core balance", "%", 0.f, 100.f);
        configParam(PWM_PARAM, -1.f, 1.f, 0.f, "Pulse threshold", " V", 0.f, 5.f);
        configParam(RESONANCE_PARAM, 0.f, 1.f, 0.f, "Core feedback resonance", "%", 0.f, 100.f);
        configParam(DRIVE_PARAM, 0.f, 1.f, .35f, "Smooth / fold drive", "%", 0.f, 100.f);
        configParam(SHAPE_PARAM, -1.f, 1.f, 0.f, "Saturation bias / spectral tilt", "%", 0.f,
                    100.f);
        configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Feedback route", {"Direct", "Cross-coupled"});
        configParam(FM_AMOUNT_PARAM, 0.f, 1.f, 0.f, "Linear FM amount", "%", 0.f, 100.f);
        configParam(SYNC_AMOUNT_PARAM, 0.f, 1.f, 0.f, "Sync level", "%", 0.f, 100.f);
        const int amounts[] = {OFFSET_AMOUNT_PARAM,    BALANCE_AMOUNT_PARAM, DRIVE_AMOUNT_PARAM,
                               RESONANCE_AMOUNT_PARAM, PWM_AMOUNT_PARAM,     SHAPE_AMOUNT_PARAM};
        const char* names[] = {"Offset CV",    "Balance CV",         "Smooth / fold CV",
                               "Resonance CV", "Pulse threshold CV", "Shape CV"};
        for (int i = 0; i < 6; ++i)
            configParam(amounts[i], -1.f, 1.f, i == 2 ? 1.f : 0.f, names[i], "%", 0.f, 100.f);
        const char* inputs[] = {"Pitch (1 V/oct)",
                                "Linear FM (C4 per volt at full amount)",
                                "Level-sensitive dual-core sync",
                                "Offset",
                                "Balance",
                                "Smooth / fold drive",
                                "Resonance",
                                "Pulse threshold",
                                "Shape"};
        for (int i = 0; i < NUM_INPUTS; ++i)
            configInput(i, inputs[i]);
        const char* outputs[] = {"Raw core", "Pulse comparator", "Smooth saturation",
                                 "Four-stage fold"};
        for (int i = 0; i < NUM_OUTPUTS; ++i)
            configOutput(i, outputs[i]);
    }

    void onReset(const ResetEvent& e) override {
        Module::onReset(e);
        for (auto& bank : banks)
            bank.reset();
    }

    void process(const ProcessArgs& args) override {
        int channels = 1;
        for (auto& input : inputs)
            channels = std::max(channels, input.getChannels());
        channels = std::min(channels, 16);
        // Output bits follow OutputIds: Raw=1, Pulse=2, Smooth=4, Fold=8.
        // Disconnected paths pause their histories; the core also pauses when
        // no output is connected.
        unsigned outputMask = 0;
        for (int o = 0; o < NUM_OUTPUTS; ++o)
            if (outputs[o].isConnected())
                outputMask |= 1u << o;
        if (!outputMask)
            return;
        for (int c = 0; c < channels; c += 4) {
            // Rack broadcasts a mono cable. Explicitly zero absent poly lanes
            // and reject non-finite CV before it enters persistent DSP state.
            auto voltage = [&](int id) {
                simd::float_4 v = inputs[id].getPolyVoltageSimd<simd::float_4>(c);
                for (int lane = 0; lane < 4; ++lane)
                    if ((inputs[id].getChannels() != 1 && c + lane >= inputs[id].getChannels()) ||
                        !std::isfinite(v[lane]))
                        v[lane] = 0.f;
                return simd::clamp(v, -100.f, 100.f);
            };
            // Full attenuverter gain maps 10 V to one normalized control unit.
            auto mod = [&](int param, int input, int amount, float low, float high) {
                return simd::clamp(params[param].getValue() +
                                       voltage(input) * .1f * params[amount].getValue(),
                                   low, high);
            };
            aladfar::Controls control;
            auto octaves =
                simd::clamp(params[FREQUENCY_PARAM].getValue() +
                                params[FINE_PARAM].getValue() / 12.f + voltage(PITCH_INPUT),
                            -16.f, 16.f);
            control.frequency = simd::clamp(
                dsp::FREQ_C4 * simd::pow(2.f, octaves) +
                    voltage(FM_INPUT) * params[FM_AMOUNT_PARAM].getValue() * dsp::FREQ_C4,
                .01f, args.sampleRate * .4f);
            control.offset = mod(OFFSET_PARAM, OFFSET_INPUT, OFFSET_AMOUNT_PARAM, 0.f, 1.f);
            control.balance = mod(BALANCE_PARAM, BALANCE_INPUT, BALANCE_AMOUNT_PARAM, 0.f, 1.f);
            control.drive = mod(DRIVE_PARAM, DRIVE_INPUT, DRIVE_AMOUNT_PARAM, 0.f, 1.f);
            control.resonance =
                mod(RESONANCE_PARAM, RESONANCE_INPUT, RESONANCE_AMOUNT_PARAM, 0.f, 1.f);
            control.pwm = mod(PWM_PARAM, PWM_INPUT, PWM_AMOUNT_PARAM, -1.f, 1.f);
            control.shape = mod(SHAPE_PARAM, SHAPE_INPUT, SHAPE_AMOUNT_PARAM, -1.f, 1.f);
            control.sync = voltage(SYNC_INPUT) * params[SYNC_AMOUNT_PARAM].getValue();
            auto out = banks[c / 4].process(control, args.sampleTime,
                                            params[MODE_PARAM].getValue() >= .5f, outputMask);
            for (int o = 0; o < NUM_OUTPUTS; ++o)
                outputs[o].setVoltageSimd(out[o], c);
        }
        for (auto& output : outputs)
            output.setChannels(channels);
    }
};

#include "AladfarUI.hpp"
Model* modelAladfar = createModel<Aladfar, AladfarWidget>("Aladfar");
