#include "plugin.hpp"

struct Vega : Module {
	enum ParamIds {
		//Do Net Reorder these
		AFORCEADV_PARAM,
		DFORCEADV_PARAM,
		SFORCEADV_PARAM,

		AOUTMODE_PARAM,
		DOUTMODE_PARAM,
		SOUTMODE_PARAM,
		ROUTMODE_PARAM,

		A_PARAM,
		ARINGATT_PARAM,
		ARINGMODE_PARAM,
		ACURVE_PARAM,
		GLOBALRINGATT_PARAM,
		GLOBALRINGOFFSET_PARAM,
		D_PARAM,
		DRINGATT_PARAM,
		DRINGMODE_PARAM,
		DCURVE_PARAM,
		SRINGATT_PARAM,
		SRINGMODE_PARAM,
		S_PARAM,
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

	//This, along with the mess in the process divider which updates lastValue, makes
	// it so that each knob will 'inherit' the default value of the knob above it
	// this makes setting equal amounts of attenuation really easy.
	struct ChainParamQuantity : ParamQuantity {
		float getDefaultValue() override {
			Vega *vega = dynamic_cast<Vega *>(module);
				if (paramId <= 0)
					return 0.f;
				if (!module)
					return 0.f;
			return vega->lastValue;
		}
	};

	//This makes the decay curve default value whatever it needs to be to go back
	//to being linear while still leaving the full range
	struct BezierParamQuantity : ParamQuantity {
		float getDefaultValue() override {
			Vega *vega = dynamic_cast<Vega *>(module);
				if (paramId <= 0)
					return 0.f;
				if (!module)
					return 0.f;
			return((1 + vega->sus)/2);
		}
	};

