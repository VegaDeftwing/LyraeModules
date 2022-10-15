#include "plugin.hpp"

struct Vega : Module {
  enum ParamIds {
    // Do Net Reorder these
    ATTACK_FORCE_ADV_PARAM,
    DECAY_FORCE_ADV_PARAM,
    SUSTAIN_FORCE_ADV_PARAM,
    ATTACK_OUT_MODE_BUTTON_PARAM,
    DECAY_OUT_MODE_BUTTON_PARAM,
    SUSTAIN_OUT_MODE_BUTTON_PARAM,
    RELEASE_OUT_MODE_BUTTON_PARAM,
    ATTACK_RING_ATT_PARAM,
    DECAY_RING_ATT_PARAM,
    SUSTAIN_RING_ATT_PARAM,
    RELEASE_RING_ATT_PARAM,
    ATTACK_TIME_PARAM,
    DECAY_TIME_PARAM,
    SUSTAIN_LEVEL_PARAM,
    RELEASE_TIME_PARAM,
    ATTACK_RING_MODE_BUTTON_PARAM,
    DECAY_RING_MODE_BUTTON_PARAM,
    SUSTAIN_RING_MODE_BUTTON_PARAM,
    RELEASE_RING_MODE_BUTTON_PARAM,
    ATTACK_CURVE_PARAM,
    DECAY_CURVE_PARAM,
    RELEASE_CURVE_PARAM,
    GLOBAL_RING_ATT_PARAM,
    GLOBAL_RING_OFFSET_PARAM,
    ANGER_PARAM,
    TRACK_PARAM,
    SAMPLE_AND_HOLD_PARAM,
    // Added to save state
    ATTACK_OUT_MODE_PARAM,
    DECAY_OUT_MODE_PARAM,
    SUSTAIN_OUT_MODE_PARAM,
    RELEASE_OUT_MODE_PARAM,
    ATTACK_RING_MODE_PARAM,
    DECAY_RING_MODE_PARAM,
    SUSTAIN_RING_MODE_PARAM,
    RELEASE_RING_MODE_PARAM,
    OUTPUT_ALT_PARAM,
    OUTPUT_EOR_PARAM,
    NUM_PARAMS
  };
  enum InputIds {
    // Do not reorder these!
    ATTACK_ADV_INPUT,
    DECALY_ADV_INPUT,
    SUSTAIN_ADV_INPUT,
    ATTACK_MOD_INPUT,
    DECAY_MOD_INPUT,
    SUSTAIN_MOD_INPUT,
    RELEASE_MOD_INPUT,
    GATE_INPUT,
    GLOBAL_RING_INPUT,
    RETRIG_INPUT,
    SANDH_INPUT,
    ANGER_INPUT,
    NUM_INPUTS
  };
  enum OutputIds {
    // Do not reorder these!
    ATTACK_OUTPUT,
    DECAY_OUTPUT,
    SUSTAIN_OUTPUT,
    RELEASE_OUTPUT,
    ATTACK_GATE_OUTPUT,
    DECAY_GATE_OUTPUT,
    SUSTAIN_GATE_OUTPUT,
    RELEASE_GATE_OUTPUT,
    MAIN_POSITIVE_OUTPUT,
    MAIN_NEGATIVE_OUTPUT,
    NUM_OUTPUTS
  };
  enum LightIds {
    ENUMS(AMODE_LIGHT, 3),
    ENUMS(DMODE_LIGHT, 3),
    ENUMS(SMODE_LIGHT, 3),
    ENUMS(RMODE_LIGHT, 3),
    ENUMS(AGATE_LIGHT, 3),
    ENUMS(DGATE_LIGHT, 3),
    ENUMS(SGATE_LIGHT, 3),
    ENUMS(RGATE_LIGHT, 3),
    NUM_LIGHTS
  };

  dsp::ClockDivider processDivider;

  // This, along with the mess in the process divider which updates lastValue,
  // makes it so that each knob will 'inherit' the default value of the knob
  // above it this makes setting equal amounts of attenuation easier.
  struct ChainParamQuantity : ParamQuantity {
    float getDefaultValue() override {
      if (paramId <= 0) return 0.f;
      if (!module) return 0.f;
      return module->params[paramId - 1].getValue();
    }
  };

  // This makes the decay curve default value whatever it needs to be to go
  // back to being linear while still leaving the full range.
  struct BezierParamQuantity : ParamQuantity {
    float getDefaultValue() override {
      Vega *vega = dynamic_cast<Vega *>(module);
      if (paramId <= 0) return 0.f;
      if (!module) return 0.f;
      return ((1 + vega->sustain_level) / 2);
    }
  };

  // These globals get modified from the expander. This is a sort of slimely way
  // of passing messeages, but it works.
  float attack_time_from_expander = 0.f;
  float decay_time_from_expander = 0.f;
  float sustain_level_from_expander = 0.f;
  float release_time_from_expander = 0.f;
  float attack_curve_from_expander = 0.f;
  float decay_curve_from_expander = 0.f;
  float release_curve_from_expander = 0.f;

