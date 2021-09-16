#include "plugin.hpp"


struct Vega : Module {
	enum ParamIds {
		//Do Net Reorder these
		AFORCEADV_PARAM,
		DFORCEADV_PARAM,
		SFORCEADV_PARAM,

		AOUTMODE_PARAM,
		A_PARAM,
		ARINGATT_PARAM,
		ARINGMODE_PARAM,
		ACURVE_PARAM,
		GLOBALRINGATT_PARAM,
		GLOBALRINGOFFSET_PARAM,
		DOUTMODE_PARAM,
		D_PARAM,
		DRINGATT_PARAM,
		DRINGMODE_PARAM,
		DCURVE_PARAM,
		SOUTMODE_PARAM,
		SRINGATT_PARAM,
		SRINGMODE_PARAM,
		S_PARAM,
		ROUTMODE_PARAM,
		R_PARAM,
		RRINGATT_PARAM,
		RRINGMODE_PARAM,
		RCURVE_PARAM,
		ANGER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		//Do not reorder these!
		AADV_INPUT,
		DADV_INPUT,
		SADV_INPUT,

		AMOD_INPUT,
		DMOD_INPUT,
		SMOD_INPUT,
		RMOD_INPUT,
		GATE_INPUT,
		GLOBALRING_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		//Do not reorder these!
		AOUT_OUTPUT,
		DOUT_OUTPUT,
		SOUT_OUTPUT,
		ROUT_OUTPUT,

		AGATE_OUTPUT,
		DGATE_OUTPUT,
		SGATE_OUTPUT,
		RGATE_OUTPUT,
		MAINOUTM_OUTPUT,
		MAINOUTP_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(AMODE_LIGHT, 3),
		ENUMS(DMODE_LIGHT, 3),
		ENUMS(SMODE_LIGHT, 3),
		ENUMS(RMODE_LIGHT, 3),
		ENUMS(AGATE_LIGHT, 3),
		ENUMS(DGATE_LIGHT, 3),
		ENUMS(SGATE_LIGHT, 3),
		ENUMS(RGATE_LIGHT, 3),
		NUM_LIGHTS
	};

	dsp::ClockDivider processDivider;

