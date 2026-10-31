#include "plugin.hpp"

struct Delta : Module
{
  enum ParamIds
  {
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
  enum InputIds
  {
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
  enum OutputIds
  {
    LEFT_OUTPUT,
    RIGHT_OUTPUT,
    NUM_OUTPUTS
  };
  enum LightIds
  {
    ENUMS(STAGE_LIGHT, 3),
    LMR_LIGHT,
    RML_LIGHT,
    NUM_LIGHTS
  };

  Delta()
  {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    configParam(R1A_PARAM, 0.f, 1.f, 0.f, "Near Stage Attenuation");
    configParam(R1O_PARAM, 0.f, 1.f, 0.f, "Near Stage Offset");
    configParam(R2A_PARAM, 0.f, 1.f, 0.f, "Mid Stage Attenuation");
    configParam(R2O_PARAM, 0.f, 1.f, 0.f, "Mid Stage Offset");
    configParam(R3A_PARAM, 0.f, 1.f, 0.f, "Far Stage Attenuation");
    configParam(R3O_PARAM, 0.f, 1.f, 0.f, "Far Stage Offset");
    configButton(MANUALCLOCK_PARAM, "MUX Advance");
    configButton(RMLGATEMANUAL_PARAM, "Momentary Right Mod Left");
    configButton(RMLGATEHOLD_PARAM, "Toggle Right Mod Left");
    configButton(LMRGATEMANUAL_PARAM, "Momentary Left Mod Right");
    configButton(LMRGATEHOLD_PARAM, "Toggle Left Mod Right");
    configParam(SELECT_PARAM, 0.f, 2.f, 0.f, "Clock Select State");
    configParam(LMR_PARAM, 0.f, 1.f, 0.f, "LMR State");
    configParam(RML_PARAM, 0.f, 1.f, 0.f, "RML State");

    configInput(LEFT_INPUT, "Left");
    configInput(RIGHT_INPUT, "Right");
    configInput(R1_INPUT, "Ring 1");
    configInput(R2_INPUT, "Ring 2");
    configInput(R3_INPUT, "Ring 3");
    configInput(CLOCK_INPUT, "Mux Advance");
    configInput(CS1_INPUT, "MUX 1");
    configInput(CS2_INPUT, "MUX 2");
    configInput(CS3_INPUT, "MUX 3");
    configInput(RMLGATE_INPUT, "RML toggle trigger");
    configInput(LMRGATE_INPUT, "LMR toggle trigger");

    configOutput(LEFT_OUTPUT, "Left");
    configOutput(RIGHT_OUTPUT, "Right");
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
  // bool RMLgate = false;
  // bool LMRgate = false;
  float STAGE2OUTL = 0.f;
  float STAGE2OUTR = 0.f;
  float STAGE3OUTL = 0.f;
  float STAGE3OUTR = 0.f;
  bool lRMLgate = false;
  bool lLMRgate = false;
  bool init = false;

  void process(const ProcessArgs &args) override
  {
    if (!init)
    {
      init = true;
      lRMLgate = params[RML_PARAM].getValue();
      lLMRgate = params[LMR_PARAM].getValue();
    }

    // BLOCK 1
    if (inputs[R1_INPUT].isConnected())
    {
      R1T = inputs[R1_INPUT].getVoltage() * params[R1A_PARAM].getValue() +
            params[R1O_PARAM].getValue();
    }
    else
    {
      R1T = 1.f;
    }

    if (inputs[R2_INPUT].isConnected())
    {
      R2T = inputs[R2_INPUT].getVoltage() * params[R2A_PARAM].getValue() +
            params[R2O_PARAM].getValue();
    }
    else
    {
      R2T = 1.f;
    }

    if (inputs[R3_INPUT].isConnected())
    {
      R3T = inputs[R3_INPUT].getVoltage() * params[R3A_PARAM].getValue() +
            params[R3O_PARAM].getValue();
    }
    else
    {
      R3T = 1.f;
    }

    float BLOCK1 = R1T * R2T * R3T;
    float STAGE1OUTL = (inputs[LEFT_INPUT].getVoltage() * -BLOCK1) / 10.f;
    float STAGE1OUTR = (inputs[RIGHT_INPUT].getVoltage() * BLOCK1) / 10.f;

    // BLOCK2 - clocksel or phase sel CS1,CS2,CS3 with xfade
    if (inputs[CS1_INPUT].isConnected() || inputs[CS2_INPUT].isConnected() ||
        inputs[CS3_INPUT].isConnected())
    {
      if (manualselect.process(params[MANUALCLOCK_PARAM].getValue()))
      {
        params[SELECT_PARAM].setValue(
            ((int)params[SELECT_PARAM].getValue() + 1) % 3);
      }
      if (clockedselect.process(inputs[CLOCK_INPUT].getVoltage()))
      {
        params[SELECT_PARAM].setValue(
            ((int)params[SELECT_PARAM].getValue() + 1) % 3);
      }

      switch ((int)params[SELECT_PARAM].getValue())
      {
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
    }
    else
    {
      STAGE2OUTL = STAGE1OUTL * 10.f;
      STAGE2OUTR = STAGE1OUTR * 10.f;
      lights[STAGE_LIGHT + 0].setBrightness(0.f);
      lights[STAGE_LIGHT + 1].setBrightness(0.f);
      lights[STAGE_LIGHT + 2].setBrightness(0.f);
    }

    // BLOCK3 - gate'd X-MOD of channels
    if (params[RMLGATEMANUAL_PARAM].getValue())
    {
      params[RML_PARAM].setValue(!lRMLgate);
    }
    else
    {
      params[RML_PARAM].setValue(lRMLgate);
      lRMLgate = params[RML_PARAM].getValue();
    }

    if (params[LMRGATEMANUAL_PARAM].getValue())
    {
      params[LMR_PARAM].setValue(!lLMRgate);
    }
    else
    {
      params[LMR_PARAM].setValue(lLMRgate);
      lLMRgate = params[LMR_PARAM].getValue();
    }

    if (RMLst.process(params[RMLGATEHOLD_PARAM].getValue()))
    {
      lRMLgate = !lRMLgate;
    }
    if (LMRst.process(params[LMRGATEHOLD_PARAM].getValue()))
    {
      lLMRgate = !lLMRgate;
    }

    if (cvRMLst.process(inputs[RMLGATE_INPUT].getVoltage()))
    {
      lRMLgate = !lRMLgate;
    }
    if (cvLMRst.process(inputs[LMRGATE_INPUT].getVoltage()))
    {
      lLMRgate = !lLMRgate;
    }

    if ((int)params[RML_PARAM].getValue())
    {
      STAGE3OUTL = -STAGE2OUTL * STAGE2OUTR / 5.f;
      lights[RML_LIGHT].setBrightness(1.f);
    }
    else
    {
      STAGE3OUTL = STAGE2OUTL;
      lights[RML_LIGHT].setBrightness(0.f);
    }

    if ((int)params[LMR_PARAM].getValue())
    {
      STAGE3OUTR = STAGE2OUTR * STAGE2OUTL / 5.f;
      lights[LMR_LIGHT].setBrightness(1.f);
    }
    else
    {
      STAGE3OUTR = STAGE2OUTR;
      lights[LMR_LIGHT].setBrightness(0.f);
    }

    outputs[LEFT_OUTPUT].setVoltage(simd::atan(simd::atan(STAGE3OUTL / 10.f)) *
                                    10.f);
    outputs[RIGHT_OUTPUT].setVoltage(simd::atan(simd::atan(STAGE3OUTR / 10.f)) *
                                     10.f);
  }
};
