#include "plugin.hpp"

struct Delta : Module {
	enum ParamIds {
		R1A_PARAM,
		R1O_PARAM,
		R2A_PARAM,
		R2O_PARAM,
		R3A_PARAM,
		R3O_PARAM,
		MANUALCLOCK_PARAM,
		RMLGATEMANUAL_PARAM,
		RMLGATEHOLD_PARAM,
		LMRGATEMANUAL_PARAM,
		LMRGATEHOLD_PARAM,
		SELECT_PARAM,
		RML_PARAM,
		LMR_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		LEFT_INPUT,
		RIGHT_INPUT,
		R1_INPUT,
		R2_INPUT,
		R3_INPUT,
		CLOCK_INPUT,
		CS1_INPUT,
		CS2_INPUT,
		CS3_INPUT,
		RMLGATE_INPUT,
		LMRGATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		LEFT_OUTPUT,
		RIGHT_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STAGE_LIGHT, 3),
		LMR_LIGHT,
		RML_LIGHT,
		NUM_LIGHTS
	};

	Delta() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(R1A_PARAM, 0.f, 1.f, 0.f, "Near Stage Attenuation");
		configParam(R1O_PARAM, 0.f, 1.f, 0.f, "Near Stage Offset");
		configParam(R2A_PARAM, 0.f, 1.f, 0.f, "Mid Stage Attenuation");
		configParam(R2O_PARAM, 0.f, 1.f, 0.f, "Mid Stage Offset");
		configParam(R3A_PARAM, 0.f, 1.f, 0.f, "Far Stage Attenuation");
		configParam(R3O_PARAM, 0.f, 1.f, 0.f, "Far Stage Offset");
		configParam(MANUALCLOCK_PARAM, 0.f, 1.f, 0.f, "Clock Advance");
		configParam(RMLGATEMANUAL_PARAM, 0.f, 1.f, 0.f, "Momentary RML");
		configParam(RMLGATEHOLD_PARAM, 0.f, 1.f, 0.f, "Toggle RML");
		configParam(LMRGATEMANUAL_PARAM, 0.f, 1.f, 0.f, "Momentar LMR");
		configParam(LMRGATEHOLD_PARAM, 0.f, 1.f, 0.f, "Toggle LMR");
		configParam(SELECT_PARAM, 0.f, 2.f, 0.f, "Clock Select State");
		configParam(LMR_PARAM, 0.f, 1.f, 0.f, "LMR State");
		configParam(RML_PARAM, 0.f, 1.f, 0.f, "RML State");
	}

	float R1T = 0.f;
	float R2T = 0.f;
	float R3T = 0.f;
	float BLOCK2 = 0.f;
	dsp::SchmittTrigger manualselect;
	dsp::SchmittTrigger clockedselect;
	dsp::SchmittTrigger RMLst;
	dsp::SchmittTrigger LMRst;
	dsp::SchmittTrigger cvRMLst;
	dsp::SchmittTrigger cvLMRst;
	//bool RMLgate = false;
	//bool LMRgate = false;
	float STAGE2OUTL = 0.f;
	float STAGE2OUTR = 0.f;
	float STAGE3OUTL = 0.f;
	float STAGE3OUTR = 0.f;
	bool lRMLgate = false;
	bool lLMRgate = false;
	bool init = false;

	void process(const ProcessArgs& args) override {
		if (!init){
			init = true;
			lRMLgate = params[RML_PARAM].getValue();
			lLMRgate = params[LMR_PARAM].getValue();
		}
		
		//BLOCK 1
		if (inputs[R1_INPUT].isConnected()){
			R1T = inputs[R1_INPUT].getVoltage() * params[R1A_PARAM].getValue() + params[R1O_PARAM].getValue();
		} else{
			R1T = 1.f;
		}
		
		if (inputs[R2_INPUT].isConnected()){
			R2T = inputs[R2_INPUT].getVoltage() * params[R2A_PARAM].getValue() + params[R2O_PARAM].getValue();
		} else{
			R2T = 1.f;
		}
		
		if (inputs[R3_INPUT].isConnected()){
			R3T = inputs[R3_INPUT].getVoltage() * params[R3A_PARAM].getValue() + params[R3O_PARAM].getValue();
		} else{
			R3T = 1.f;
		}

		float BLOCK1 = R1T * R2T * R3T;
		float STAGE1OUTL = (inputs[LEFT_INPUT].getVoltage() * -BLOCK1)/10.f;
		float STAGE1OUTR = (inputs[RIGHT_INPUT].getVoltage() * BLOCK1)/10.f;

		//BLOCK2 - clocksel or phase sel CS1,CS2,CS3 with xfade
		if (inputs[CS1_INPUT].isConnected() || inputs[CS2_INPUT].isConnected() || inputs[CS3_INPUT].isConnected() ){
			if(manualselect.process(params[MANUALCLOCK_PARAM].getValue())){
				params[SELECT_PARAM].setValue(((int)params[SELECT_PARAM].getValue() + 1)%3);
			}
			if(clockedselect.process(inputs[CLOCK_INPUT].getVoltage())){
				params[SELECT_PARAM].setValue(((int)params[SELECT_PARAM].getValue() + 1)%3);
			}
			
			switch ((int)params[SELECT_PARAM].getValue()){
			case 0:
				BLOCK2 = inputs[CS1_INPUT].getVoltage();
				lights[STAGE_LIGHT + 0].setBrightness(1.f);
				lights[STAGE_LIGHT + 1].setBrightness(0.f);
				lights[STAGE_LIGHT + 2].setBrightness(0.f);
				break;
			case 1:
				BLOCK2 = inputs[CS2_INPUT].getVoltage();
				lights[STAGE_LIGHT + 0].setBrightness(0.f);
				lights[STAGE_LIGHT + 1].setBrightness(1.f);
				lights[STAGE_LIGHT + 2].setBrightness(0.f);
				break;
			case 2:
				BLOCK2 = inputs[CS3_INPUT].getVoltage();
				lights[STAGE_LIGHT + 0].setBrightness(0.f);
				lights[STAGE_LIGHT + 1].setBrightness(0.f);
				lights[STAGE_LIGHT + 2].setBrightness(1.f);
				break;
			default:
				BLOCK2 = 0.f;
				break;
			}
			STAGE2OUTL = STAGE1OUTL * BLOCK2;
			STAGE2OUTR = STAGE1OUTR * -BLOCK2;
		} else {
			STAGE2OUTL = STAGE1OUTL * 10.f;
			STAGE2OUTR = STAGE1OUTR * 10.f;
			lights[STAGE_LIGHT + 0].setBrightness(0.f);
			lights[STAGE_LIGHT + 1].setBrightness(0.f);
			lights[STAGE_LIGHT + 2].setBrightness(0.f);
		}

		//BLOCK3 - gate'd X-MOD of channels
		if (params[RMLGATEMANUAL_PARAM].getValue()){
			params[RML_PARAM].setValue(!lRMLgate);
		} else { params[RML_PARAM].setValue(lRMLgate); lRMLgate = params[RML_PARAM].getValue();}

		if (params[LMRGATEMANUAL_PARAM].getValue()){
			params[LMR_PARAM].setValue(!lLMRgate);
		} else { params[LMR_PARAM].setValue(lLMRgate); lLMRgate = params[LMR_PARAM].getValue(); }

		if(RMLst.process(params[RMLGATEHOLD_PARAM].getValue())){
			lRMLgate = !lRMLgate;
		}
		if(LMRst.process(params[LMRGATEHOLD_PARAM].getValue())){
			lLMRgate = !lLMRgate;
		}

		if(cvRMLst.process(inputs[RMLGATE_INPUT].getVoltage())){
			lRMLgate = !lRMLgate;
		}
		if(cvLMRst.process(inputs[LMRGATE_INPUT].getVoltage())){
			lLMRgate = !lLMRgate;
		}

		if((int)params[RML_PARAM].getValue()){
			STAGE3OUTL = -STAGE2OUTL * STAGE2OUTR / 5.f;
			lights[RML_LIGHT].setBrightness(1.f);
		} else{ 
			STAGE3OUTL = STAGE2OUTL;
			lights[RML_LIGHT].setBrightness(0.f);
		}

		
		if((int)params[LMR_PARAM].getValue()){ 
			STAGE3OUTR = STAGE2OUTR * STAGE2OUTL / 5.f ;
			lights[LMR_LIGHT].setBrightness(1.f);
		} else{
			STAGE3OUTR = STAGE2OUTR;
			lights[LMR_LIGHT].setBrightness(0.f);
		}

		outputs[LEFT_OUTPUT].setVoltage(simd::atan(simd::atan(STAGE3OUTL/10.f))*10.f);
		outputs[RIGHT_OUTPUT].setVoltage(simd::atan(simd::atan(STAGE3OUTR/10.f))*10.f);
	}
};


