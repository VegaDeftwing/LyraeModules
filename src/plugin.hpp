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