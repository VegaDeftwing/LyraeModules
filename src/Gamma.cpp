#include "plugin.hpp"
#include "Polyphonic.hpp"

struct Gamma : Module
{
  virtual Gamma* polyVoice(int channel) { return channel == 0 ? this : nullptr; }

  enum ParamIds
  {
    KNOB_PARAM,
    INVERTSWITCH_PARAM,
    NUM_PARAMS
  };
  enum InputIds
  {
    OFFSET_INPUT,
    LEFT_INPUT,
    RIGHT_INPUT,
    NUM_INPUTS
  };
  enum OutputIds
  {
    LEFT_OUTPUT,
    RIGHT_OUTPUT,
    NUM_OUTPUTS
  };
  enum LightIds
  {
    NUM_LIGHTS
  };

  Gamma()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configInput(LEFT_INPUT, "Left");
    configInput(RIGHT_INPUT, "Right");
    configInput(OFFSET_INPUT, "Offset");

    configOutput(LEFT_OUTPUT, "Left");
    configOutput(RIGHT_OUTPUT, "Right");

    configParam(KNOB_PARAM, -10.f, 10.f, 0.f,
                "Bipolar offset/ring attenuversion");
    configSwitch(INVERTSWITCH_PARAM, 0.f, 1.f, 0.f, "2nd Output",
                 {"Normal", "Inverted"});
  }

  float offset = 0.f;

  void process(const ProcessArgs &args) override
  {
    if (inputs[OFFSET_INPUT].isConnected())
    {
      offset = inputs[OFFSET_INPUT].getVoltage() *
               (.1 * params[KNOB_PARAM].getValue());
    }
    else
    {
      offset = params[KNOB_PARAM].getValue();
    }

    outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() + offset);

    if (inputs[RIGHT_INPUT].isConnected())
    {
      if (params[INVERTSWITCH_PARAM].getValue())
      {
        outputs[RIGHT_OUTPUT].setVoltage(
            -1.f * inputs[RIGHT_INPUT].getVoltage() - offset);
      }
      else
      {
        outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage() -
                                         offset);
      }
    }
    else
    {
      if (params[INVERTSWITCH_PARAM].getValue())
      {
        outputs[RIGHT_OUTPUT].setVoltage(
            -1.f * inputs[LEFT_INPUT].getVoltage() - offset);
      }
      else
      {
        outputs[RIGHT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage() -
                                         offset);
      }
    }
  }
};

struct GammaWidget : ModuleWidget
{
  GammaWidget(Gamma *module)
  {
    setModule(module);
    setPanel(
        APP->window->loadSvg(asset::plugin(pluginInstance, "res/Gamma.svg")));

    addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH, +5)));
    addChild(createWidget<Bolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 5,
                                    RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParamCentered<HexKnob>(mm2px(Vec(5.08, 69.693)), module,
                                          Gamma::KNOB_PARAM));
    addParam(createParamCentered<CKSS>(mm2px(Vec(5.191, 116.946)), module,
                                       Gamma::INVERTSWITCH_PARAM));

    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 56.943)), module,
                                         Gamma::OFFSET_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 96.016)), module,
                                         Gamma::LEFT_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 105.876)), module,
                                         Gamma::RIGHT_INPUT));

    addOutput(createOutputCentered<OutJack>(mm2px(Vec(5.08, 12.4)), module,
                                            Gamma::LEFT_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(5.08, 22.26)), module,
                                            Gamma::RIGHT_OUTPUT));
  }
};

// Gamma has no history: process four cable channels directly, without the
// per-voice module/parameter copies needed by the stateful modules.
template <> struct Polyphonic<Gamma> : Gamma {
  void process(const ProcessArgs& args) override {
    int channels = 1;
    for (auto& input : inputs) channels = std::max(channels, input.getChannels());
    if (channels == 1) {
      Gamma::process(args);
      outputs[LEFT_OUTPUT].setChannels(1);
      outputs[RIGHT_OUTPUT].setChannels(1);
      return;
    }
    outputs[LEFT_OUTPUT].setChannels(channels);
    outputs[RIGHT_OUTPUT].setChannels(channels);
    for (int c = 0; c < channels; c += 4) {
      simd::float_4 offsets;
      for (int lane = 0; lane < 4; ++lane) {
        // The original .1 literal promotes this product to double. Preserve
        // that rounding before vectorizing the remaining signal arithmetic.
        offsets[lane] = inputs[OFFSET_INPUT].isConnected()
            ? inputs[OFFSET_INPUT].getPolyVoltage(c + lane) *
                (.1 * params[KNOB_PARAM].getValue())
            : params[KNOB_PARAM].getValue();
      }
      if (c == 0) offset = offsets[0];
      auto left = inputs[LEFT_INPUT].getPolyVoltageSimd<simd::float_4>(c);
      auto right = inputs[RIGHT_INPUT].isConnected()
          ? inputs[RIGHT_INPUT].getPolyVoltageSimd<simd::float_4>(c) : left;
      outputs[LEFT_OUTPUT].setVoltageSimd(left + offsets, c);
      if (params[INVERTSWITCH_PARAM].getValue()) right = -1.f * right;
      outputs[RIGHT_OUTPUT].setVoltageSimd(right - offsets, c);
    }
  }
};

Model *modelGamma = createModel<Polyphonic<Gamma>, GammaWidget>("Gamma");