  Vega() {
    processDivider.setDivision(64);

    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    configParam<ChainParamQuantity>(ATTACK_RING_ATT_PARAM, -0.2, 0.2, 0.f,
                                    "Attack Ring Attenuate");
    configParam<ChainParamQuantity>(DECAY_RING_ATT_PARAM, -0.2, 0.2, 0.f,
                                    "Decay Ring Attenuate");
    configParam<ChainParamQuantity>(SUSTAIN_RING_ATT_PARAM, -0.2, 0.2, 0.f,
                                    "Sustain Ring Attenuate");
    configParam<ChainParamQuantity>(RELEASE_RING_ATT_PARAM, -0.2, 0.2, 0.f,
                                    "Release Ring Attenuate");

    // Basic ADSR controls (time,time,level,time)
    configParam(ATTACK_TIME_PARAM, 0.5, 1.5, 1.1125, "Attack Time");
    configParam(DECAY_TIME_PARAM, 0.9, 1.5, 1.216, "Decay Time");
    configParam(SUSTAIN_LEVEL_PARAM, 0.f, 1.f, 0.5, "Sustain Level");
    configParam(RELEASE_TIME_PARAM, 0.9, 1.6, 1.2682, "Release Time");

    // Output mode buttons
    configButton(ATTACK_OUT_MODE_BUTTON_PARAM, "Attack Output Mode");
    configButton(DECAY_OUT_MODE_BUTTON_PARAM, "Decay Output Mode");
    configButton(SUSTAIN_OUT_MODE_BUTTON_PARAM, "Sustain Output Mode");
    configButton(RELEASE_OUT_MODE_BUTTON_PARAM, "Release Output Mode");

    // Modulation mode buttons
    configButton(ATTACK_RING_MODE_BUTTON_PARAM, "Attack Modulation Mode");
    configButton(DECAY_RING_MODE_BUTTON_PARAM, "Decay Modulation Mode");
    configButton(SUSTAIN_RING_MODE_BUTTON_PARAM, "Sustain Modulation Mode");
    configButton(RELEASE_RING_MODE_BUTTON_PARAM, "Release Modulation Mode");

    // A,D,R curve parameters - Decay get's the default set to be linear based
    // on the value of SUSTAIN_LEVEL_PARAM
    configParam(ATTACK_CURVE_PARAM, 0.2, 3.f, 1.f, "Attack Curve");
    configParam<BezierParamQuantity>(DECAY_CURVE_PARAM, 0.f, 1.3f, 0.75,
                                     "Decay Curve");
    configParam(RELEASE_CURVE_PARAM, 0.2, 7.4, 1.f, "Release Curve");

    // Force advance buttons
    configButton(ATTACK_FORCE_ADV_PARAM, "Attack Force Advance");
    configButton(DECAY_FORCE_ADV_PARAM, "Decay Force Advance");
    configButton(SUSTAIN_FORCE_ADV_PARAM, "Sustain Force Advance");

    // Global controls - Anger controls X-FADE time.
    // Offset acts as attenuator if no ring input
    configParam(ANGER_PARAM, 1.f, 0.f, .5, "Transistion Time Control");
    configParam(GLOBAL_RING_ATT_PARAM, 0.f, 0.2, 0.f, "Gloal Ring Attenuate");
    configParam(GLOBAL_RING_OFFSET_PARAM, 0.f, 1.f, 1.f, "Global Ring Offset");

    // S&H Section
    configParam(SAMPLE_AND_HOLD_PARAM, 40.f, 1.f, 40.f, "S&H Frequency");
    configParam(TRACK_PARAM, 0.f, 1.f, 0.f, "Slew after S&H");

    // Internal parameters for saving state
    configParam(ATTACK_OUT_MODE_PARAM, 0.f, 1.f, 0.f, "Attack Output Mode");
    configParam(DECAY_OUT_MODE_PARAM, 0.f, 1.f, 0.f, "Decay Output Mode");
    configParam(SUSTAIN_OUT_MODE_PARAM, 0.f, 2.f, 0.f, "Sustain Output Mode");
    configParam(RELEASE_OUT_MODE_PARAM, 0.f, 1.f, 0.f, "Release Output Mode");
    configParam(ATTACK_RING_MODE_PARAM, 0.f, 3.f, 0.f,
                "Attack Modulation Mode");
    configParam(DECAY_RING_MODE_PARAM, 0.f, 3.f, 0.f, "Decay Modulation Mode");
    configParam(SUSTAIN_RING_MODE_PARAM, 0.f, 1.f, 0.f,
                "Sustain Modulation Mode");
    configParam(RELEASE_RING_MODE_PARAM, 0.f, 3.f, 0.f,
                "Release Modulation Mode");
    configParam(OUTPUT_ALT_PARAM, 0.f, 3.f, 0.f, "Dry output on negative");
  }

  enum Stage { ATTACK, DECAY, SUSTAIN, RELEASE, FINISHED };

  Stage current_stage = ATTACK;
  // If the envelope is still running
  bool is_running = false;
  // The linear envelope, generated piecewise
  float phasor = 0.f;
  // The envelope after curve has been applied
  float env = 0.f;
  // The value that will be math'd onto the envelope to get the output
  float modulation = 0.f;
  // The modulation gets XFaded between stages depending on the anger knob
  float modulation_source = 0.f;
  float modulation_dest = 0.f;
  // The envelope with all the modulation
  float output = 0.f;
  // Primary Gate Trigger
  dsp::SchmittTrigger gateDetect;
  dsp::SchmittTrigger retrigDetect;
  // Output Mode Triggers
  dsp::SchmittTrigger AOMDetect;
  dsp::SchmittTrigger DOMDetect;
  dsp::SchmittTrigger SOMDetect;
  dsp::SchmittTrigger ROMDetect;
  // Modulation Mode Triggers
  dsp::SchmittTrigger AMDetect;
  dsp::SchmittTrigger DMDetect;
  dsp::SchmittTrigger SMDetect;
  dsp::SchmittTrigger RMDetect;
  // EOR trigger
  dsp::SchmittTrigger EORDetect;

  // Alt mode in R-Click menu to switch the per-stage gate outputs to triggers
  // TODO - not yet implimented
  bool menu_stage_gates_are_triggers = false;

  // This holds the sustain level as it's used a lot, running getValue() a lot
  // is inefficient and hard to read. It needs to be global to deal with
  // BezierParamQuantity above.
  float sustain_level = 0.75;

  // Oscilator for S&H.
  float sampleSquare = 0.f;

  void displayActive(Stage lstage) {
    lights[AGATE_LIGHT + 0].setBrightness(lstage == ATTACK ? 1.f : 0.f);
    lights[DGATE_LIGHT + 0].setBrightness(lstage == DECAY ? 1.f : 0.f);
    lights[SGATE_LIGHT + 0].setBrightness(lstage == SUSTAIN ? 1.f : 0.f);
    lights[RGATE_LIGHT + 0].setBrightness(lstage == RELEASE ? 1.f : 0.f);
  }

