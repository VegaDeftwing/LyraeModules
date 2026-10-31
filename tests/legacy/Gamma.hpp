#include "plugin.hpp"

struct Gamma : Module
{
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
