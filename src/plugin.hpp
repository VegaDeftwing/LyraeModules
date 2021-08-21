#pragma once
//#include <rack.hpp>
// This is done to make auto complete work in VSCode
#include </home/vega/Documents/Rack-SDK/include/rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;
extern Model* modelSulafat;
extern Model* modelGamma;
extern Model* modelVega;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;

struct Bolt : SvgScrew {
	Bolt() {
		sw->svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/Bolt.svg"));
		sw->wrap();
		box.size = sw->box.size;
	}
};

struct HexKnob : app::SvgKnob {
    HexKnob() {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HexKnob.svg")));
    }
};

struct SmallHexKnob : app::SvgKnob {
    SmallHexKnob() {
        minAngle = -0.8 * M_PI;
        maxAngle = 0.8 * M_PI;
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SmallHexKnob.svg")));
    }
};

struct InJack : app::SvgPort {
    widget::TransformWidget* tw;

    InJack() {
        fb->removeChild(sw);
		tw = new TransformWidget();
		tw->addChild(sw);
		fb->addChild(tw);
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Jack.svg")));
        tw->box.size = sw->box.size;
		box.size = tw->box.size;
		float angle = random::uniform() * M_PI;
		tw->identity();
		// Rotate SVG
		math::Vec center = sw->box.getCenter();
		tw->translate(center);
		tw->rotate(angle);
		tw->translate(center.neg());
    }
};

struct OutJack : app::SvgPort {
    widget::TransformWidget* tw;

    OutJack() {
        fb->removeChild(sw);
		tw = new TransformWidget();
		tw->addChild(sw);
		fb->addChild(tw);
        setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Jack.svg")));
        tw->box.size = sw->box.size;
		box.size = tw->box.size;
		float angle = random::uniform() * M_PI;
		tw->identity();
		// Rotate SVG
		math::Vec center = sw->box.getCenter();
		tw->translate(center);
		tw->rotate(angle);
		tw->translate(center.neg());
    }
};