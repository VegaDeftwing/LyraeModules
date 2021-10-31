#include "plugin.hpp"


struct BD383238 : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		ACURVE_INPUT,
		D_INPUT,
		DCURVE_INPUT,
		S_INPUT,
		R_INPUT,
		RCURVE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	BD383238() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs& args) override {
	}
};


struct BD383238Widget : ModuleWidget {
	BD383238Widget(BD383238* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BD383238.svg")));

		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH, + 5)));
		addChild(createWidget<Bolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 5, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 14.467)), module, BD383238::A_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 24.119)), module, BD383238::ACURVE_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 38.467)), module, BD383238::D_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 48.119)), module, BD383238::DCURVE_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 66.839)), module, BD383238::S_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 86.467)), module, BD383238::R_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 96.118)), module, BD383238::RCURVE_INPUT));
	}
};


Model* modelBD383238 = createModel<BD383238, BD383238Widget>("BD383238");