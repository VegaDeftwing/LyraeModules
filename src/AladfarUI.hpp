#pragma once

// Coordinates match res/Aladfar.svg (24 HP). Lettering is outlined in the SVG.
struct AladfarWidget : ModuleWidget {
    explicit AladfarWidget(Aladfar* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Aladfar.svg")));
        addChild(createWidget<Bolt>(Vec(2, 2)));
        addChild(createWidget<Bolt>(Vec(348, 368)));

        auto knob = [&](float x, float y, int id) {
            addParam(createParamCentered<HexKnob>(Vec(x, y), module, id));
        };
        auto small = [&](float x, float y, int id) {
            addParam(createParamCentered<SmallHexKnob>(Vec(x, y), module, id));
        };
        auto input = [&](float x, float y, int id) {
            addInput(createInputCentered<InJack>(Vec(x, y), module, id));
        };
        auto cv = [&](float x, float y, int inputId, int amountId) {
            input(x - 20.f, y, inputId);
            small(x + 20.f, y, amountId);
        };

        // Pitch and external modulation.
        knob(43, 56, Aladfar::FREQUENCY_PARAM);
        small(89, 56, Aladfar::FINE_PARAM);
        input(143, 56, Aladfar::PITCH_INPUT);
        small(199, 56, Aladfar::FM_AMOUNT_PARAM);
        input(237, 56, Aladfar::FM_INPUT);
        small(289, 56, Aladfar::SYNC_AMOUNT_PARAM);
        input(327, 56, Aladfar::SYNC_INPUT);

        // Core controls, each with its own CV input and attenuverter below.
        knob(65, 144, Aladfar::OFFSET_PARAM);
        knob(180, 144, Aladfar::BALANCE_PARAM);
        knob(277, 144, Aladfar::RESONANCE_PARAM);
        addParam(createParamCentered<CKSS>(Vec(320, 144), module, Aladfar::MODE_PARAM));
        cv(65, 186, Aladfar::OFFSET_INPUT, Aladfar::OFFSET_AMOUNT_PARAM);
        cv(180, 186, Aladfar::BALANCE_INPUT, Aladfar::BALANCE_AMOUNT_PARAM);
        cv(295, 186, Aladfar::RESONANCE_INPUT, Aladfar::RESONANCE_AMOUNT_PARAM);

        // Wave shaping.
        knob(65, 255, Aladfar::DRIVE_PARAM);
        knob(180, 255, Aladfar::SHAPE_PARAM);
        knob(295, 255, Aladfar::PWM_PARAM);
        cv(65, 297, Aladfar::DRIVE_INPUT, Aladfar::DRIVE_AMOUNT_PARAM);
        cv(180, 297, Aladfar::SHAPE_INPUT, Aladfar::SHAPE_AMOUNT_PARAM);
        cv(295, 297, Aladfar::PWM_INPUT, Aladfar::PWM_AMOUNT_PARAM);

        addOutput(createOutputCentered<OutJack>(Vec(55, 347), module, Aladfar::RAW_OUTPUT));
        addOutput(createOutputCentered<OutJack>(Vec(138, 347), module, Aladfar::PULSE_OUTPUT));
        addOutput(createOutputCentered<OutJack>(Vec(222, 347), module, Aladfar::SMOOTH_OUTPUT));
        addOutput(createOutputCentered<OutJack>(Vec(305, 347), module, Aladfar::FOLD_OUTPUT));
    }
};