  void forceAdvance(Stage lstage) {
    if (inputs[lstage].isConnected()) {
      if (inputs[lstage].getVoltage() >= 5.f) {
        current_stage = (Stage)((int)lstage + 1);
        if (lstage == 0) {
          // if the attack stage is skipped the envelope needs to be set
          // high so the decay stage has something to work with
          phasor = 1;
        }
      }
    }
    if (params[current_stage].getValue() >= .5f) {
      current_stage = (Stage)((int)lstage + 1);
      if (lstage == 0) {
        // if the attack stage is skipped the envelope needs to be set high
        // so the decay stage has something to work with
        phasor = 1;
      }
    }
  }

  void perStageOutput(Stage stage, int mode) {
    // accessing output and env as globals. This is probably frowed upon but oh
    // well.
    if (stage != ATTACK) {
      if (outputs[stage - 1].isConnected()) {
        outputs[stage - 1].setVoltage(0.f);
      }
    }
    if (outputs[stage].isConnected()) {
      if (mode == 0) {  // LED OFF output mode, basic env
        outputs[stage].setVoltage(10.f * env);
      } else if (mode == 1) {  // BLUE LED output mode, env w/ modulation
        outputs[stage].setVoltage(10.f * output *
                                  params[GLOBAL_RING_OFFSET_PARAM].getValue());
      } else {  // GREEN LED output mode, env - DC, only available on Decay
        outputs[stage].setVoltage(10.f * (env - sustain_level));
      }
    }
  }

  void perStageGateOutput(Stage stage) {
    outputs[ATTACK_GATE_OUTPUT].setVoltage(stage == ATTACK ? 10.f : 0.f);
    outputs[DECAY_GATE_OUTPUT].setVoltage(stage == DECAY ? 10.f : 0.f);
    outputs[SUSTAIN_GATE_OUTPUT].setVoltage(stage == SUSTAIN ? 10.f : 0.f);

    if (params[OUTPUT_EOR_PARAM].getValue()) {
      outputs[RELEASE_GATE_OUTPUT].setVoltage(
          EORDetect.process(stage == FINISHED) ? 10.f : 0.f);
    } else {
      outputs[RELEASE_GATE_OUTPUT].setVoltage(stage == RELEASE ? 10.f : 0.f);
    }
  }

  void setModeLight(Stage lstage, int lmode) {
    int offset = (int)lstage * 3;
    switch (lmode) {
      case 0:
        lights[offset + 0].setBrightness(1.f);
        lights[offset + 1].setBrightness(0.f);
        lights[offset + 2].setBrightness(0.f);
        break;
      case 1:
        lights[offset + 0].setBrightness(0.f);
        lights[offset + 1].setBrightness(1.f);
        lights[offset + 2].setBrightness(0.f);
        break;
      case 2:
        lights[offset + 0].setBrightness(0.f);
        lights[offset + 1].setBrightness(0.f);
        lights[offset + 2].setBrightness(1.f);
        break;
      case 3:
        lights[offset + 0].setBrightness(1.f);
        lights[offset + 1].setBrightness(1.f);
        lights[offset + 2].setBrightness(1.f);
        break;
      default:
        lights[offset + 0].setBrightness(0.f);
        lights[offset + 1].setBrightness(0.f);
        lights[offset + 2].setBrightness(0.f);
        break;
    }
  }

  void attack_stage(float &phasor, float &env, float &anger, Stage &stage) {
    phasor += simd::pow(.000315, params[ATTACK_TIME_PARAM].getValue() +
                                     attack_time_from_expander);
    env = simd::pow(phasor, params[ATTACK_CURVE_PARAM].getValue() +
                                attack_curve_from_expander);

    if (phasor > 1.0) {
      stage = DECAY;
    }

    // This is crossfading the modulation signal input. This would be okay if
    // the modulation method were the same on each stage, but if one stage is
    // RM and the other Add, this probably fails to make smooth transitions due
    // to RM and ADD leading to different signal amplitudes. A quick test makes
    // it seem like this isn't a problem, but I'm not sure. and franky I don't
    // think it's 100% necessary that it does work like that, so this is going
    // to stay as is

    if (inputs[ATTACK_MOD_INPUT].isConnected()) {
      if (inputs[DECAY_MOD_INPUT].isConnected()) {  // Necessary for normalling
        modulation =
            simd::crossfade(inputs[ATTACK_MOD_INPUT].getVoltage() *
                                params[ATTACK_RING_ATT_PARAM].getValue(),
                            inputs[DECAY_MOD_INPUT].getVoltage() *
                                params[DECAY_RING_ATT_PARAM].getValue(),
                            (simd::fmax(0, anger * env - anger + 1)));
      } else {
        modulation =
            simd::crossfade(inputs[ATTACK_MOD_INPUT].getVoltage() *
                                params[ATTACK_RING_ATT_PARAM].getValue(),
                            inputs[ATTACK_MOD_INPUT].getVoltage() *
                                params[DECAY_RING_ATT_PARAM].getValue(),
                            (simd::fmax(0, anger * env - anger + 1)));
      }

      switch ((int)params[ATTACK_RING_MODE_PARAM].getValue()) {
        case 0:  // Ring
          output = modulation * env + env;
          break;
        case 1:  // Addition
          output = modulation + env;
          break;
        case 2:              // Self-Env Addition
          if (env <= 0.2) {  // first 20% of Attack stage
            output = (modulation * env * 10) + env;
          } else {
            output = modulation + env;
          }
          break;
        case 3:
          output = ((-env + 1) * modulation) + env;
          break;
        default:
          output = modulation * env + env;
          break;
      }
    } else {
      modulation = simd::crossfade(0.f,
                                   inputs[DECAY_MOD_INPUT].getVoltage() *
                                       params[DECAY_RING_ATT_PARAM].getValue(),
                                   (simd::fmax(0, anger * env - anger + 1)));
      output = env;
    }

    displayActive(ATTACK);
    perStageOutput(ATTACK, params[ATTACK_OUT_MODE_PARAM].getValue());
    perStageGateOutput(ATTACK);
    forceAdvance(ATTACK);  // checks if the force advance is true internally
  }

