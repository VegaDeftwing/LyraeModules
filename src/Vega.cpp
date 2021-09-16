#include "plugin.hpp"


struct Vega : Module {
	enum ParamIds {
		AOUTMODE_PARAM,
		A_PARAM,
		ARINGATT_PARAM,
		ARINGMODE_PARAM,
		AFORCEADV_PARAM,
		ACURVE_PARAM,
		GLOBALRINGATT_PARAM,
		GLOBALRINGOFFSET_PARAM,
		DOUTMODE_PARAM,
		D_PARAM,
		DRINGATT_PARAM,
		DRINGMODE_PARAM,
		DFORCEADV_PARAM,
		DCURVE_PARAM,
		SOUTMODE_PARAM,
		SRINGATT_PARAM,
		SRINGMODE_PARAM,
		S_PARAM,
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
		AMODELIGHT_LIGHT,
		AGATELIGHT_LIGHT,
		DMODELIGHT_LIGHT,
		DGATELIGHT_LIGHT,
		SMODELIGHT_LIGHT,
		SGATELIGHT_LIGHT,
		RMODELIGHT_LIGHT,
		RGATELIGHT_LIGHT,
		NUM_LIGHTS
	};

	Vega() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ARINGATT_PARAM, 0.f, 1.f, 0.f, "Attack Ring Attenuate");
		configParam(AOUTMODE_PARAM, 0.f, 1.f, 0.f, "Attack Output Mode");
		configParam(A_PARAM, 0.01, 0.f, 0.f, "Attack Time");
		configParam(ARINGMODE_PARAM, 0.f, 1.f, 0.f, "Attack Ring Mode");
		configParam(ACURVE_PARAM, 0.f, 1.f, 0.f, "Attack Curve");
		configParam(AFORCEADV_PARAM, 0.f, 1.f, 0.f, "Attack Force Advance");
		configParam(DRINGATT_PARAM, 0.f, 1.f, 0.f, "Decay Ring Attenuate");
		configParam(DOUTMODE_PARAM, 0.f, 1.f, 0.f, "Decay Output Mode");
		configParam(D_PARAM, 0.001, 0.f, 0.f, "Decay Time");
		configParam(DRINGMODE_PARAM, 0.f, 1.f, 0.f, "Decay Ring Mode");
		configParam(DCURVE_PARAM, 0.f, 1.f, 0.f, "Decay Curve");
		configParam(DFORCEADV_PARAM, 0.f, 1.f, 0.f, "Decay Force Advance");
		configParam(SRINGATT_PARAM, 0.f, 1.f, 0.f, "Sustain Ring Attenuate");
		configParam(SOUTMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Mode");
		configParam(S_PARAM, 0.f, 1.f, 0.f, "Sustain Level");
		configParam(SRINGMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Ring Mode");
		configParam(SFORCEADV_PARAM, 0.f, 1.f, 0.f, "Sustain Force Advance");
		configParam(ROUTMODE_PARAM, 0.f, 1.f, 0.f, "Release Ring Mode");
		configParam(R_PARAM, 0.001, 0.f, 0.f, "Release Time");
		configParam(RRINGATT_PARAM, 0.f, 1.f, 0.f, "Release Ring Attenuate");
		configParam(RRINGMODE_PARAM, 0.f, 1.f, 0.f, "Release Ring Mode");
		configParam(RCURVE_PARAM, 0.f, 1.f, 0.f, "Release Curve");
		configParam(ANGER_PARAM, 0.f, 1.f, 0.f, "Transistion Time Control");
		configParam(GLOBALRINGATT_PARAM, 0.f, 1.f, 0.f, "Gloal Ring Attenuate");
		configParam(GLOBALRINGOFFSET_PARAM, 0.f, 1.f, 0.f, "Global Ring Offset");
	}

	int stage = 0;
	bool isRunning = false;
	float env = 0.f;
	float output = 0.f;
	dsp::SchmittTrigger gateDetect;

	void process(const ProcessArgs& args) override {
		// First, we need to get the gate
		bool gate = inputs[GATE_INPUT].value > 1.0;
		if (gateDetect.process(gate)) {
			isRunning = true;
			stage = 0; //Attack
		}

		if (isRunning) {
			if (gate){
				switch (stage){
				case 0: // Attack
					env += params[A_PARAM].getValue();
					if (env > 1.0){
						stage = 1;
					}
					if (inputs[AMOD_INPUT].isConnected()){
						output = (inputs[AMOD_INPUT].getVoltage() * params[ARINGATT_PARAM].getValue()) * env + env;
					} else{
						output = env;
					}
					outputs[AGATE_OUTPUT].setVoltage(10.f);
					outputs[DGATE_OUTPUT].setVoltage(0.f);
					outputs[SGATE_OUTPUT].setVoltage(0.f);
					outputs[RGATE_OUTPUT].setVoltage(0.f);
					if (outputs[AOUT_OUTPUT].isConnected()){
						outputs[AOUT_OUTPUT].setVoltage(10.f * env);
					}
					break;
				case 1: // Decay
					env -= params[D_PARAM].getValue();
					if (env <= params[S_PARAM].getValue() + 0.001){
						stage = 2;
					}
					if (inputs[DMOD_INPUT].isConnected()){
						output = (inputs[DMOD_INPUT].getVoltage() * params[DRINGATT_PARAM].getValue()) * env + env;
					} else{
						output = env;
					}
					outputs[AGATE_OUTPUT].setVoltage(0.f);
					outputs[DGATE_OUTPUT].setVoltage(10.f);
					outputs[SGATE_OUTPUT].setVoltage(0.f);
					outputs[RGATE_OUTPUT].setVoltage(0.f);
					if (outputs[AOUT_OUTPUT].isConnected()){
						outputs[AOUT_OUTPUT].setVoltage(0.f);
					}
					if (outputs[DOUT_OUTPUT].isConnected()){
						outputs[DOUT_OUTPUT].setVoltage(10.f * env);
					}
					break;
				case 2: // Sustain
					if (inputs[SMOD_INPUT].isConnected()){
						output = (inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue()) * env + env;
					} else{
						output = params[S_PARAM].getValue();
					}
					outputs[AGATE_OUTPUT].setVoltage(0.f);
					outputs[DGATE_OUTPUT].setVoltage(0.f);
					outputs[SGATE_OUTPUT].setVoltage(10.f);
					outputs[RGATE_OUTPUT].setVoltage(0.f);
					if (outputs[DOUT_OUTPUT].isConnected()){
						outputs[DOUT_OUTPUT].setVoltage(0.f);
					}
					if (outputs[SOUT_OUTPUT].isConnected()){
						outputs[SOUT_OUTPUT].setVoltage(10.f * env);
					}
					break;
				default:
					break;
				}
			} else{
				stage = 3; //Release
			}
			if (stage == 3) //Release
			{
				env -= params[R_PARAM].getValue();
				outputs[AGATE_OUTPUT].setVoltage(0.f);
				outputs[DGATE_OUTPUT].setVoltage(0.f);
				outputs[SGATE_OUTPUT].setVoltage(0.f);
				outputs[RGATE_OUTPUT].setVoltage(10.f);

				if (env < 0){
					env = 0;
					outputs[RGATE_OUTPUT].setVoltage(0.f);
					isRunning = false;
				}

				if (inputs[RMOD_INPUT].isConnected()){
					output = (inputs[RMOD_INPUT].getVoltage() * params[RRINGATT_PARAM].getValue()) * env + env;
				} else{
					output = env;
				}

				if (outputs[SOUT_OUTPUT].isConnected()){
					outputs[SOUT_OUTPUT].setVoltage(0.f);
				}
				if (outputs[ROUT_OUTPUT].isConnected()){
					outputs[ROUT_OUTPUT].setVoltage(10.f * env);
				}
			}

			//Output
			// TODO need a global ring att and offset
			if (inputs[GLOBALRING_INPUT].isConnected()){
				if (outputs[MAINOUTP_OUTPUT].isConnected()){
					outputs[MAINOUTP_OUTPUT].setVoltage(output * 10.f * inputs[GLOBALRING_INPUT].getVoltage());
				}
				if (outputs[MAINOUTM_OUTPUT].isConnected()){
					outputs[MAINOUTM_OUTPUT].setVoltage(-1.f * output * 10.f * inputs[GLOBALRING_INPUT].getVoltage());
				}
			} else{
				if (outputs[MAINOUTP_OUTPUT].isConnected()){
					outputs[MAINOUTP_OUTPUT].setVoltage(output * 10.f);
				}
				if (outputs[MAINOUTM_OUTPUT].isConnected()){
					outputs[MAINOUTM_OUTPUT].setVoltage(-1.f * output * 10.f);
				}
			}
			
		}
	}
};


