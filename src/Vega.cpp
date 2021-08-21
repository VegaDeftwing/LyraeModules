#include "plugin.hpp"


struct Vega : Module {
	enum ParamIds {
		ARINGATT_PARAM,
		AOUTMODE_PARAM,
		A_PARAM,
		ARINGMODE_PARAM,
		ACURVE_PARAM,
		AFORCEADV_PARAM,
		DRINGATT_PARAM,
		DOUTMODE_PARAM,
		D_PARAM,
		DRINGMODE_PARAM,
		DCURVE_PARAM,
		DFORCEADV_PARAM,
		SRINGATT_PARAM,
		SOUTMODE_PARAM,
		S_PARAM,
		SRINGMODE_PARAM,
		SFORCEADV_PARAM,
		ROUTMODE_PARAM,
		R_PARAM,
		RRINGATT_PARAM,
		RRINGMODE_PARAM,
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
		configParam(ARINGATT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(AOUTMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(A_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ARINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ACURVE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(AFORCEADV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DRINGATT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DOUTMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(D_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DRINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DCURVE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DFORCEADV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SRINGATT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SOUTMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(S_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SRINGMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SFORCEADV_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ROUTMODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(R_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RRINGATT_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RRINGMODE_PARAM, 0.f, 1.f, 0.f, "");
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

		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH*12, 5))); // Top
		addChild(createWidget<Bolt>(Vec(box.size.x - 12 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH))); //Bottom

		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(19.964, 10.795)), module, Vega::ARINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(45.599, 10.839)), module, Vega::AOUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 14.467)), module, Vega::A_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(28.186, 18.839)), module, Vega::ARINGMODE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 24.119)), module, Vega::ACURVE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(36.32, 26.798)), module, Vega::AFORCEADV_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(19.964, 34.795)), module, Vega::DRINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(45.599, 34.839)), module, Vega::DOUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 38.467)), module, Vega::D_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(28.186, 42.839)), module, Vega::DRINGMODE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 48.119)), module, Vega::DCURVE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(36.32, 50.798)), module, Vega::DFORCEADV_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(19.964, 58.795)), module, Vega::SRINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(45.599, 59.089)), module, Vega::SOUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 66.839)), module, Vega::S_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(28.186, 66.839)), module, Vega::SRINGMODE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(36.32, 74.798)), module, Vega::SFORCEADV_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(45.599, 83.089)), module, Vega::ROUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 86.467)), module, Vega::R_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(23.709, 86.839)), module, Vega::RRINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(31.843, 94.798)), module, Vega::RRINGMODE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 96.118)), module, Vega::RCURVE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(32.372, 110.027)), module, Vega::ANGER_PARAM));

		addInput(createInputCentered<InJack>(mm2px(Vec(24.098, 14.839)), module, Vega::AMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(32.274, 22.839)), module, Vega::AADV_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(24.098, 38.839)), module, Vega::DMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(32.274, 46.839)), module, Vega::DADV_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(24.098, 62.839)), module, Vega::SMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(32.274, 70.839)), module, Vega::SADV_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(27.797, 90.839)), module, Vega::RMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(8.332, 110.027)), module, Vega::GATE_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(20.23, 110.027)), module, Vega::GLOBALRING_INPUT));

		addOutput(createOutputCentered<OutJack>(mm2px(Vec(49.687, 14.839)), module, Vega::AOUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(57.864, 22.839)), module, Vega::AGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(49.687, 38.839)), module, Vega::DOUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(57.864, 46.839)), module, Vega::DGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(49.687, 63.089)), module, Vega::SOUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(57.864, 71.089)), module, Vega::SGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(49.687, 87.089)), module, Vega::ROUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(57.864, 95.089)), module, Vega::RGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(47.774, 119.85)), module, Vega::MAINOUTM_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(59.3, 119.889)), module, Vega::MAINOUTP_OUTPUT));
	}
};


Model* modelVega = createModel<Vega, VegaWidget>("Vega");