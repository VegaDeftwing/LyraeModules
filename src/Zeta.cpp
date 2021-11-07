#include "plugin.hpp"


struct Zeta : Module {
	enum ParamIds {
		FLIPPED_PARAM,
		ALT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Zeta() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(ALT_PARAM, 0.f, 2.f, 0.f, "Alt Display");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct ZetaWidget : ModuleWidget {

    void draw(const DrawArgs& args) override {

		std::shared_ptr<Svg> svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zeta.svg"));

		if (module != NULL) {
			Zeta *zeta = dynamic_cast<Zeta *>(module);

			if (zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 1.f) {
				if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f){
					svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zetab.svg"));
				} else {
					svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/ZetabAlt.svg"));
				}

			}else if (zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 2.f){
				if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f){
					svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zetac.svg"));
				} else {
					svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/ZetacAlt.svg"));
				}
			}else{
				if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f){
					svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zeta.svg"));
				} else {
					svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/ZetaAlt.svg"));
				}
			}
		}

        if (svg && svg->handle) {
            svgDraw(args.vg, svg->handle);
        }
    }

	ZetaWidget(Zeta* module) {
		setModule(module);
		// setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zeta.svg")));
		std::shared_ptr<rack::Svg> svg=APP->window->loadSvg(asset::plugin(pluginInstance, "res/Zeta.svg"));
		setPanel(svg);
	}

	void appendContextMenu(Menu *menu) override{
		Zeta *zeta = dynamic_cast<Zeta *>(module);
		assert(zeta);

		struct ZetaOutputAltItem : MenuItem{
            Zeta *zeta;

            void onAction(const event::Action &e) override
            {
                //zeta->outputAlt = !zeta->outputAlt;
				if (zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f){
					zeta->paramQuantities[Zeta::FLIPPED_PARAM]->setValue(0.f);
				} else {
					zeta->paramQuantities[Zeta::FLIPPED_PARAM]->setValue(1.f);
				}
            }
            void step() override
            {
                rightText = CHECKMARK(zeta->paramQuantities[Zeta::FLIPPED_PARAM]->getValue() == 1.f);
            }
        };

		struct ZetaOutputAltaItem : MenuItem{
            Zeta *zeta;

            void onAction(const event::Action &e) override
            {
				zeta->paramQuantities[Zeta::ALT_PARAM]->setValue(0.f);
            }
            void step() override
            {
                rightText = CHECKMARK(zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 0.f);
            }
        };

		struct ZetaOutputAltbItem : MenuItem{
            Zeta *zeta;

            void onAction(const event::Action &e) override
            {
				zeta->paramQuantities[Zeta::ALT_PARAM]->setValue(1.f);
            }
            void step() override
            {
                rightText = CHECKMARK(zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 1.f);
            }
        };

		struct ZetaOutputAltcItem : MenuItem{
            Zeta *zeta;

            void onAction(const event::Action &e) override
            {
				zeta->paramQuantities[Zeta::ALT_PARAM]->setValue(3.f);
            }
            void step() override
            {
                rightText = CHECKMARK(zeta->paramQuantities[Zeta::ALT_PARAM]->getValue() == 2.f);
            }
        };

		menu->addChild(new MenuEntry);
		ZetaOutputAltItem *altOutput = createMenuItem<ZetaOutputAltItem>("Flip Panel");
        altOutput->zeta = zeta;
		menu->addChild(altOutput);
		ZetaOutputAltaItem *altaOutput = createMenuItem<ZetaOutputAltaItem>("Green Echos");
        altaOutput->zeta = zeta;
		menu->addChild(altaOutput);
		ZetaOutputAltbItem *altbOutput = createMenuItem<ZetaOutputAltbItem>("Blue Fish");
        altbOutput->zeta = zeta;
		menu->addChild(altbOutput);
		ZetaOutputAltcItem *altcOutput = createMenuItem<ZetaOutputAltcItem>("Purple Demons");
        altcOutput->zeta = zeta;
		menu->addChild(altcOutput);
	}
};


Model* modelZeta = createModel<Zeta, ZetaWidget>("Zeta");