  void decay_stage(float &phasor, float &env, float &anger, Stage &stage) {
    phasor -= simd::pow(.000315, params[DECAY_TIME_PARAM].getValue() +
                                     decay_time_from_expander);
    // using bezier curves (ish) because nothing else wanted to
    // work p0 is starting point, p2 is the ending point, p1 is the
    // control point t is the phasor from 0 to 1
    // p1+(1-t)^2*(p0-p1)+t^2*(p2-p1)
    env = (params[DECAY_CURVE_PARAM].getValue() +
           decay_curve_from_expander)   // p1
          + simd::pow((1 - phasor), 2)  //+(1-t)^2
                * (sustain_level - (params[DECAY_CURVE_PARAM].getValue() +
                                    decay_curve_from_expander))  //*(p0-p1)
          + simd::pow(phasor, 2)                                 //+t^2
                * (1 - (params[DECAY_CURVE_PARAM].getValue() +
                        decay_curve_from_expander));  //*(p2-p1)

    if (phasor <= 0) {
      stage = SUSTAIN;
    }

    // Normal modulation inputs going down
    if (inputs[DECAY_MOD_INPUT].isConnected()) {
      modulation_source = inputs[DECAY_MOD_INPUT].getVoltage();
    } else if (inputs[ATTACK_MOD_INPUT].isConnected()) {
      modulation_source = inputs[ATTACK_MOD_INPUT].getVoltage();
    } else {
      modulation_source = 0;
    }

    // modulation with xfade, envelope gets inverted to make it
    // similar to the attack envelope
    modulation = simd::crossfade(
        modulation_source * params[DECAY_RING_ATT_PARAM].getValue(),
        modulation_dest * params[SUSTAIN_RING_ATT_PARAM].getValue(),
        (simd::fmax(0, anger * (-env) - anger + (1 / (sustain_level + 0.01)))));
    //.01 is still not perfect, low sustain vals will still cause
    // issues.
    // but, it makes the range of bad values low enough to just let
    // clamping handle the weird situations.
    switch ((int)params[DECAY_RING_MODE_PARAM].getValue()) {
      case 0:  // Ring
        output = modulation * env + env;
        break;
      case 1:  // Addition
        output = modulation + env;
        break;
      case 2:  // Self-Env Addition
        if ((-env + 1 * (1 / sustain_level)) <=
            0.2) {  // first 20% of decay stage
          output = (modulation * env * 10) + env;
        } else {
          output = modulation + env;
        }
        break;
      case 3:
        output = ((-env + sustain_level) * modulation) + env;
        break;
      default:
        output = modulation * env + env;
        break;
    }

    displayActive(DECAY);
    perStageOutput(DECAY, params[DECAY_OUT_MODE_PARAM].getValue());
    perStageGateOutput(DECAY);
    forceAdvance(DECAY);  // checks if the force advance is true internally
  }

  void sustain_stage(float &phasor, float &env, float &anger, Stage &stage) {
    env = sustain_level;
    phasor = sustain_level;

    // Normal modulation inputs going down, if none connected 0-out
    // the modulation source
    if (inputs[SUSTAIN_MOD_INPUT].isConnected()) {
      modulation_source = inputs[SUSTAIN_MOD_INPUT].getVoltage();
    } else if (inputs[DECAY_MOD_INPUT].isConnected()) {
      modulation_source = inputs[DECAY_MOD_INPUT].getVoltage();
    } else if (inputs[ATTACK_MOD_INPUT].isConnected()) {
      modulation_source = inputs[ATTACK_MOD_INPUT].getVoltage();
    } else {
      modulation_source = 0;
    }

    // No XFade on the sustain stage, as it's length is unknown
    // This is resolved by XFading on on the decay stage and the
    // Release stage
    modulation = modulation_source * params[SUSTAIN_RING_ATT_PARAM].getValue();

    // Sustain stage has less modes because it's constant
    switch ((int)params[SUSTAIN_RING_MODE_PARAM].getValue()) {
      case 0:  // Ring
        output = modulation * env + env;
        break;
      case 1:  // Addition
        output = modulation + env;
        break;
      default:
        output = modulation * env + env;
        break;
    }

    displayActive(SUSTAIN);
    perStageOutput(SUSTAIN, params[SUSTAIN_OUT_MODE_PARAM].getValue());
    perStageGateOutput(SUSTAIN);
    forceAdvance(SUSTAIN);  // checks if the force advance is true internally
  }

  void release_stage(float &phasor, float &env, float &anger, Stage &stage) {
    if (phasor <= 0.f) {
      return;
    }

    phasor -= simd::pow(.000315, params[RELEASE_TIME_PARAM].getValue() +
                                     release_time_from_expander);
    env = simd::pow(phasor * (1 / (sustain_level + .00001)),
                    params[RELEASE_CURVE_PARAM].getValue() +
                        release_curve_from_expander) *
          sustain_level;

    // Normal modulation inputs going down
    if (inputs[RELEASE_MOD_INPUT].isConnected()) {
      modulation_source = inputs[RELEASE_MOD_INPUT].getVoltage();
    } else if (inputs[SUSTAIN_MOD_INPUT].isConnected()) {
      modulation_source = inputs[SUSTAIN_MOD_INPUT].getVoltage();
    } else if (inputs[DECAY_MOD_INPUT].isConnected()) {
      modulation_source = inputs[DECAY_MOD_INPUT].getVoltage();
    } else if (inputs[ATTACK_MOD_INPUT].isConnected()) {
      modulation_source = inputs[ATTACK_MOD_INPUT].getVoltage();
    } else {
      modulation_source = 0;
    }

    // modulation with xfade, envelope gets inverted to make it similar
    // to the attack envelope
    modulation = simd::crossfade(
        modulation_source * params[RELEASE_RING_ATT_PARAM].getValue(),
        modulation_dest * params[SUSTAIN_RING_ATT_PARAM].getValue(),
        (simd::fmax(0,
                    anger * (-env) - anger + (1 / (sustain_level + 0.0001)))));

    switch ((int)params[RELEASE_RING_MODE_PARAM].getValue()) {
      case 0:  // Ring
        output = modulation * env + env;
        break;
      case 1:  // Addition
        output = modulation + env;
        break;
      case 2:  // Self-Env Addition
        if ((-env + 1 * (1 / sustain_level)) <=
            0.2) {  // first 20% of release stage
          output = (modulation * env * 10) + env;
        } else {
          output = modulation + env;
        }
        break;
      case 3:
        output = ((-env + sustain_level) * modulation) + env;
        break;
      default:
        output = modulation * env + env;
        break;
    }

    displayActive(RELEASE);
    perStageOutput(RELEASE, params[RELEASE_OUT_MODE_PARAM].getValue());
    perStageGateOutput(RELEASE);
  }