	Vega() {
		processDivider.setDivision(64);
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ARINGATT_PARAM, 0.f, 1.f, 0.f, "Attack Ring Attenuate");
		configParam(AOUTMODE_PARAM, 0.f, 1.f, 0.f, "Attack Output Mode");
		configParam(A_PARAM, 0.5, 1.5, 1.045, "Attack Time");
		configParam(ARINGMODE_PARAM, 0.f, 1.f, 0.f, "Attack Ring Mode");
		configParam(ACURVE_PARAM, 0.5, 2.f, 1.f, "Attack Curve");
		configParam(AFORCEADV_PARAM, 0.f, 1.f, 0.f, "Attack Force Advance");
		configParam(DRINGATT_PARAM, 0.f, 1.f, 0.f, "Decay Ring Attenuate");
		configParam(DOUTMODE_PARAM, 0.f, 1.f, 0.f, "Decay Output Mode");
		configParam(D_PARAM, 0.9, 1.5, 1.0648, "Decay Time");
		configParam(DRINGMODE_PARAM, 0.f, 1.f, 0.f, "Decay Ring Mode");
		configParam(DCURVE_PARAM, 0.f, 1.f, 0.f, "Decay Curve");
		configParam(DFORCEADV_PARAM, 0.f, 1.f, 0.f, "Decay Force Advance");
		configParam(SRINGATT_PARAM, 0.f, 1.f, 0.f, "Sustain Ring Attenuate");
		configParam(SOUTMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Mode");
		configParam(S_PARAM, 0.f, 1.f, 0.5, "Sustain Level");
		configParam(SRINGMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Ring Mode");
		configParam(SFORCEADV_PARAM, 0.f, 1.f, 0.f, "Sustain Force Advance");
		configParam(ROUTMODE_PARAM, 0.f, 1.f, 0.f, "Release Ring Mode");
		configParam(R_PARAM, 0.9, 1.5, 1.1836, "Release Time");
		configParam(RRINGATT_PARAM, 0.f, 1.f, 0.f, "Release Ring Attenuate");
		configParam(RRINGMODE_PARAM, 0.f, 1.f, 0.f, "Release Ring Mode");
		configParam(RCURVE_PARAM, 0.f, 1.f, 0.f, "Release Curve");
		configParam(ANGER_PARAM, 0.f, 1.f, 0.f, "Transistion Time Control");
		configParam(GLOBALRINGATT_PARAM, 0.f, 1.f, 0.f, "Gloal Ring Attenuate");
		configParam(GLOBALRINGOFFSET_PARAM, 0.f, 1.f, 1.f, "Global Ring Offset");
	}

	int stage = 0;
	bool isRunning = false;
	float env = 0.f;
	float output = 0.f;
	dsp::SchmittTrigger gateDetect;
	bool AOutMode = false;
	bool DOutMode = false;
	bool SOutMode = true;
	bool ROutMode = false;

	void displayActive(int lstage){
		//TODO these lights need to change color depending on the output mode
		lights[AGATE_LIGHT + 0].setBrightness(lstage == 0 ? 1.f : 0.f);
		lights[DGATE_LIGHT + 0].setBrightness(lstage == 1 ? 1.f : 0.f);
		lights[SGATE_LIGHT + 0].setBrightness(lstage == 2 ? 1.f : 0.f);
		lights[RGATE_LIGHT + 0].setBrightness(lstage == 3 ? 1.f : 0.f);
		lights[AGATE_LIGHT + 2].setBrightness(AOutMode ? 1.f : 0.f);
		lights[DGATE_LIGHT + 2].setBrightness(DOutMode ? 1.f : 0.f);
		lights[SGATE_LIGHT + 2].setBrightness(SOutMode ? 0.6 : 0.f);
		lights[RGATE_LIGHT + 2].setBrightness(ROutMode ? 1.f : 0.f);
		outputs[AGATE_OUTPUT].setVoltage(lstage == 0 ? 10.f : 0.f);
		outputs[DGATE_OUTPUT].setVoltage(lstage == 1 ? 10.f : 0.f);
		outputs[SGATE_OUTPUT].setVoltage(lstage == 2 ? 10.f : 0.f);
		outputs[RGATE_OUTPUT].setVoltage(lstage == 3 ? 10.f : 0.f);
	}

	void forceAdvance(int lstage){
		if (inputs[lstage].isConnected()){
			if (inputs[lstage].getVoltage() >= 5.f){
				stage = lstage + 1;
			}
		}
		if (params[stage].getValue() >= .5f){
			stage = lstage + 1;
		}
	}

	void perStageOutput(int stage, bool mode, float output){
		if (stage != 0){ //the attack stage dosen't need to turn off the release stage's output
			if (outputs[stage-1].isConnected()){
				outputs[stage-1].setVoltage(0.f);
			}
		}
		if (outputs[stage].isConnected()){
			if (mode == false) //default, not with ring{
				outputs[stage].setVoltage(10.f * env);
			} else{
				outputs[stage].setVoltage(output);
			}
	}

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
					env += simd::pow(.000315,params[A_PARAM].getValue());

					
					if (env > 1.0){
						stage = 1;
					}
					if (inputs[AMOD_INPUT].isConnected()){
						output = (inputs[AMOD_INPUT].getVoltage() * params[ARINGATT_PARAM].getValue()) * env + env;
					} else{
						output = env;
					}
					displayActive(0);
					perStageOutput(0,false,output);
					forceAdvance(0); //checks if the force advance is true internally
					
					break;
				case 1: // Decay
					env -= simd::pow(.000315,params[D_PARAM].getValue());
					if (env <= params[S_PARAM].getValue() + 0.001){
						stage = 2;
					}
					if (inputs[DMOD_INPUT].isConnected()){
						output = (inputs[DMOD_INPUT].getVoltage() * params[DRINGATT_PARAM].getValue()) * env + env;
					} else{
						output = env;
					}
					displayActive(1);
					perStageOutput(1,false,output);
					forceAdvance(1); //checks if the force advance is true internally

					break;
				case 2: // Sustain
					if (inputs[SMOD_INPUT].isConnected()){
						output = (inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue()) * env + env;
					} else{
						output = params[S_PARAM].getValue();
					}
					displayActive(2);
					perStageOutput(2,false,output);
					forceAdvance(2); //checks if the force advance is true internally

					break;
				default:
					break;
				}
			} else{
				stage = 3; //Release
			}
			if (stage == 3) //Release
			{
				env -= simd::pow(.000315,params[R_PARAM].getValue());
				displayActive(3);

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

				perStageOutput(3,false,output);
			}

			//Output
			if (inputs[GLOBALRING_INPUT].isConnected()){
				if (outputs[MAINOUTP_OUTPUT].isConnected()){
					outputs[MAINOUTP_OUTPUT].setVoltage(output * 10.f * (inputs[GLOBALRING_INPUT].getVoltage() * params[GLOBALRINGATT_PARAM].getValue() + params[GLOBALRINGOFFSET_PARAM].getValue()));
				}
				if (outputs[MAINOUTM_OUTPUT].isConnected()){
					outputs[MAINOUTM_OUTPUT].setVoltage(-1.f * output * 10.f * (inputs[GLOBALRING_INPUT].getVoltage() * params[GLOBALRINGATT_PARAM].getValue() + params[GLOBALRINGOFFSET_PARAM].getValue()));
				}
			} else{
				// If no ring input connected, the offset knob works as a volume knob, to add more headroom when necessary
				if (outputs[MAINOUTP_OUTPUT].isConnected()){
					outputs[MAINOUTP_OUTPUT].setVoltage(output * (10.f * params[GLOBALRINGOFFSET_PARAM].getValue()));
				}
				if (outputs[MAINOUTM_OUTPUT].isConnected()){
					outputs[MAINOUTM_OUTPUT].setVoltage(-1.f * output * (10.f * params[GLOBALRINGOFFSET_PARAM].getValue()));
				}
			}
			
		}
		if (processDivider.process()){
			// TODO toggle states for Out Gate modes, then set outputs on each stage to either include the ring mod or not
			// TODO toggle states for Ring Modes, update mode LED respectively
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
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(29.573, 106.448)), module, Vega::GLOBALRINGATT_PARAM));
		addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(29.573, 113.94)), module, Vega::GLOBALRINGOFFSET_PARAM));
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

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(37.108, 18.839)), module, Vega::AMODE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(66.712, 18.839)), module, Vega::AGATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(37.108, 42.839)), module, Vega::DMODE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(66.712, 42.839)), module, Vega::DGATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(37.108, 66.839)), module, Vega::SMODE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(66.712, 67.089)), module, Vega::SGATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(37.108, 90.839)), module, Vega::RMODE_LIGHT));
		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(66.712, 91.089)), module, Vega::RGATE_LIGHT));
	}
};


Model* modelVega = createModel<Vega, VegaWidget>("Vega");