struct DeltaWidget : ModuleWidget {
	DeltaWidget(Delta* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/delta1.svg")));

		addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH - 13, + 5)));
		addChild(createWidget<Bolt>(Vec(78, 5)));
		addChild(createWidget<Bolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 14.5, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(15.24, 27.932)), module, Delta::R1A_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(25.4, 27.932)), module, Delta::R1O_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(15.24, 38.092)), module, Delta::R2A_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(25.4, 38.092)), module, Delta::R2O_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(15.24, 48.252)), module, Delta::R3A_PARAM));
		addParam(createParamCentered<MedHexKnob>(mm2px(Vec(25.4, 48.252)), module, Delta::R3O_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(25.4, 63.413)), module, Delta::MANUALCLOCK_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(15.24, 88.732)), module, Delta::RMLGATEMANUAL_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(25.4, 88.732)), module, Delta::RMLGATEHOLD_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(15.24, 98.892)), module, Delta::LMRGATEMANUAL_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(25.4, 98.892)), module, Delta::LMRGATEHOLD_PARAM));

		addInput(createInputCentered<InJack>(mm2px(Vec(10.16, 7.08)), module, Delta::LEFT_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(20.312, 7.08)), module, Delta::RIGHT_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 27.932)), module, Delta::R1_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 38.092)), module, Delta::R2_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 48.252)), module, Delta::R3_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(15.24, 63.413)), module, Delta::CLOCK_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 73.572)), module, Delta::CS1_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(15.24, 73.573)), module, Delta::CS2_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(25.4, 73.573)), module, Delta::CS3_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 88.732)), module, Delta::RMLGATE_INPUT));
		addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 98.892)), module, Delta::LMRGATE_INPUT));

		addOutput(createOutputCentered<OutJack>(mm2px(Vec(14.795, 119.205)), module, Delta::LEFT_OUTPUT));
		addOutput(createOutputCentered<OutJack>(mm2px(Vec(24.947, 119.205)), module, Delta::RIGHT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(mm2px(Vec(5.08, 63.413)), module, Delta::STAGE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.25, 88.732)), module, Delta::RML_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(20.25, 98.892)), module, Delta::LMR_LIGHT));
	}
};


Model* modelDelta = createModel<Delta, DeltaWidget>("Delta");