  void update_lights(void) {
    lights[AGATE_LIGHT + 2].setBrightness(
        params[ATTACK_OUT_MODE_PARAM].getValue() ? 1.f : 0.f);
    switch ((int)params[DECAY_OUT_MODE_PARAM].getValue()) {
      case 0:
        lights[DGATE_LIGHT + 1].setBrightness(0.f);
        lights[DGATE_LIGHT + 2].setBrightness(0.f);
        break;
      case 1:
        lights[DGATE_LIGHT + 1].setBrightness(0.f);
        lights[DGATE_LIGHT + 2].setBrightness(1.f);
        break;
      case 2:
        lights[DGATE_LIGHT + 1].setBrightness(1.f);
        lights[DGATE_LIGHT + 2].setBrightness(0.f);
        break;
      default:
        break;
    }

    lights[SGATE_LIGHT + 2].setBrightness(
        params[SUSTAIN_OUT_MODE_PARAM].getValue() ? 1.f : 0.f);

    lights[RGATE_LIGHT + 2].setBrightness(
        params[RELEASE_OUT_MODE_PARAM].getValue() ? 1.f : 0.f);

    setModeLight(ATTACK, (int)params[ATTACK_RING_MODE_PARAM].getValue());
    setModeLight(DECAY, (int)params[DECAY_RING_MODE_PARAM].getValue());
    setModeLight(SUSTAIN, (int)params[SUSTAIN_RING_MODE_PARAM].getValue());
    setModeLight(RELEASE, (int)params[RELEASE_RING_MODE_PARAM].getValue());

    // Yes, I realize it's bad style to not put these in a loop but
    // ¯\_(ツ)_/¯
    if (AOMDetect.process(params[ATTACK_OUT_MODE_BUTTON_PARAM].getValue())) {
      params[ATTACK_OUT_MODE_PARAM].setValue(
          ((int)params[ATTACK_OUT_MODE_PARAM].getValue() + 1) % 2);
    }
    if (DOMDetect.process(params[DECAY_OUT_MODE_BUTTON_PARAM].getValue())) {
      params[DECAY_OUT_MODE_PARAM].setValue(
          ((int)params[DECAY_OUT_MODE_PARAM].getValue() + 1) % 3);
    }
    if (SOMDetect.process(params[SUSTAIN_OUT_MODE_BUTTON_PARAM].getValue())) {
      params[SUSTAIN_OUT_MODE_PARAM].setValue(
          ((int)params[SUSTAIN_OUT_MODE_PARAM].getValue() + 1) % 2);
    }
    if (ROMDetect.process(params[RELEASE_OUT_MODE_BUTTON_PARAM].getValue())) {
      params[RELEASE_OUT_MODE_PARAM].setValue(
          ((int)params[RELEASE_OUT_MODE_PARAM].getValue() + 1) % 2);
    }

    // toggle states for Modulation Modes, update mode LED respectively
    if (AMDetect.process(params[ATTACK_RING_MODE_BUTTON_PARAM].getValue())) {
      params[ATTACK_RING_MODE_PARAM].setValue(
          ((int)params[ATTACK_RING_MODE_PARAM].getValue() + 1) % 4);
    }
    if (DMDetect.process(params[DECAY_RING_MODE_BUTTON_PARAM].getValue())) {
      params[DECAY_RING_MODE_PARAM].setValue(
          ((int)params[DECAY_RING_MODE_PARAM].getValue() + 1) % 4);
    }
    if (SMDetect.process(params[SUSTAIN_RING_MODE_BUTTON_PARAM].getValue())) {
      params[SUSTAIN_RING_MODE_PARAM].setValue(
          ((int)params[SUSTAIN_RING_MODE_PARAM].getValue() + 1) % 2);
    }
    if (RMDetect.process(params[RELEASE_RING_MODE_BUTTON_PARAM].getValue())) {
      params[RELEASE_RING_MODE_PARAM].setValue(
          ((int)params[RELEASE_RING_MODE_PARAM].getValue() + 1) % 4);
    }
  }

  void ring_active_output(float &output) {
    if (outputs[MAIN_POSITIVE_OUTPUT].isConnected()) {
      outputs[MAIN_POSITIVE_OUTPUT].setVoltage(
          simd::clamp(output * 10.f *
                          (inputs[GLOBAL_RING_INPUT].getVoltage() *
                               params[GLOBAL_RING_ATT_PARAM].getValue() +
                           params[GLOBAL_RING_OFFSET_PARAM].getValue()),
                      -12.0, 12.0));
    }
    if (outputs[MAIN_NEGATIVE_OUTPUT].isConnected()) {
      if (params[OUTPUT_ALT_PARAM].getValue()) {
        // Right Click menu option
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(
            simd::clamp(env * 10.f, -12.0, 12.0));
      } else {
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(
            simd::clamp(-1.f * output * 10.f *
                            (inputs[GLOBAL_RING_INPUT].getVoltage() *
                                 params[GLOBAL_RING_ATT_PARAM].getValue() +
                             params[GLOBAL_RING_OFFSET_PARAM].getValue()),
                        -12.0, 12.0));
      }
    }
  }

