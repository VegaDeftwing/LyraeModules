#include "plugin.hpp"

struct Sulafat : Module {
	enum ParamIds {
		KNOB_PARAM,
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
		configParam(KNOB_PARAM, 0.f, 7.f, 0.f, "");
		processDivider.setDivision(64);
	}

	int mode = 0;
	float phase1 = 0.f;
	float phase2 = 0.f;
	float pitch = 0.f;

	float clamp(float n, float lower, float upper) {
  		return std::max(lower, std::min(n, upper));
	}

	void process(const ProcessArgs& args) override {
		// Generate Interal LFOs
		phase1 += dsp::FREQ_C4 / 300 * args.sampleTime * std::pow(2.f, pitch);
		phase2 += dsp::FREQ_C4 / 220 * args.sampleTime;
		if (phase1 >= .5f){
			phase1 = -.5f;
		}
		if (phase2 >= .5f){
			phase2 = -.5f;
		}
		float sine1 = std::sin(2.f * M_PI * phase1);
		float sine2 = std::sin(2.f * M_PI * phase2);

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
			outputs[LEFT_OUTPUT].setVoltage(fmod(inputs[LEFT_INPUT].getVoltage()+(sine2*.15),3) * 1.66);
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(fmod(inputs[RIGHT_INPUT].getVoltage()+(sine1*.15),3) * 1.66);
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(fmod(inputs[LEFT_INPUT].getVoltage()+(sine1*.15),3.3) * 1.51);
			}
			break;
		case 2: //Direct modulo, with modulation. This Basically does a S&H effect on top of the wave folder
			outputs[LEFT_OUTPUT].setVoltage((float)((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % 3) * 2.5);
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(((int)(inputs[RIGHT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25)) % 3) * 2.5);
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25)) % 3) * 2.5);
			}
			break;
		case 3: //Tangent + Clamping = Wavefolding? Again?
			outputs[LEFT_OUTPUT].setVoltage(clamp((std::tan(.25f * inputs[LEFT_INPUT].getVoltage() - 2.f + (sine1*.15) )),-5.f,5.f));
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[RIGHT_OUTPUT].setVoltage(clamp((std::tan(.25f * inputs[RIGHT_INPUT].getVoltage() - 2.f + (sine2*.15) )),-5.f,5.f));
			} else{
				outputs[RIGHT_OUTPUT].setVoltage(clamp((std::tan(.25f * inputs[LEFT_INPUT].getVoltage() - 2.f + (sine2*.15) )),-5.f,5.f));
			}
			break;
		case 4: // Combine modes 1 & 2 on each half of the wave, filter mode 2 a bit to keep it from dominating the output

			lowpassFilter.setCutoffFreq(1000 / args.sampleRate);

			if (inputs[LEFT_INPUT].getVoltage() > 0){
				float filterMe = (float)((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % 3) * 2.5;
				lowpassFilter.process(filterMe);
				filterMe = lowpassFilter.lowpass();
				outputs[LEFT_OUTPUT].setVoltage(filterMe);
			} else{
				outputs[LEFT_OUTPUT].setVoltage(fmod(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35),3) * 1.66);
			}

			if (inputs[RIGHT_INPUT].isConnected()){
				if (inputs[RIGHT_INPUT].getVoltage() < 0){
					float filterMe = (float)((int)(inputs[RIGHT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % 3) * 2.5;
					lowpassFilter.process(filterMe);
					filterMe = lowpassFilter.lowpass();
					outputs[RIGHT_OUTPUT].setVoltage(filterMe);
				} else{
					outputs[RIGHT_OUTPUT].setVoltage(fmod(inputs[RIGHT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25),3) * 1.66);
				}
			} else{
				if (inputs[LEFT_INPUT].getVoltage() < 0){
					float filterMe = (float)((int)(inputs[LEFT_INPUT].getVoltage()+(sine1*.25)+(sine2*.35)) % 3) * 2.5;
					lowpassFilter.process(filterMe);
					filterMe = lowpassFilter.lowpass();
					outputs[RIGHT_OUTPUT].setVoltage(filterMe);
				} else{
					outputs[RIGHT_OUTPUT].setVoltage(fmod(inputs[LEFT_INPUT].getVoltage()+(sine1*.35)+(sine2*.25),3) * 1.66);
				}
			}
			break;
			
		case 5: // Simple Ring-Mod + modulation, if no right input, use LFOs
			if (inputs[RIGHT_INPUT].isConnected()){
				outputs[LEFT_OUTPUT].setVoltage(clamp(inputs[LEFT_INPUT].getVoltage() * (inputs[RIGHT_INPUT].getVoltage() -5.f + (sine1*.35)-(sine2*.25)),-5.f,5.f));
				outputs[RIGHT_OUTPUT].setVoltage(clamp(inputs[RIGHT_INPUT].getVoltage() * (inputs[LEFT_INPUT].getVoltage() -5.f + (sine1*.25)-(sine2*.35)) ,-5.f,5.f));
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
				if (rack::math::isEven((int)inputs[LEFT_INPUT].getVoltage())){
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
			pitch = params[KNOB_PARAM].getValue();
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

		addParam(createParamCentered<HexKnob>(mm2px(Vec(5.08, 69.693)), module, Sulafat::KNOB_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 12.4)), module, Sulafat::LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 22.26)), module, Sulafat::RIGHT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 104.406)), module, Sulafat::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.08, 114.266)), module, Sulafat::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 51.223)), module, Sulafat::LED1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 54.703)), module, Sulafat::LED2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 58.183)), module, Sulafat::LED3_LIGHT));
	}
};


Model* modelSulafat = createModel<Sulafat, SulafatWidget>("Sulafat");