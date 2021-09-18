#include "plugin.hpp"

using simd::float_4;
struct Sulafat : Module {
	enum ParamIds {
		KNOB_PARAM,
		PARAM_LFO1,
		PARAM_LFO2,
		PARAM_FLEFT,
		PARAM_FRIGHT,
		NUM_PARAMS
	};
	enum InputIds {
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
		LED1_LIGHT,
		LED2_LIGHT,
		LED3_LIGHT,
		NUM_LIGHTS
	};

	dsp::ClockDivider processDivider;
	dsp::RCFilter lowpassFilter;

	Sulafat() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(KNOB_PARAM, 0.f, 7.f, 0.f, "Mode Selection");
		configParam(PARAM_LFO1, 0.0f, 1.0f, 0.f, "LFO 1 SPEED", "%", 0.f, 100.f);
		configParam(PARAM_LFO2, 0.0f, 1.0f, 0.05, "LFO 2 SPEED", "%", 0.f, 100.f);
		configParam(PARAM_FLEFT, 10.0f, 0.0f, 3.f, "Fold Left", "", 0.f, 1.f);
		configParam(PARAM_FRIGHT, 10.0f, 0.0f, 3.3, "Fold Right", "", 0.f, 1.f);
		processDivider.setDivision(64);
	}

	int mode = 0;
	float phase1 = 0.f;
	float phase2 = 0.f;
	float lfo1speed = 0.f;
	float lfo2speed = 0.f;
	float foldl = 0.f;
	float foldr = 0.f;

	void process(const ProcessArgs& args) override {
		// Generate Interal LFOs
		lfo1speed = params[PARAM_LFO1].getValue();
		lfo2speed = params[PARAM_LFO2].getValue();
		foldl = params[PARAM_FLEFT].getValue();
		foldr = params[PARAM_FRIGHT].getValue();

		phase1 += dsp::FREQ_C4 / 300 * args.sampleTime * simd::pow(2.f, lfo1speed * 7);
		phase2 += dsp::FREQ_C4 / 300 * args.sampleTime * simd::pow(2.f, lfo2speed * 7);
		if (phase1 >= .5f){
			phase1 = -.5f;
		}
		if (phase2 >= .5f){
			phase2 = -.5f;
		}
		float sine1 = simd::sin(2.f * M_PI * phase1);
		float sine2 = simd::sin(2.f * M_PI * phase2);

		// All The Modes
		switch (mode)
		{
		case 0: //Bypass mode
			outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage());
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
			}
			break;
		case 1: //Wavefolder with DC offset modulation
			outputs[LEFT_OUTPUT].setVoltage(simd::fmod(inputs[LEFT_INPUT].getVoltage()+(sine2*.15),foldl) * 1.66);
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(simd::fmod(inputs[RIGHT_INPUT].getVoltage()+(sine1*.15),foldr) * 1.66);
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(simd::fmod(inputs[LEFT_INPUT].getVoltage()+(sine1*.15),foldr) * 1.51);
			}
			break;
		case 2: //Direct modulo, with modulation. This Basically does a S&H effect on top of the wave folder
			outputs[LEFT_OUTPUT].setVoltage((float)((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % (int)foldl) * 2.5);
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(((int)(inputs[RIGHT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25)) % (int)foldr) * 2.5);
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25)) % (int)foldr) * 2.5);
			}
			break;
		case 3: //Tangent + clamping = Wavefolding? Again?
			outputs[LEFT_OUTPUT].setVoltage(simd::clamp((simd::tan(.25f * inputs[LEFT_INPUT].getVoltage() - 2.f + (sine1*.15) )),-5.f,5.f));
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(simd::clamp((simd::tan(.25f * inputs[RIGHT_INPUT].getVoltage() - 2.f + (sine2*.15) )),-5.f,5.f));
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(simd::clamp((simd::tan(.25f * inputs[LEFT_INPUT].getVoltage() - 2.f + (sine2*.15) )),-5.f,5.f));
			}
			break;
		case 4: // Combine modes 1 & 2 on each half of the wave, filter mode 2 a bit to keep it from dominating the output

			lowpassFilter.setCutoffFreq(1000 / args.sampleRate);

			if (inputs[LEFT_INPUT].getVoltage() > 0){
				float filterMe = (float)((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % (int)foldl) * 2.5;
				lowpassFilter.process(filterMe);
				filterMe = lowpassFilter.lowpass();
				outputs[LEFT_OUTPUT].setVoltage(filterMe);
			} else{
				outputs[LEFT_OUTPUT].setVoltage(simd::fmod(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35),foldl) * 1.66);
			}

			if (inputs[RIGHT_INPUT].isConnected()){
				if (inputs[RIGHT_INPUT].getVoltage() < 0){
					float filterMe = (float)((int)(inputs[RIGHT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % (int)foldr) * 2.5;
					lowpassFilter.process(filterMe);
					filterMe = lowpassFilter.lowpass();
					outputs[RIGHT_OUTPUT].setVoltage(filterMe);
				} else{
					outputs[RIGHT_OUTPUT].setVoltage(simd::fmod(inputs[RIGHT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25),foldr) * 1.66);
				}
			} else{
				if (inputs[LEFT_INPUT].getVoltage() < 0){
					float filterMe = (float)((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % (int)foldr) * 2.5;
					lowpassFilter.process(filterMe);
					filterMe = lowpassFilter.lowpass();
					outputs[RIGHT_OUTPUT].setVoltage(filterMe);
				} else{
					outputs[RIGHT_OUTPUT].setVoltage(simd::fmod(inputs[LEFT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25),foldr) * 1.66);
				}
			}
			break;
			
		case 5: // Simple Ring-Mod + modulation, if no right input, use LFOs
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[LEFT_OUTPUT].setVoltage(simd::clamp(inputs[LEFT_INPUT].getVoltage() * (inputs[RIGHT_INPUT].getVoltage() -5.f + (sine1*.35)-(sine2*.25)),-5.f,5.f));
				outputs[RIGHT_OUTPUT].setVoltage(simd::clamp(inputs[RIGHT_INPUT].getVoltage() * (inputs[LEFT_INPUT].getVoltage() -5.f + (sine1*.25)-(sine2*.35)) ,-5.f,5.f));
			} else {
				outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() * (sine2*.35)+(sine1*.75));
				outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() * (-sine1*.75)+(sine2*.35) );
			}
			break;
			
		case 6: // This is very stupid, but it's fun.

			if (rack::math::isOdd((int)inputs[LEFT_INPUT].getVoltage())){
				outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
			}

			if (inputs[RIGHT_INPUT].isConnected()){
				if (rack::math::isOdd((int)inputs[LEFT_INPUT].getVoltage())){
					outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage());
				}
			} else{
				//This was isEven until 1.0.2, but I discovered that made it just pass the input directly. Oops.
				if (rack::math::isOdd((int)inputs[LEFT_INPUT].getVoltage())){
					outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
				}
			}
			break;

		case 7: //Ringmod w/ LFOs but also just do the raw output with isOdd, this gives a really weird output
				outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() * (sine2*.35)+(sine1*.75));
				outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() * (-sine1*.75)+(sine2*.35));
				if (rack::math::isOdd((int)inputs[LEFT_INPUT].getVoltage())){
					outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
					outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
				}

			break;
		case 8: //No input, output the LFO's, make knob frequency
			outputs[LEFT_OUTPUT].setVoltage(sine1*5.f);
			outputs[RIGHT_OUTPUT].setVoltage(sine1*sine2*5.f);
			break;

		default:
			mode = 0;
			break;
		}
	
		if (processDivider.process()){
			mode = (int)params[KNOB_PARAM].getValue();
			lights[LED1_LIGHT].setBrightness(mode & 0b001 ? 1.f : 0.f);
			lights[LED2_LIGHT].setBrightness(mode & 0b010 ? 1.f : 0.f);
			lights[LED3_LIGHT].setBrightness(mode & 0b100 ? 1.f : 0.f);
			if (!inputs[LEFT_INPUT].isConnected())
				mode = 8;
		}
	}
};