	Vega() {
		processDivider.setDivision(64);
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ARINGATT_PARAM, 0.f, 0.2, 0.f, "Attack Ring Attenuate");
		configParam(AOUTMODE_PARAM, 0.f, 1.f, 0.f, "Attack Output Mode");
		configParam(A_PARAM, 0.5, 1.5, 1.1125, "Attack Time");
		configParam(ARINGMODE_PARAM, 0.f, 1.f, 0.f, "Attack Ring Mode");
		configParam(ACURVE_PARAM, 0.2, 3.f, 1.f, "Attack Curve");
		configParam(AFORCEADV_PARAM, 0.f, 1.f, 0.f, "Attack Force Advance");
		configParam<ChainParamQuantity>(DRINGATT_PARAM, 0.f, 0.2, 0.f, "Decay Ring Attenuate");
		configParam(DOUTMODE_PARAM, 0.f, 1.f, 0.f, "Decay Output Mode");
		configParam(D_PARAM, 0.9, 1.5, 1.216, "Decay Time");
		configParam(DRINGMODE_PARAM, 0.f, 1.f, 0.f, "Decay Ring Mode");
		configParam<BezierParamQuantity>(DCURVE_PARAM, 0.f, 1.3f, 0.75, "Decay Curve");
		configParam(DFORCEADV_PARAM, 0.f, 1.f, 0.f, "Decay Force Advance");
		configParam<ChainParamQuantity>(SRINGATT_PARAM, 0.f, 0.2, 0.f, "Sustain Ring Attenuate");
		configParam(SOUTMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Mode");
		configParam(S_PARAM, 0.f, 1.f, 0.5, "Sustain Level");
		configParam(SRINGMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Ring Mode");
		configParam(SFORCEADV_PARAM, 0.f, 1.f, 0.f, "Sustain Force Advance");
		configParam(ROUTMODE_PARAM, 0.f, 1.f, 0.f, "Release Ring Mode");
		configParam(R_PARAM, 0.9, 1.6, 1.2682, "Release Time");
		configParam<ChainParamQuantity>(RRINGATT_PARAM, 0.f, 0.2, 0.f, "Release Ring Attenuate");
		configParam(RRINGMODE_PARAM, 0.f, 1.f, 0.f, "Release Ring Mode");
		configParam(RCURVE_PARAM, 0.2, 7.4, 1.f, "Release Curve");
		configParam(ANGER_PARAM, 0.f, 1.f, .5, "Transistion Time Control");
		configParam(GLOBALRINGATT_PARAM, 0.f, 0.2, 0.f, "Gloal Ring Attenuate");
		configParam(GLOBALRINGOFFSET_PARAM, 0.f, 1.f, 1.f, "Global Ring Offset");
	}

	int stage = 0;
	bool isRunning = false;
	float modulation = 0.f;
	float phasor = 0.f;
	float env = 0.f;
	float output = 0.f;
	dsp::SchmittTrigger gateDetect;
	//Output Mode Triggers
	dsp::SchmittTrigger AOMDetect;
	dsp::SchmittTrigger DOMDetect;
	dsp::SchmittTrigger SOMDetect;
	dsp::SchmittTrigger ROMDetect;
	//Modulation Mode Triggers
	dsp::SchmittTrigger AMDetect;
	dsp::SchmittTrigger DMDetect;
	dsp::SchmittTrigger SMDetect;
	dsp::SchmittTrigger RMDetect;
	bool AOutMode = false;
	bool DOutMode = false;
	bool SOutMode = true;
	bool ROutMode = false;
	int AMMode = 0;
	int DMMode = 0;
	int SMMode = 0;
	int RMMode = 0;
	bool outputAlt = false; //Use negitive output as dry
	float lastValue = 0.f;
	float sus = 0.75;

	void displayActive(int lstage){
		lights[AGATE_LIGHT + 0].setBrightness(lstage == 0 ? 1.f : 0.f);
		lights[DGATE_LIGHT + 0].setBrightness(lstage == 1 ? 1.f : 0.f);
		lights[SGATE_LIGHT + 0].setBrightness(lstage == 2 ? 1.f : 0.f);
		lights[RGATE_LIGHT + 0].setBrightness(lstage == 3 ? 1.f : 0.f);
		outputs[AGATE_OUTPUT].setVoltage(lstage == 0 ? 10.f : 0.f);
		outputs[DGATE_OUTPUT].setVoltage(lstage == 1 ? 10.f : 0.f);
		outputs[SGATE_OUTPUT].setVoltage(lstage == 2 ? 10.f : 0.f);
		outputs[RGATE_OUTPUT].setVoltage(lstage == 3 ? 10.f : 0.f);
	}

	void forceAdvance(int lstage){
		if (inputs[lstage].isConnected()){
			if (inputs[lstage].getVoltage() >= 5.f){
				stage = lstage + 1;
				if (lstage == 0){
					// if the attack stage is skipped the envelope needs to be set high
					// so the decay stage has something to work with
					env = 1;
				}
			}
		}
		if (params[stage].getValue() >= .5f){
			stage = lstage + 1;
			if (lstage == 0){
				// if the attack stage is skipped the envelope needs to be set high
				// so the decay stage has something to work with
				env = 1;
			}
		}
		
	}

	void perStageOutput(int stage, bool mode){
		//accessing output and env as globals. This is probably frowed upon but oh well.
		if (stage != 0){ //the attack stage dosen't need to turn off the release stage's output
			if (outputs[stage-1].isConnected()){
				outputs[stage-1].setVoltage(0.f);
			}
		}
		if (outputs[stage].isConnected()){
			//TODO change mode to int, make modes: Plain wave, 
			if (mode){
				outputs[stage].setVoltage(10.f * output * params[GLOBALRINGOFFSET_PARAM].getValue());
			} else {
				outputs[stage].setVoltage(10.f * env);
			}
		}	
	}

	void setModeLight(int lstage, int lmode){
			int offset = lstage * 3;
			switch (lmode){
			case 0:
				lights[offset + 0].setBrightness(1.f);
				lights[offset + 1].setBrightness(0.f);
				lights[offset + 2].setBrightness(0.f);
				break;
			case 1:
				lights[offset + 0].setBrightness(0.f);
				lights[offset + 1].setBrightness(1.f);
				lights[offset + 2].setBrightness(0.f);
				break;
			case 2:
				lights[offset + 0].setBrightness(0.f);
				lights[offset + 1].setBrightness(0.f);
				lights[offset + 2].setBrightness(1.f);
				break;

			case 3:
				lights[offset + 0].setBrightness(1.f);
				lights[offset + 1].setBrightness(1.f);
				lights[offset + 2].setBrightness(0.f);
				break;
			
			default:
				lights[offset + 0].setBrightness(0.f);
				lights[offset + 1].setBrightness(0.f);
				lights[offset + 2].setBrightness(0.f);
				break;
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
			float anger = (simd::pow(params[ANGER_PARAM].getValue(),2)*8)+1;
			sus = params[S_PARAM].getValue();
			if (gate){
				switch (stage){
				case 0: // Attack
					phasor += simd::pow(.000315,params[A_PARAM].getValue());
					env = simd::pow(phasor,params[ACURVE_PARAM].getValue());

					if (phasor > 1.0){
						stage = 1;
					}

					//TODO Right now this is crossfading the modulation signal input. This would be okay if the modulation
					// method were the same on each stage, but if one stage is RM and the other Add, this probably fails
					// to make smooth transitions due to RM and ADD leading to different signal amplitudes. A quick test
					// makes it seem like this isn't a problem, but I'm not sure.

					//TODO Normal modulation inputs going down

					if (inputs[AMOD_INPUT].isConnected()){
						modulation = simd::crossfade(inputs[AMOD_INPUT].getVoltage() * params[ARINGATT_PARAM].getValue(),
													inputs[DMOD_INPUT].getVoltage() * params[DRINGATT_PARAM].getValue(),
													(simd::fmax(0,anger*env-anger+1)));
						switch (AMMode){
						case 0: // Ring
							output = modulation * env + env;
							break;
						case 1: // Addition
							output = modulation + env;
							break;
						case 3: // Self-Env Addition
							//TODO this might not be working, or be working to well? - it seems to be the same as basic ring
							if (env <= 0.1){ //first 10% of Attack stage
								output = (modulation * env * 10) + env;
							} else{
								output = modulation + env;
							}
							break;
						case 4: // Non offset Ring
							//This is only very sightly different from additon, but creates smoother transitions at start of env
							// Might want to make this the Mode 3?
							output = modulation * env;
							break;
						default:
							output = modulation * env + env;
							break;
						}
					} else{
						modulation = simd::crossfade(0.f,
							inputs[DMOD_INPUT].getVoltage() * params[DRINGATT_PARAM].getValue(),
							(simd::fmax(0,anger*env-anger+1)));
						output = env;
					}

					displayActive(0);
					perStageOutput(0,AOutMode);
					forceAdvance(0); //checks if the force advance is true internally
					
					break;
				case 1: // Decay
					phasor -= simd::pow(.000315,params[D_PARAM].getValue());
					//TODO figure out the math for this curve
					//env = simd::pow(phasor,params[DCURVE_PARAM].getValue());

					//using bezier curves (ish) because nothing else wanted to work
					//p0 is starting point, p2 is the ending point, p1 is the control point
					//t is the phasor from 0 to 1
					// p1+(1-t)^2*(p0-p1)+t^2*(p2-p1)
					//TODO this works BUT the DCURVE_PARAM needs to be scaled such that the minimum value is the sustain level.
					// there's also the problem of chaning the sustain level changes the slope. But it's closer.
					env = params[DCURVE_PARAM].getValue()+simd::pow((1-phasor),2)*(sus-params[DCURVE_PARAM].getValue())+simd::pow(phasor,2)*(1-params[DCURVE_PARAM].getValue());

					if (phasor <= 0){
						stage = 2;
					}

					if (inputs[DMOD_INPUT].isConnected()){
						//modulation with xfade, envelope gets inverted to make it similar to the attack envelope
						modulation = simd::crossfade(inputs[DMOD_INPUT].getVoltage() * params[DRINGATT_PARAM].getValue(),
													inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue(),
													(simd::fmax(0,anger*(-env)-anger+(1/(sus+0.0001)))));
						switch (DMMode){
						case 0: // Ring
							output = modulation * env + env;
							break;
						case 1: // Addition
							output = modulation + env;
							break;
						case 3: // Self-Env Addition
							//TODO this doesn't seem to be working
							if ((-env + 1 * (1/sus)) <= 0.1){ //first 10% of decay stage
								output = (modulation * env * 10) + env;
							} else{
								output = modulation + env;
							}
							break;
						case 4: // Non offset Ring
							//TODO this might be the same as basic addition?
							output = modulation * env;
							break;
						default:
							output = modulation * env + env;
							break;
						}
					} else{
						modulation = simd::crossfade(0.f,
													inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue(),
													(simd::fmax(0,anger*(-env)-anger+(1/(sus+0.0001)))));
						output = modulation * env + env;
					}


					displayActive(1);
					perStageOutput(1,DOutMode);
					forceAdvance(1); //checks if the force advance is true internally

					break;
				case 2: // Sustain
					env = sus;
					phasor = sus;
					if (inputs[SMOD_INPUT].isConnected()){
						output = (inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue()) * env + env;
					} else{
						output = env;
					}
					displayActive(2);
					perStageOutput(2,SOutMode);
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
				phasor -= simd::pow(.000315,params[R_PARAM].getValue());
				env = simd::pow(phasor*(1/sus),params[RCURVE_PARAM].getValue())*sus;

				displayActive(3);

				if (inputs[RMOD_INPUT].isConnected()){
					modulation = simd::crossfade(inputs[RMOD_INPUT].getVoltage() * params[RRINGATT_PARAM].getValue(),
												inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue(),
												(simd::fmax(0,anger*(-env)-anger+(1/(sus+0.0001)))));
					switch (RMMode){
					case 0: // Ring
						output = modulation * env + env;
						break;
					case 1: // Addition
						output = modulation + env;
						break;
					case 3: // Self-Env Addition
						//TODO This doesn't seem to be working
						if ((-env + sus * (1/sus)) <= 0.1){ //first 10% of release stage
							output = (modulation * env * 10) + env;
						} else{
							output = modulation + env;
						}
						break;
					case 4: // Non offset Ring
						//TODO This might be the same as basic addition
						output = modulation * env;
						break;
					default:
						output = modulation * env + env;
						break;
					}
				} else{
					modulation = simd::crossfade(0.f,
												inputs[SMOD_INPUT].getVoltage() * params[SRINGATT_PARAM].getValue(),
												(simd::fmax(0,anger*(-env)-anger+(1/(sus+0.0001)))));
					output = modulation * env + env;
				}

				if (phasor <= 0){
					env = 0.f;
					phasor = 0.f;
					output = 0.f;
					outputs[RGATE_OUTPUT].setVoltage(0.f);
					isRunning = false;
				}

				perStageOutput(3,ROutMode);
			}

			//Output
			if (inputs[GLOBALRING_INPUT].isConnected()){
				if (outputs[MAINOUTP_OUTPUT].isConnected()){
					outputs[MAINOUTP_OUTPUT].setVoltage(output * 10.f * (inputs[GLOBALRING_INPUT].getVoltage() * params[GLOBALRINGATT_PARAM].getValue() + params[GLOBALRINGOFFSET_PARAM].getValue()));
				}
				if (outputs[MAINOUTM_OUTPUT].isConnected()){
					if (outputAlt){
						//Right Click menu option
						outputs[MAINOUTM_OUTPUT].setVoltage(env * 10.f);
					} else{
						outputs[MAINOUTM_OUTPUT].setVoltage(-1.f * output * 10.f * (inputs[GLOBALRING_INPUT].getVoltage() * params[GLOBALRINGATT_PARAM].getValue() + params[GLOBALRINGOFFSET_PARAM].getValue()));
					}
				}
			} else{
				// If no ring input connected, the offset knob works as a volume knob, to add more headroom when necessary
				if (outputs[MAINOUTP_OUTPUT].isConnected()){
					outputs[MAINOUTP_OUTPUT].setVoltage(output * (10.f * params[GLOBALRINGOFFSET_PARAM].getValue()));
				}
				if (outputs[MAINOUTM_OUTPUT].isConnected()){
					if (outputAlt){
						//Right click menu option
						outputs[MAINOUTM_OUTPUT].setVoltage(env * 10.f);
					} else{
						outputs[MAINOUTM_OUTPUT].setVoltage(output * (-10.f * params[GLOBALRINGOFFSET_PARAM].getValue()));
					}
				}
			}
			
		}
		if (processDivider.process()){
			//TODO Set default value to the value of the value of the last mod input with a connection
			//VCV-Vortico: Subclass ParamQuantity and override float Quantity::getDefaultValue().
			// Call configParam<MyParamQuantity>(...) to use your subclass instead of the default class.

			if (inputs[AMOD_INPUT].isConnected()){
				lastValue = params[ARINGATT_PARAM].getValue();
			}
			if (inputs[DMOD_INPUT].isConnected()){
				lastValue = params[ARINGATT_PARAM].getValue();
			}
			//configParam<MyParamQuantity>(DRINGATT_PARAM, 0.f, 0.2, lastValue, "Delay Ring Attenuate");
			if (inputs[SMOD_INPUT].isConnected()){
				lastValue = params[ARINGATT_PARAM].getValue();
			}
			//configParam<MyParamQuantity>(SRINGATT_PARAM, 0.f, 0.2, lastValue, "Sustain Ring Attenuate");
			//configParam<MyParamQuantity>(RRINGATT_PARAM, 0.f, 0.2, lastValue, "Release Ring Attenuate");

			//Yes, I realize it's bad style to not put these in a loop but ¯\_(ツ)_/¯
			if (AOMDetect.process(params[AOUTMODE_PARAM].getValue())) {
				AOutMode = !AOutMode;
				lights[AGATE_LIGHT + 2].setBrightness(AOutMode ? 1.f : 0.f);
			}
			if (DOMDetect.process(params[DOUTMODE_PARAM].getValue())) {
				DOutMode = !DOutMode;
				lights[DGATE_LIGHT + 2].setBrightness(DOutMode ? 1.f : 0.f);
			}
			if (SOMDetect.process(params[SOUTMODE_PARAM].getValue())) {
				SOutMode = !SOutMode;
				lights[SGATE_LIGHT + 2].setBrightness(SOutMode ? 1.f : 0.f);
			}
			if (ROMDetect.process(params[ROUTMODE_PARAM].getValue())) {
				ROutMode = !ROutMode;
				lights[RGATE_LIGHT + 2].setBrightness(ROutMode ? 1.f : 0.f);
			}
			// toggle states for Modulation Modes, update mode LED respectively
			if (AMDetect.process(params[ARINGMODE_PARAM].getValue())) {
				AMMode = (AMMode + 1)%4;
				setModeLight(0,AMMode);
			}
			if (DMDetect.process(params[DRINGMODE_PARAM].getValue())) {
				DMMode = (DMMode + 1)%4;
				setModeLight(1,DMMode);
			}
			if (SMDetect.process(params[SRINGMODE_PARAM].getValue())) {
				SMMode = (SMMode + 1)%4;
				setModeLight(2,SMMode);
			}
			if (RMDetect.process(params[RRINGMODE_PARAM].getValue())) {
				RMMode = (RMMode + 1)%4;
				setModeLight(3,RMMode);
			}
		}
	}

	void onAdd() override {
		//Set the modulation mode LEDS to inital value at startup
		lights[AMODE_LIGHT + 0].setBrightness(1.f);
		lights[DMODE_LIGHT + 0].setBrightness(1.f);
		lights[SMODE_LIGHT + 0].setBrightness(1.f);
		lights[RMODE_LIGHT + 0].setBrightness(1.f);
		//Set the outputmode LEDs to inital value at startup
		lights[AGATE_LIGHT + 2].setBrightness(0.f);
		lights[DGATE_LIGHT + 2].setBrightness(0.f);
		lights[SGATE_LIGHT + 2].setBrightness(1.f);
		lights[RGATE_LIGHT + 2].setBrightness(0.f);
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

	void appendContextMenu(Menu *menu) override{
		Vega *vega = dynamic_cast<Vega *>(module);
		assert(vega);

		struct VegaOutputAltItem : MenuItem{
            Vega *vega;

            void onAction(const event::Action &e) override
            {
                vega->outputAlt = !vega->outputAlt;
            }
            void step() override
            {
                rightText = CHECKMARK(vega->outputAlt);
            }
        };

		menu->addChild(new MenuEntry);
		VegaOutputAltItem *altOutput = createMenuItem<VegaOutputAltItem>("Negitive Out Dry");
        altOutput->vega = vega;
		menu->addChild(altOutput);
	}
};

Model* modelVega = createModel<Vega, VegaWidget>("Vega");