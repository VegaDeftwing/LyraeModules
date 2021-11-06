#include "plugin.hpp"
#include "Vega.hpp"

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
	
	Vega* findHostModulePtr(Module* module){
		if (module){
			if (module->rightExpander.module){
				if (module->rightExpander.module->model == modelVega){
					return reinterpret_cast<Vega*>(module->rightExpander.module);
				}
			}else{
				return nullptr;
			}
		}
		
	}


	void process(const ProcessArgs& args) override {
		//connect to right expander module
		Vega * mother = findHostModulePtr(this);

		if (mother)
		{
			//TIME controls
			if (inputs[A_INPUT].isConnected()){
					mother->aext = inputs[A_INPUT].getVoltage()/10;
			} else {
				mother->aext = 0;
			}
			if (inputs[D_INPUT].isConnected()){
					mother->dext = inputs[D_INPUT].getVoltage()/10;
			} else {
				mother->dext = 0;
			}
			if (inputs[R_INPUT].isConnected()){
					mother->rext = inputs[R_INPUT].getVoltage()/10;
			}else {
				mother->rext = 0;
			}

			//S Levels
			if (inputs[S_INPUT].isConnected()){
					mother->sext = inputs[S_INPUT].getVoltage()/10;
			}else {
				mother->sext = 0;
			}

			//CURVE CONTROLS
			if (inputs[ACURVE_INPUT].isConnected()){
					mother->acext = inputs[ACURVE_INPUT].getVoltage()/10;
			}else {
				mother->acext = 0;
			}
			if (inputs[DCURVE_INPUT].isConnected()){
					mother->dcext = inputs[DCURVE_INPUT].getVoltage()/10;
			}else {
				mother->dcext = 0;
			}
			if (inputs[RCURVE_INPUT].isConnected()){
					mother->rcext = inputs[RCURVE_INPUT].getVoltage()/10;
			}else {
				mother->rcext = 0;
			}

		} else{
		}
		
		

	}

	void onAdd() override {

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