struct SulafatWidget : ModuleWidget {
	SulafatWidget(Sulafat* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Sulafat.svg")));

		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH, + 5)));
		addChild(createWidget<Bolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 5, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<SnappingHexKnob>(mm2px(Vec(5.08, 69.693)), module, Sulafat::KNOB_PARAM));

		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 12.4)), module, Sulafat::LEFT_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 22.26)), module, Sulafat::RIGHT_INPUT));

		addOutput(createOutputCentered<OutJack>(mm2px(Vec(5.08, 104.406)), module, Sulafat::LEFT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(5.08, 114.266)), module, Sulafat::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 51.223)), module, Sulafat::LED1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 54.703)), module, Sulafat::LED2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 58.183)), module, Sulafat::LED3_LIGHT));
	}

	void appendContextMenu(Menu *menu) override{
		Sulafat *sulafat = dynamic_cast<Sulafat *>(module);
		assert(sulafat);

		struct LFO1Slider : ui::Slider {
			LFO1Slider(Sulafat *module) {
				box.size.x = 180.0f;
				quantity = module->paramQuantities[Sulafat::PARAM_LFO1];
			}
		};

		struct LFO2Slider : ui::Slider {
			LFO2Slider(Sulafat *module) {
				box.size.x = 180.0f;
				quantity = module->paramQuantities[Sulafat::PARAM_LFO2];
			}
		};

		struct FLSlider : ui::Slider {
			FLSlider(Sulafat *module) {
				box.size.x = 180.0f;
				quantity = module->paramQuantities[Sulafat::PARAM_FLEFT];
			}
		};

		struct FRSlider : ui::Slider {
			FRSlider(Sulafat *module) {
				box.size.x = 180.0f;
				quantity = module->paramQuantities[Sulafat::PARAM_FRIGHT];
			}
		};

		menu->addChild(new MenuEntry);
		menu->addChild(new LFO1Slider(sulafat));
		menu->addChild(new LFO2Slider(sulafat));
		menu->addChild(new FLSlider(sulafat));
		menu->addChild(new FRSlider(sulafat));
	}

};




Model* modelSulafat = createModel<Sulafat, SulafatWidget>("Sulafat");