  void plain_output(float &env, float &output) {
    // If no ring input connected, the offset knob works as a
    // volume knob, to add more headroom when necessary
    if (outputs[MAIN_POSITIVE_OUTPUT].isConnected()) {
      outputs[MAIN_POSITIVE_OUTPUT].setVoltage(simd::clamp(
          output * (10.f * params[GLOBAL_RING_OFFSET_PARAM].getValue()), -12.0,
          12.0));
    }

    if (outputs[MAIN_NEGATIVE_OUTPUT].isConnected()) {
      if (params[OUTPUT_ALT_PARAM].getValue()) {
        // Right click menu option
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(
            simd::clamp(env * 10.f, -12.0, 12.0));
      } else {
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(simd::clamp(
            output * (-10.f * params[GLOBAL_RING_OFFSET_PARAM].getValue()),
            -12.0, 12.0));
      }
    }
  }

  void process(const ProcessArgs &args) override {
    // First, we need to get the gate
    bool gate_active = inputs[GATE_INPUT].value > 1.0;

    if (gateDetect.process(gate_active)) {
      is_running = true;
      current_stage = ATTACK;
    }

    bool retrig = inputs[RETRIG_INPUT].getVoltage() > 1.0;

    if (retrigDetect.process(retrig)) {
      current_stage = ATTACK;
    }

    if (is_running) {
      float anger = (simd::pow(params[ANGER_PARAM].getValue(), 2) * 8) + 1 -
                    (inputs[ANGER_INPUT].getVoltage() / 10.f);

      sustain_level =
          params[SUSTAIN_LEVEL_PARAM].getValue() + sustain_level_from_expander;

      // Modulation destination is dependent on whatever input is being
      // normalled to the sustain stage This is the signal that is used to
      // crossfade the decay->sustain and the sustain->release it is **not**
      // used for attack->decay, as that is a special case and so handled in
      // the attack stage's logic

      if (inputs[SUSTAIN_MOD_INPUT].isConnected()) {
        modulation_dest = inputs[SUSTAIN_MOD_INPUT].getVoltage();
      } else if (inputs[DECAY_MOD_INPUT].isConnected()) {
        modulation_dest = inputs[DECAY_MOD_INPUT].getVoltage();
      } else if (inputs[ATTACK_MOD_INPUT].isConnected()) {
        modulation_dest = inputs[ATTACK_MOD_INPUT].getVoltage();
      } else {
        modulation_dest = 0;
      }

      // Generate S&H osc
      if (inputs[SANDH_INPUT].isConnected()) {
        sampleSquare += args.sampleTime *
                        (params[SAMPLE_AND_HOLD_PARAM].getValue() *
                         (1.f - (inputs[SANDH_INPUT].getVoltage() / 10.f)));
      } else {
        sampleSquare +=
            args.sampleTime * params[SAMPLE_AND_HOLD_PARAM].getValue();
      }

      if (sampleSquare >= .5f) {
        sampleSquare = -.5f;
      }

      if (params[SAMPLE_AND_HOLD_PARAM].getValue() == 40.f) {
        sampleSquare = 1;  // always update output
      }

      switch (current_stage) {
        case ATTACK:
          attack_stage(phasor, env, anger, current_stage);
          break;
        case DECAY:
          decay_stage(phasor, env, anger, current_stage);
          break;
        case SUSTAIN:
          sustain_stage(phasor, env, anger, current_stage);
          if (!gate_active) {
            current_stage = RELEASE;
          }
          break;
        case RELEASE:
          release_stage(phasor, env, anger, current_stage);
          if (phasor <= 0.f) {
            output = 0.f;
            current_stage = FINISHED;
          }
          break;
        case FINISHED:
          displayActive(FINISHED);
          perStageGateOutput(FINISHED);
          output = 0.f;
          is_running = false;
      }

      // Output, this first if controls the s&h and the TRACK_PARAM the
      // track&hold amount output is clampped to -12 and 12. There's just to
      // much going on to ensure that bad outputs can't happen.

      if (sampleSquare >= (.499f - params[TRACK_PARAM].getValue())) {
        if (inputs[GLOBAL_RING_INPUT].isConnected()) {
          ring_active_output(output);
        } else {
          plain_output(env, output);
        }
      }
    } else {  // END isrunning
      // Necessary as modulation/S&H may leave it high
      outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(0.f);
      outputs[MAIN_POSITIVE_OUTPUT].setVoltage(0.f);
    }

    if (processDivider.process()) {
      update_lights();
    }
  }

  void onAdd() override {
    // Set the modulation mode LEDS to inital value at startup
    lights[AMODE_LIGHT + 0].setBrightness(1.f);
    lights[DMODE_LIGHT + 0].setBrightness(1.f);
    lights[SMODE_LIGHT + 0].setBrightness(1.f);
    lights[RMODE_LIGHT + 0].setBrightness(1.f);
    // Set the outputmode LEDs to inital value at startup
    lights[AGATE_LIGHT + 2].setBrightness(0.f);
    lights[DGATE_LIGHT + 2].setBrightness(0.f);
    lights[SGATE_LIGHT + 2].setBrightness(0.f);
    lights[RGATE_LIGHT + 2].setBrightness(0.f);
  }
};

