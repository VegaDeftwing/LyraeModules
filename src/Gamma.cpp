#include "plugin.hpp"


struct Gamma : Module {
	enum ParamIds {
		KNOB_PARAM,
		INVERTSWITCH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		OFFSET_INPUT,
		LEFT_INPUT,
		RIGHT_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Gamma() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(KNOB_PARAM, -10.f, 10.f, 0.f, "bipolar offset/ring attenuversion");
		configParam(INVERTSWITCH_PARAM, 0.f, 1.f, 0.f, "Invert 2nd Output");
	}

	float offset = 0.f;

	void process(const ProcessArgs& args) override {
		
		if (inputs[OFFSET_INPUT].isConnected()){
			offset = inputs[OFFSET_INPUT].getVoltage() * (.1 * params[KNOB_PARAM].getValue());
		} else{
			offset = params[KNOB_PARAM].getValue();
		}
		
		outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() + offset);

		if (inputs[RIGHT_INPUT].isConnected()){
			if (params[INVERTSWITCH_PARAM].getValue()){
				outputs[RIGHT_OUTPUT].setVoltage(-1.f * inputs[RIGHT_INPUT].getVoltage() - offset);
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage() - offset );
			}
		} else{
			if (params[INVERTSWITCH_PARAM].getValue()){
				outputs[RIGHT_OUTPUT].setVoltage(-1.f * inputs[LEFT_INPUT].getVoltage() - offset);
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() - offset );
			}
		}
	}
};


struct GammaWidget : ModuleWidget {
	GammaWidget(Gamma* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Gamma.svg")));

		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH, + 5)));
		addChild(createWidget<Bolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 5, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<HexKnob>(mm2px(Vec(5.08, 69.693)), module, Gamma::KNOB_PARAM));
		addParam(createParamCentered<NKK>(mm2px(Vec(5.191, 116.946)), module, Gamma::INVERTSWITCH_PARAM));

		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 56.943)), module, Gamma::OFFSET_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 96.016)), module, Gamma::LEFT_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 105.876)), module, Gamma::RIGHT_INPUT));

		addOutput(createOutputCentered<OutJack>(mm2px(Vec(5.08, 12.4)), module, Gamma::LEFT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(5.08, 22.26)), module, Gamma::RIGHT_OUTPUT));
	}
};


Model* modelGamma = createModel<Gamma, GammaWidget>("Gamma");