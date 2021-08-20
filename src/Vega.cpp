#include "plugin.hpp"


struct Vega : Module {
	enum ParamIds {
		ARINGMODE_PARAM,
		A_PARAM,
		ACURVE_PARAM,
		ASELF_PARAM,
		DRINGMODE_PARAM,
		D_PARAM,
		DCURVE_PARAM,
		DSELF_PARAM,
		SRINGMODE_PARAM,
		S_PARAM,
		R_PARAM,
		RRINGMODE_PARAM,
		RSELF_PARAM,
		RCURVE_PARAM,
		ANGER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		AMOD_INPUT,
		AADV_INPUT,
		DMOD_INPUT,
		DADV_INPUT,
		SMOD_INPUT,
		SADV_INPUT,
		RMOD_INPUT,
		GATE_INPUT,
		GLOBALRING_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		AOUT_OUTPUT,
		AGATE_OUTPUT,
		DOUT_OUTPUT,
		DGATE_OUTPUT,
		SOUT_OUTPUT,
		SGATE_OUTPUT,
		ROUT_OUTPUT,
		RGATE_OUTPUT,
		MAINOUTM_OUTPUT,
		MAINOUTP_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Vega() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ARINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(A_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ACURVE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ASELF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DRINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(D_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DCURVE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DSELF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SRINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(S_PARAM, 0.f, 1.f, 0.f, "");
		configParam(R_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RRINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RSELF_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RCURVE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ANGER_PARAM, 0.f, 1.f, 0.f, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct VegaWidget : ModuleWidget {
	VegaWidget(Vega* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Vega.svg")));

		// Bolts
		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH*12, 5))); // Top
		addChild(createWidget<Bolt>(Vec(box.size.x - 12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH))); //Bottom

		// Attack
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 14.467)), module, Vega::A_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(20.083, 10.577)), module, Vega::ARINGMODE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 24.119)), module, Vega::ACURVE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(36.19, 26.465)), module, Vega::ASELF_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.098, 14.839)), module, Vega::AMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.274, 22.839)), module, Vega::AADV_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.687, 14.839)), module, Vega::AOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(57.864, 22.839)), module, Vega::AGATE_OUTPUT));

		// Decay
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 38.467)), module, Vega::D_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(20.083, 34.577)), module, Vega::DRINGMODE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 48.119)), module, Vega::DCURVE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(36.19, 50.465)), module, Vega::DSELF_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.098, 38.839)), module, Vega::DMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.274, 46.839)), module, Vega::DADV_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.687, 38.839)), module, Vega::DOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(57.864, 46.839)), module, Vega::DGATE_OUTPUT));

		// Sustain
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 66.839)), module, Vega::S_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(20.083, 58.577)), module, Vega::SRINGMODE_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.098, 62.839)), module, Vega::SMOD_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.274, 70.839)), module, Vega::SADV_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.687, 63.089)), module, Vega::SOUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(57.864, 71.089)), module, Vega::SGATE_OUTPUT));

		// Release
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 86.467)), module, Vega::R_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(23.606, 86.873)), module, Vega::RRINGMODE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(31.713, 94.465)), module, Vega::RSELF_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 96.118)), module, Vega::RCURVE_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(27.797, 90.839)), module, Vega::RMOD_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.687, 87.089)), module, Vega::ROUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(57.864, 95.089)), module, Vega::RGATE_OUTPUT));

		// Global
		addParam(createParamCentered<HexKnob>(mm2px(Vec(32.372, 110.027)), module, Vega::ANGER_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.332, 110.027)), module, Vega::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.23, 110.027)), module, Vega::GLOBALRING_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(47.774, 119.85)), module, Vega::MAINOUTM_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(59.3, 119.889)), module, Vega::MAINOUTP_OUTPUT));
	}
};


Model* modelVega = createModel<Vega, VegaWidget>("Vega");