struct VegaWidget : ModuleWidget {
	VegaWidget(Vega* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Vega.svg")));

		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH*15, 5))); // Top
		addChild(createWidget<Bolt>(Vec(box.size.x - 15 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH))); //Bottom

		addParam(createParamCentered<TL1105>(mm2px(Vec(54.916, 14.974)), module, Vega::AOUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 14.467)), module, Vega::A_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 14.839)), module, Vega::ARINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 14.839)), module, Vega::ARINGMODE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 22.839)), module, Vega::AFORCEADV_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 24.119)), module, Vega::ACURVE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(106.448, 29.573)), module, Vega::GLOBALRINGATT_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(113.94, 29.573)), module, Vega::GLOBALRINGOFFSET_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 38.839)), module, Vega::DOUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 38.467)), module, Vega::D_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 38.839)), module, Vega::DRINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 38.839)), module, Vega::DRINGMODE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 46.839)), module, Vega::DFORCEADV_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 48.119)), module, Vega::DCURVE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 63.089)), module, Vega::SOUTMODE_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 62.839)), module, Vega::SRINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 62.839)), module, Vega::SRINGMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 66.839)), module, Vega::S_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 70.839)), module, Vega::SFORCEADV_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 87.089)), module, Vega::ROUTMODE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 86.467)), module, Vega::R_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 86.839)), module, Vega::RRINGATT_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 86.839)), module, Vega::RRINGMODE_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 96.118)), module, Vega::RCURVE_PARAM));
		addParam(createParamCentered<HexKnob>(mm2px(Vec(46.111, 109.923)), module, Vega::ANGER_PARAM));

		addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 14.839)), module, Vega::AMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(41.196, 22.839)), module, Vega::AADV_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 38.839)), module, Vega::DMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(41.196, 46.839)), module, Vega::DADV_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 62.839)), module, Vega::SMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(41.196, 70.839)), module, Vega::SADV_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 86.839)), module, Vega::RMOD_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(8.332, 110.027)), module, Vega::GATE_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(20.23, 110.027)), module, Vega::GLOBALRING_INPUT));

		addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 14.839)), module, Vega::AOUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 22.839)), module, Vega::AGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 38.839)), module, Vega::DOUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 46.839)), module, Vega::DGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 63.089)), module, Vega::SOUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 71.089)), module, Vega::SGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 87.089)), module, Vega::ROUT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 95.089)), module, Vega::RGATE_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(63.014, 119.784)), module, Vega::MAINOUTM_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(74.54, 119.823)), module, Vega::MAINOUTP_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.108, 18.839)), module, Vega::AMODELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(66.712, 18.839)), module, Vega::AGATELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.108, 42.839)), module, Vega::DMODELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(66.712, 42.839)), module, Vega::DGATELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.108, 66.839)), module, Vega::SMODELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(66.712, 67.089)), module, Vega::SGATELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(37.108, 90.839)), module, Vega::RMODELIGHT_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(66.712, 91.089)), module, Vega::RGATELIGHT_LIGHT));
	}
};


Model* modelVega = createModel<Vega, VegaWidget>("Vega");