struct VegaWidget : ModuleWidget {
  VegaWidget(Vega *module) {
    setModule(module);
    setPanel(
        APP->window->loadSvg(asset::plugin(pluginInstance, "res/Vega.svg")));

    addParam(createParamCentered<TL1105>(mm2px(Vec(54.916, 14.974)), module,
                                         Vega::ATTACK_OUT_MODE_BUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 14.467)), module,
                                          Vega::ATTACK_TIME_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 14.839)), module,
                                             Vega::ATTACK_RING_ATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 14.839)), module,
                                         Vega::ATTACK_RING_MODE_BUTTON_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 22.839)), module,
                                         Vega::ATTACK_FORCE_ADV_PARAM));
    addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 24.119)), module,
                                               Vega::ATTACK_CURVE_PARAM));

    addParam(createParamCentered<HexKnob>(mm2px(Vec(20.23, 120)), module,
                                          Vega::GLOBAL_RING_OFFSET_PARAM));
    addParam(createParamCentered<SmallHexKnobInv>(
        mm2px(Vec(20.23, 120)), module, Vega::GLOBAL_RING_ATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 38.839)), module,
                                         Vega::DECAY_OUT_MODE_BUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 38.467)), module,
                                          Vega::DECAY_TIME_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 38.839)), module,
                                             Vega::DECAY_RING_ATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 38.839)), module,
                                         Vega::DECAY_RING_MODE_BUTTON_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 46.839)), module,
                                         Vega::DECAY_FORCE_ADV_PARAM));
    addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 48.119)), module,
                                               Vega::DECAY_CURVE_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 63.089)), module,
                                         Vega::SUSTAIN_OUT_MODE_BUTTON_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 62.839)), module,
                                             Vega::SUSTAIN_RING_ATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 62.839)), module,
                                         Vega::SUSTAIN_RING_MODE_BUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 66.839)), module,
                                          Vega::SUSTAIN_LEVEL_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 70.839)), module,
                                         Vega::SUSTAIN_FORCE_ADV_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 87.089)), module,
                                         Vega::RELEASE_OUT_MODE_BUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 86.467)), module,
                                          Vega::RELEASE_TIME_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 86.839)), module,
                                             Vega::RELEASE_RING_ATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 86.839)), module,
                                         Vega::RELEASE_RING_MODE_BUTTON_PARAM));
    addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 96.118)), module,
                                               Vega::RELEASE_CURVE_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(32.128, 120)), module,
                                          Vega::SAMPLE_AND_HOLD_PARAM));
    addParam(createParamCentered<SmallHexKnobInv>(mm2px(Vec(32.128, 120)),
                                                  module, Vega::TRACK_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(44.026, 120)), module,
                                          Vega::ANGER_PARAM));

    addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 14.839)), module,
                                         Vega::ATTACK_MOD_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(41.196, 22.839)), module,
                                         Vega::ATTACK_ADV_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 38.839)), module,
                                         Vega::DECAY_MOD_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(41.196, 46.839)), module,
                                         Vega::DECALY_ADV_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 62.839)), module,
                                         Vega::SUSTAIN_MOD_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(41.196, 70.839)), module,
                                         Vega::SUSTAIN_ADV_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(33.02, 86.839)), module,
                                         Vega::RELEASE_MOD_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(8.332, 107.027)), module,
                                         Vega::GATE_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(20.23, 107.027)), module,
                                         Vega::GLOBAL_RING_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(32.128, 107.027)), module,
                                         Vega::SANDH_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(8.332, 119.784)), module,
                                         Vega::RETRIG_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(44.026, 107.027)), module,
                                         Vega::ANGER_INPUT));

    addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 14.839)), module,
                                            Vega::ATTACK_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 22.839)), module,
                                            Vega::ATTACK_GATE_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 38.839)), module,
                                            Vega::DECAY_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 46.839)), module,
                                            Vega::DECAY_GATE_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 63.089)), module,
                                            Vega::SUSTAIN_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 71.089)), module,
                                            Vega::SUSTAIN_GATE_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(62.624, 87.089)), module,
                                            Vega::RELEASE_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(70.8, 95.089)), module,
                                            Vega::RELEASE_GATE_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(63.014, 119.784)), module,
                                            Vega::MAIN_NEGATIVE_OUTPUT));
    addOutput(createOutputCentered<OutJack>(mm2px(Vec(74.54, 119.784)), module,
                                            Vega::MAIN_POSITIVE_OUTPUT));

    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(37.108, 18.839)), module, Vega::AMODE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(66.712, 18.839)), module, Vega::AGATE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(37.108, 42.839)), module, Vega::DMODE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(66.712, 42.839)), module, Vega::DGATE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(37.108, 66.839)), module, Vega::SMODE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(66.712, 67.089)), module, Vega::SGATE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(37.108, 90.839)), module, Vega::RMODE_LIGHT));
    addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(
        mm2px(Vec(66.712, 91.089)), module, Vega::RGATE_LIGHT));
  }

  void appendContextMenu(Menu *menu) override {
    Vega *vega = dynamic_cast<Vega *>(module);
    assert(vega);

    struct VegaOutputAltItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        if (vega->paramQuantities[Vega::OUTPUT_ALT_PARAM]->getValue() == 1.f) {
          vega->paramQuantities[Vega::OUTPUT_ALT_PARAM]->setValue(0.f);
        } else {
          vega->paramQuantities[Vega::OUTPUT_ALT_PARAM]->setValue(1.f);
        }
      }
      void step() override {
        rightText = CHECKMARK(
            vega->paramQuantities[Vega::OUTPUT_ALT_PARAM]->getValue() == 1.f);
      }
    };

    struct VegaOutputEORItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        if (vega->paramQuantities[Vega::OUTPUT_EOR_PARAM]->getValue() == 1.f) {
          vega->paramQuantities[Vega::OUTPUT_EOR_PARAM]->setValue(0.f);
        } else {
          vega->paramQuantities[Vega::OUTPUT_EOR_PARAM]->setValue(1.f);
        }
      }

      void step() override {
        rightText = CHECKMARK(
            vega->paramQuantities[Vega::OUTPUT_EOR_PARAM]->getValue() == 1.f);
      }
    };

    struct VegaOutputTriggersItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        vega->menu_stage_gates_are_triggers =
            !vega->menu_stage_gates_are_triggers;
      }
      void step() override {
        rightText = CHECKMARK(vega->menu_stage_gates_are_triggers);
      }
    };

    struct VegaDecTimeItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        vega->paramQuantities[Vega::ATTACK_TIME_PARAM]->setValue(
            vega->paramQuantities[Vega::ATTACK_TIME_PARAM]->getValue() - .1);
        vega->paramQuantities[Vega::DECAY_TIME_PARAM]->setValue(
            vega->paramQuantities[Vega::DECAY_TIME_PARAM]->getValue() - .1);
        vega->paramQuantities[Vega::RELEASE_TIME_PARAM]->setValue(
            vega->paramQuantities[Vega::RELEASE_TIME_PARAM]->getValue() - .1);
      }
    };

    struct VegaIncTimeItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        vega->paramQuantities[Vega::ATTACK_TIME_PARAM]->setValue(
            vega->paramQuantities[Vega::ATTACK_TIME_PARAM]->getValue() + .1);
        vega->paramQuantities[Vega::DECAY_TIME_PARAM]->setValue(
            vega->paramQuantities[Vega::DECAY_TIME_PARAM]->getValue() + .1);
        vega->paramQuantities[Vega::RELEASE_TIME_PARAM]->setValue(
            vega->paramQuantities[Vega::RELEASE_TIME_PARAM]->getValue() + .1);
      }
    };

    menu->addChild(new MenuEntry);
    VegaOutputAltItem *altOutput =
        createMenuItem<VegaOutputAltItem>("Negative Out Dry");
    altOutput->vega = vega;
    menu->addChild(altOutput);

    VegaOutputEORItem *eorOutput =
        createMenuItem<VegaOutputEORItem>("Release Gate → EOR Trig");
    eorOutput->vega = vega;
    menu->addChild(eorOutput);

    // VegaOutputTriggersItem *triggersOutput =
    // createMenuItem<VegaOutputTriggersItem>("Stage Gates to Trigs");
    // triggersOutput->vega = vega;
    // menu->addChild(triggersOutput);

    VegaDecTimeItem *decTime = createMenuItem<VegaDecTimeItem>("Decrease Time");
    decTime->vega = vega;
    menu->addChild(decTime);

    VegaIncTimeItem *incTime = createMenuItem<VegaIncTimeItem>("Increase Time");
    incTime->vega = vega;
    menu->addChild(incTime);

    menu->addChild(construct<MenuLabel>(
        &MenuLabel::text,
        "MODULATION MODES:\nRED: Ring\nGREEN: Add\nBLUE: Add With Fade "
        "(A,D,R "
        "Only)\nWHITE: Inverse Envelope Addition (A,D,R Only)"));
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, ""));
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, ""));
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, ""));
    menu->addChild(
        construct<MenuLabel>(&MenuLabel::text,
                             "OUTPUT MODES:\nOFF: Basic Envelope\nBLUE: With "
                             "Modulation\nGREEN: Basic Env - DC (Decay Only)"));
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, ""));
    menu->addChild(construct<MenuLabel>(&MenuLabel::text, ""));
  }
};

Model *modelVega = createModel<Vega, VegaWidget>("Vega");

// BD383238 - Vega Expander.

struct BD383238 : Module {
  enum ParamIds { NUM_PARAMS };
  enum InputIds {
    ATTACK_TIME_INPUT,
    ATTACK_CURVE_INPUT,
    DECAY_TIME_INPUT,
    DECAY_CURVE_INPUT,
    SUSTAIN_LEVEL_INPUT,
    RELEASE_TIME_INPUT,
    RELESAE_CURVE_INPUT,
    NUM_INPUTS
  };
  enum OutputIds { NUM_OUTPUTS };
  enum LightIds { NUM_LIGHTS };

  BD383238() { config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS); }

  Vega *findHostModulePtr(Module *module) {
    if (module) {
      if (module->rightExpander.module) {
        if (module->rightExpander.module->model == modelVega) {
          return reinterpret_cast<Vega *>(module->rightExpander.module);
        }
      } else {
        return nullptr;
      }
    }
    return (nullptr);
  }

  void process(const ProcessArgs &args) override {
    // connect to right expander module
    Vega *mother = findHostModulePtr(this);

    if (mother) {
      // TIME controls
      if (inputs[ATTACK_TIME_INPUT].isConnected()) {
        mother->attack_time_from_expander =
            inputs[ATTACK_TIME_INPUT].getVoltage() / 10;
      } else {
        mother->attack_time_from_expander = 0;
      }
      if (inputs[DECAY_TIME_INPUT].isConnected()) {
        mother->decay_time_from_expander =
            inputs[DECAY_TIME_INPUT].getVoltage() / 10;
      } else {
        mother->decay_time_from_expander = 0;
      }
      if (inputs[RELEASE_TIME_INPUT].isConnected()) {
        mother->release_time_from_expander =
            inputs[RELEASE_TIME_INPUT].getVoltage() / 10;
      } else {
        mother->release_time_from_expander = 0;
      }

      // S Levels
      if (inputs[SUSTAIN_LEVEL_INPUT].isConnected()) {
        mother->sustain_level_from_expander =
            inputs[SUSTAIN_LEVEL_INPUT].getVoltage() / 10;
      } else {
        mother->sustain_level_from_expander = 0;
      }

      // CURVE CONTROLS
      if (inputs[ATTACK_CURVE_INPUT].isConnected()) {
        mother->attack_curve_from_expander =
            inputs[ATTACK_CURVE_INPUT].getVoltage() / 10;
      } else {
        mother->attack_curve_from_expander = 0;
      }
      if (inputs[DECAY_CURVE_INPUT].isConnected()) {
        mother->decay_curve_from_expander =
            inputs[DECAY_CURVE_INPUT].getVoltage() / 10;
      } else {
        mother->decay_curve_from_expander = 0;
      }
      if (inputs[RELESAE_CURVE_INPUT].isConnected()) {
        mother->release_curve_from_expander =
            inputs[RELESAE_CURVE_INPUT].getVoltage() / 10;
      } else {
        mother->release_curve_from_expander = 0;
      }
    }
  }

  void onAdd() override {}
};

struct BD383238Widget : ModuleWidget {
  BD383238Widget(BD383238 *module) {
    setModule(module);
    setPanel(APP->window->loadSvg(
        asset::plugin(pluginInstance, "res/BD383238.svg")));

    addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH, +5)));
    addChild(createWidget<Bolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 5,
                                    RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 14.467)), module,
                                         BD383238::ATTACK_TIME_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 24.119)), module,
                                         BD383238::ATTACK_CURVE_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 38.467)), module,
                                         BD383238::DECAY_TIME_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 48.119)), module,
                                         BD383238::DECAY_CURVE_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 66.839)), module,
                                         BD383238::SUSTAIN_LEVEL_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 86.467)), module,
                                         BD383238::RELEASE_TIME_INPUT));
    addInput(createInputCentered<InJack>(mm2px(Vec(5.08, 96.118)), module,
                                         BD383238::RELESAE_CURVE_INPUT));
  }
};

Model *modelBD383238 = createModel<BD383238, BD383238Widget>("BD383238");