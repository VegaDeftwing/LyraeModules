#include "plugin.hpp"

struct Vega : Module {
  enum ParamIds {
    // Do Net Reorder these
    AFORCEADV_PARAM,
    DFORCEADV_PARAM,
    SFORCEADV_PARAM,
    AOUTMODEBUTTON_PARAM,
    DOUTMODEBUTTON_PARAM,
    SOUTMODEBUTTON_PARAM,
    ROUTMODEBUTTON_PARAM,
    ARINGATT_PARAM,
    DRINGATT_PARAM,
    SRINGATT_PARAM,
    RRINGATT_PARAM,
    A_PARAM,
    D_PARAM,
    S_PARAM,
    R_PARAM,
    ARINGMODEBUTTON_PARAM,
    DRINGMODEBUTTON_PARAM,
    SRINGMODEBUTTON_PARAM,
    RRINGMODEBUTTON_PARAM,
    ACURVE_PARAM,
    DCURVE_PARAM,
    RCURVE_PARAM,
    GLOBALRINGATT_PARAM,
    GLOBALRINGOFFSET_PARAM,
    ANGER_PARAM,
    TRACK_PARAM,
    SANDH_PARAM,
    // Added to save state
    AOUTMODE_PARAM,
    DOUTMODE_PARAM,
    SOUTMODE_PARAM,
    ROUTMODE_PARAM,
    ARINGMODE_PARAM,
    DRINGMODE_PARAM,
    SRINGMODE_PARAM,
    RRINGMODE_PARAM,
    OUTPUTALT_PARAM,
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

    configParam<ChainParamQuantity>(ARINGATT_PARAM, -0.2, 0.2, 0.f,
                                    "Attack Ring Attenuate");
    configParam<ChainParamQuantity>(DRINGATT_PARAM, -0.2, 0.2, 0.f,
                                    "Decay Ring Attenuate");
    configParam<ChainParamQuantity>(SRINGATT_PARAM, -0.2, 0.2, 0.f,
                                    "Sustain Ring Attenuate");
    configParam<ChainParamQuantity>(RRINGATT_PARAM, -0.2, 0.2, 0.f,
                                    "Release Ring Attenuate");

    // Basic ADSR controls (time,time,level,time)
    configParam(A_PARAM, 0.5, 1.5, 1.1125, "Attack Time");
    configParam(D_PARAM, 0.9, 1.5, 1.216, "Decay Time");
    configParam(S_PARAM, 0.f, 1.f, 0.5, "Sustain Level");
    configParam(R_PARAM, 0.9, 1.6, 1.2682, "Release Time");

    // Output mode buttons
    configButton(AOUTMODEBUTTON_PARAM, "Attack Output Mode");
    configButton(DOUTMODEBUTTON_PARAM, "Decay Output Mode");
    configButton(SOUTMODEBUTTON_PARAM, "Sustain Output Mode");
    configButton(ROUTMODEBUTTON_PARAM, "Release Output Mode");

    // Modulation mode buttons
    configButton(ARINGMODEBUTTON_PARAM, "Attack Modulation Mode");
    configButton(DRINGMODEBUTTON_PARAM, "Decay Modulation Mode");
    configButton(SRINGMODEBUTTON_PARAM, "Sustain Modulation Mode");
    configButton(RRINGMODEBUTTON_PARAM, "Release Modulation Mode");

    // A,D,R curve parameters - Decay get's the default set to be linear based
    // on the value of S_PARAM
    configParam(ACURVE_PARAM, 0.2, 3.f, 1.f, "Attack Curve");
    configParam<BezierParamQuantity>(DCURVE_PARAM, 0.f, 1.3f, 0.75,
                                     "Decay Curve");
    configParam(RCURVE_PARAM, 0.2, 7.4, 1.f, "Release Curve");

    // Force advance buttons
    configButton(AFORCEADV_PARAM, "Attack Force Advance");
    configButton(DFORCEADV_PARAM, "Decay Force Advance");
    configButton(SFORCEADV_PARAM, "Sustain Force Advance");

    // Global controls - Anger controls X-FADE time.
    // Offset acts as attenuator if no ring input
    configParam(ANGER_PARAM, 1.f, 0.f, .5, "Transistion Time Control");
    configParam(GLOBALRINGATT_PARAM, 0.f, 0.2, 0.f, "Gloal Ring Attenuate");
    configParam(GLOBALRINGOFFSET_PARAM, 0.f, 1.f, 1.f, "Global Ring Offset");

    // S&H Section
    configParam(SANDH_PARAM, 40.f, 1.f, 40.f, "S&H Frequency");
    configParam(TRACK_PARAM, 0.f, 1.f, 0.f, "Slew after S&H");

    // Internal parameters for saving state
    configParam(AOUTMODE_PARAM, 0.f, 1.f, 0.f, "Attack Output Mode");
    configParam(DOUTMODE_PARAM, 0.f, 1.f, 0.f, "Decay Output Mode");
    configParam(SOUTMODE_PARAM, 0.f, 2.f, 0.f, "Sustain Output Mode");
    configParam(ROUTMODE_PARAM, 0.f, 1.f, 0.f, "Release Output Mode");
    configParam(ARINGMODE_PARAM, 0.f, 3.f, 0.f, "Attack Modulation Mode");
    configParam(DRINGMODE_PARAM, 0.f, 3.f, 0.f, "Decay Modulation Mode");
    configParam(SRINGMODE_PARAM, 0.f, 1.f, 0.f, "Sustain Modulation Mode");
    configParam(RRINGMODE_PARAM, 0.f, 3.f, 0.f, "Release Modulation Mode");
    configParam(OUTPUTALT_PARAM, 0.f, 3.f, 0.f, "Dry output on negative");
  }

  enum Stage { Attack, Decay, Sustain, Release };

  Stage current_stage = Attack;
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

  // Alt mode in right click menu to switch the negative output from -output to
  // env (dry, no modulation). This is, and sholud only be, used in the code for
  // the menu handling.
  bool menu_use_alt_output = false;

  // Alt mode in R-Click menu to switch the release gate output to an EOR
  // trigger
  // TODO - not yet implimented
  bool menu_release_gate_is_eor = false;

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
    lights[AGATE_LIGHT + 0].setBrightness(lstage == 0 ? 1.f : 0.f);
    lights[DGATE_LIGHT + 0].setBrightness(lstage == 1 ? 1.f : 0.f);
    lights[SGATE_LIGHT + 0].setBrightness(lstage == 2 ? 1.f : 0.f);
    lights[RGATE_LIGHT + 0].setBrightness(lstage == 3 ? 1.f : 0.f);
    outputs[ATTACK_GATE_OUTPUT].setVoltage(lstage == 0 ? 10.f : 0.f);
    outputs[DECAY_GATE_OUTPUT].setVoltage(lstage == 1 ? 10.f : 0.f);
    outputs[SUSTAIN_GATE_OUTPUT].setVoltage(lstage == 2 ? 10.f : 0.f);
    outputs[RELEASE_GATE_OUTPUT].setVoltage(lstage == 3 ? 10.f : 0.f);
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
    if (stage !=
        Attack) {  // the attack stage dosen't need to turn off the release
      // stage's output
      if (outputs[stage - 1].isConnected()) {
        outputs[stage - 1].setVoltage(0.f);
      }
    }
    if (outputs[stage].isConnected()) {
      if (mode == 0) {  // LED OFF output mode, basic env
        outputs[stage].setVoltage(10.f * env);
      } else if (mode == 1) {  // BLUE LED output mode, env w/ modulation
        outputs[stage].setVoltage(10.f * output *
                                  params[GLOBALRINGOFFSET_PARAM].getValue());
      } else {  // GREEN LED output mode, env - DC, only available on Decay
        outputs[stage].setVoltage(10.f * (env - sustain_level));
      }
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
    phasor += simd::pow(.000315,
                        params[A_PARAM].getValue() + attack_time_from_expander);
    env = simd::pow(
        phasor, params[ACURVE_PARAM].getValue() + attack_curve_from_expander);

    if (phasor > 1.0) {
      stage = Decay;
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
        modulation = simd::crossfade(inputs[ATTACK_MOD_INPUT].getVoltage() *
                                         params[ARINGATT_PARAM].getValue(),
                                     inputs[DECAY_MOD_INPUT].getVoltage() *
                                         params[DRINGATT_PARAM].getValue(),
                                     (simd::fmax(0, anger * env - anger + 1)));
      } else {
        modulation = simd::crossfade(inputs[ATTACK_MOD_INPUT].getVoltage() *
                                         params[ARINGATT_PARAM].getValue(),
                                     inputs[ATTACK_MOD_INPUT].getVoltage() *
                                         params[DRINGATT_PARAM].getValue(),
                                     (simd::fmax(0, anger * env - anger + 1)));
      }

      switch ((int)params[ARINGMODE_PARAM].getValue()) {
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
                                       params[DRINGATT_PARAM].getValue(),
                                   (simd::fmax(0, anger * env - anger + 1)));
      output = env;
    }

    displayActive(Attack);
    perStageOutput(Attack, params[AOUTMODE_PARAM].getValue());
    forceAdvance(Attack);  // checks if the force advance is true internally
  }

  void decay_stage(float &phasor, float &env, float &anger, Stage &stage) {
    phasor -= simd::pow(.000315,
                        params[D_PARAM].getValue() + decay_time_from_expander);
    // using bezier curves (ish) because nothing else wanted to
    // work p0 is starting point, p2 is the ending point, p1 is the
    // control point t is the phasor from 0 to 1
    // p1+(1-t)^2*(p0-p1)+t^2*(p2-p1)
    env = (params[DCURVE_PARAM].getValue() + decay_curve_from_expander)  // p1
          + simd::pow((1 - phasor), 2)  //+(1-t)^2
                * (sustain_level - (params[DCURVE_PARAM].getValue() +
                                    decay_curve_from_expander))  //*(p0-p1)
          + simd::pow(phasor, 2)                                 //+t^2
                * (1 - (params[DCURVE_PARAM].getValue() +
                        decay_curve_from_expander));  //*(p2-p1)

    if (phasor <= 0) {
      stage = Sustain;
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
        modulation_source * params[DRINGATT_PARAM].getValue(),
        modulation_dest * params[SRINGATT_PARAM].getValue(),
        (simd::fmax(0, anger * (-env) - anger + (1 / (sustain_level + 0.01)))));
    //.01 is still not perfect, low sustain vals will still cause
    // issues.
    // but, it makes the range of bad values low enough to just let
    // clamping handle the weird situations.
    switch ((int)params[DRINGMODE_PARAM].getValue()) {
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

    displayActive(Decay);
    perStageOutput(Decay, params[DOUTMODE_PARAM].getValue());
    forceAdvance(Decay);  // checks if the force advance is true internally
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
    modulation = modulation_source * params[SRINGATT_PARAM].getValue();

    // Sustain stage has less modes because it's constant
    switch ((int)params[SRINGMODE_PARAM].getValue()) {
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

    displayActive(Sustain);
    perStageOutput(Sustain, params[SOUTMODE_PARAM].getValue());
    forceAdvance(Sustain);  // checks if the force advance is true internally
  }

  void release_stage(float &phasor, float &env, float &anger, Stage &stage) {
    phasor -= simd::pow(
        .000315, params[R_PARAM].getValue() + release_time_from_expander);
    env = simd::pow(
              phasor * (1 / (sustain_level + .00001)),
              params[RCURVE_PARAM].getValue() + release_curve_from_expander) *
          sustain_level;

    displayActive(Release);

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
    modulation =
        simd::crossfade(modulation_source * params[RRINGATT_PARAM].getValue(),
                        modulation_dest * params[SRINGATT_PARAM].getValue(),
                        (simd::fmax(0, anger * (-env) - anger +
                                           (1 / (sustain_level + 0.0001)))));

    switch ((int)params[RRINGMODE_PARAM].getValue()) {
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

    if (phasor <= 0) {
      env = 0.f;
      phasor = 0.f;
      output = 0.f;
      outputs[RELEASE_GATE_OUTPUT].setVoltage(0.f);
      is_running = false;
    }

    perStageOutput(Release, params[ROUTMODE_PARAM].getValue());
  }

  void update_lights(void) {
    lights[AGATE_LIGHT + 2].setBrightness(
        params[AOUTMODE_PARAM].getValue() ? 1.f : 0.f);
    switch ((int)params[DOUTMODE_PARAM].getValue()) {
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
        params[SOUTMODE_PARAM].getValue() ? 1.f : 0.f);

    lights[RGATE_LIGHT + 2].setBrightness(
        params[ROUTMODE_PARAM].getValue() ? 1.f : 0.f);

    setModeLight(Attack, (int)params[ARINGMODE_PARAM].getValue());
    setModeLight(Decay, (int)params[DRINGMODE_PARAM].getValue());
    setModeLight(Sustain, (int)params[SRINGMODE_PARAM].getValue());
    setModeLight(Release, (int)params[RRINGMODE_PARAM].getValue());

    // Yes, I realize it's bad style to not put these in a loop but
    // ¯\_(ツ)_/¯
    if (AOMDetect.process(params[AOUTMODEBUTTON_PARAM].getValue())) {
      params[AOUTMODE_PARAM].setValue(
          ((int)params[AOUTMODE_PARAM].getValue() + 1) % 2);
    }
    if (DOMDetect.process(params[DOUTMODEBUTTON_PARAM].getValue())) {
      params[DOUTMODE_PARAM].setValue(
          ((int)params[DOUTMODE_PARAM].getValue() + 1) % 3);
    }
    if (SOMDetect.process(params[SOUTMODEBUTTON_PARAM].getValue())) {
      params[SOUTMODE_PARAM].setValue(
          ((int)params[SOUTMODE_PARAM].getValue() + 1) % 2);
    }
    if (ROMDetect.process(params[ROUTMODEBUTTON_PARAM].getValue())) {
      params[ROUTMODE_PARAM].setValue(
          ((int)params[ROUTMODE_PARAM].getValue() + 1) % 2);
    }

    // toggle states for Modulation Modes, update mode LED respectively
    if (AMDetect.process(params[ARINGMODEBUTTON_PARAM].getValue())) {
      params[ARINGMODE_PARAM].setValue(
          ((int)params[ARINGMODE_PARAM].getValue() + 1) % 4);
    }
    if (DMDetect.process(params[DRINGMODEBUTTON_PARAM].getValue())) {
      params[DRINGMODE_PARAM].setValue(
          ((int)params[DRINGMODE_PARAM].getValue() + 1) % 4);
    }
    if (SMDetect.process(params[SRINGMODEBUTTON_PARAM].getValue())) {
      params[SRINGMODE_PARAM].setValue(
          ((int)params[SRINGMODE_PARAM].getValue() + 1) % 2);
    }
    if (RMDetect.process(params[RRINGMODEBUTTON_PARAM].getValue())) {
      params[RRINGMODE_PARAM].setValue(
          ((int)params[RRINGMODE_PARAM].getValue() + 1) % 4);
    }
  }

  void ring_active_output(float &output) {
    if (outputs[MAIN_POSITIVE_OUTPUT].isConnected()) {
      outputs[MAIN_POSITIVE_OUTPUT].setVoltage(
          simd::clamp(output * 10.f *
                          (inputs[GLOBAL_RING_INPUT].getVoltage() *
                               params[GLOBALRINGATT_PARAM].getValue() +
                           params[GLOBALRINGOFFSET_PARAM].getValue()),
                      -12.0, 12.0));
    }
    if (outputs[MAIN_NEGATIVE_OUTPUT].isConnected()) {
      if (params[OUTPUTALT_PARAM].getValue()) {
        // Right Click menu option
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(
            simd::clamp(env * 10.f, -12.0, 12.0));
      } else {
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(
            simd::clamp(-1.f * output * 10.f *
                            (inputs[GLOBAL_RING_INPUT].getVoltage() *
                                 params[GLOBALRINGATT_PARAM].getValue() +
                             params[GLOBALRINGOFFSET_PARAM].getValue()),
                        -12.0, 12.0));
      }
    }
  }

  void plain_output(float &env, float &output) {
    // If no ring input connected, the offset knob works as a
    // volume knob, to add more headroom when necessary
    if (outputs[MAIN_POSITIVE_OUTPUT].isConnected()) {
      outputs[MAIN_POSITIVE_OUTPUT].setVoltage(simd::clamp(
          output * (10.f * params[GLOBALRINGOFFSET_PARAM].getValue()), -12.0,
          12.0));
    }

    if (outputs[MAIN_NEGATIVE_OUTPUT].isConnected()) {
      if (params[OUTPUTALT_PARAM].getValue()) {
        // Right click menu option
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(
            simd::clamp(env * 10.f, -12.0, 12.0));
      } else {
        outputs[MAIN_NEGATIVE_OUTPUT].setVoltage(simd::clamp(
            output * (-10.f * params[GLOBALRINGOFFSET_PARAM].getValue()), -12.0,
            12.0));
      }
    }
  }

  void process(const ProcessArgs &args) override {
    // First, we need to get the gate
    bool gate_active = inputs[GATE_INPUT].value > 1.0;

    if (gateDetect.process(gate_active)) {
      is_running = true;
      current_stage = Attack;
    }

    if (is_running) {
      bool retrig = inputs[RETRIG_INPUT].getVoltage() > 1.0;

      if (retrigDetect.process(retrig)) {
        current_stage = Attack;
      }

      float anger = (simd::pow(params[ANGER_PARAM].getValue(), 2) * 8) + 1 -
                    (inputs[ANGER_INPUT].getVoltage() / 10.f);

      sustain_level = params[S_PARAM].getValue() + sustain_level_from_expander;

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
                        (params[SANDH_PARAM].getValue() *
                         (1.f - (inputs[SANDH_INPUT].getVoltage() / 10.f)));
      } else {
        sampleSquare += args.sampleTime * params[SANDH_PARAM].getValue();
      }

      if (sampleSquare >= .5f) {
        sampleSquare = -.5f;
      }

      if (params[SANDH_PARAM].getValue() == 40.f) {
        sampleSquare = 1;  // always update output
      }

      if (gate_active) {
        switch (current_stage) {
          case 0:  // Attack
            attack_stage(phasor, env, anger, current_stage);
            break;
          case 1:  // Decay
            decay_stage(phasor, env, anger, current_stage);
            break;
          case 2:  // Sustain
            sustain_stage(phasor, env, anger, current_stage);
            break;
          default:
            break;
        }
      } else {  // Gate released, phasor == sustain_level already
        current_stage = Release;
      }

      if (current_stage == Release) {
        release_stage(phasor, env, anger, current_stage);
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

    addChild(createWidget<Bolt>(Vec(RACK_GRID_WIDTH * 15, 5)));  // Top
    // addChild(createWidget<Bolt>(Vec(box.size.x - 15 * RACK_GRID_WIDTH,
    // RACK_GRID_HEIGHT - RACK_GRID_WIDTH))); //Bottom

    addParam(createParamCentered<TL1105>(mm2px(Vec(54.916, 14.974)), module,
                                         Vega::AOUTMODEBUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 14.467)), module,
                                          Vega::A_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 14.839)), module,
                                             Vega::ARINGATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 14.839)), module,
                                         Vega::ARINGMODEBUTTON_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 22.839)), module,
                                         Vega::AFORCEADV_PARAM));
    addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 24.119)), module,
                                               Vega::ACURVE_PARAM));
    // golbal
    // addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(29.573, 106.448)),
    // module, Vega::GLOBALRINGATT_PARAM));
    // addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(29.573, 113.94)),
    // module, Vega::GLOBALRINGOFFSET_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(20.23, 120)), module,
                                          Vega::GLOBALRINGOFFSET_PARAM));
    addParam(createParamCentered<SmallHexKnobInv>(
        mm2px(Vec(20.23, 120)), module, Vega::GLOBALRINGATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 38.839)), module,
                                         Vega::DOUTMODEBUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 38.467)), module,
                                          Vega::D_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 38.839)), module,
                                             Vega::DRINGATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 38.839)), module,
                                         Vega::DRINGMODEBUTTON_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 46.839)), module,
                                         Vega::DFORCEADV_PARAM));
    addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 48.119)), module,
                                               Vega::DCURVE_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 63.089)), module,
                                         Vega::SOUTMODEBUTTON_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 62.839)), module,
                                             Vega::SRINGATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 62.839)), module,
                                         Vega::SRINGMODEBUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 66.839)), module,
                                          Vega::S_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(33.02, 70.839)), module,
                                         Vega::SFORCEADV_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(54.448, 87.089)), module,
                                         Vega::ROUTMODEBUTTON_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(8.0, 86.467)), module,
                                          Vega::R_PARAM));
    addParam(createParamCentered<MedHexKnob>(mm2px(Vec(24.844, 86.839)), module,
                                             Vega::RRINGATT_PARAM));
    addParam(createParamCentered<TL1105>(mm2px(Vec(41.196, 86.839)), module,
                                         Vega::RRINGMODEBUTTON_PARAM));
    addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(8.0, 96.118)), module,
                                               Vega::RCURVE_PARAM));
    // addParam(createParamCentered<HexKnob>(mm2px(Vec(46.111, 109.923)),
    // module, Vega::ANGER_PARAM));
    // addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(46.111,122.0)),
    // module, Vega::TRACK_PARAM));
    // addParam(createParamCentered<SmallHexKnob>(mm2px(Vec(37.0,122.0)),
    // module, Vega::SANDH_PARAM));
    addParam(createParamCentered<HexKnob>(mm2px(Vec(32.128, 120)), module,
                                          Vega::SANDH_PARAM));
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
        vega->menu_use_alt_output = !vega->menu_use_alt_output;
        if (vega->paramQuantities[Vega::OUTPUTALT_PARAM]->getValue() == 1.f) {
          vega->paramQuantities[Vega::OUTPUTALT_PARAM]->setValue(0.f);
        } else {
          vega->paramQuantities[Vega::OUTPUTALT_PARAM]->setValue(1.f);
        }
      }
      void step() override {
        rightText = CHECKMARK(
            vega->paramQuantities[Vega::OUTPUTALT_PARAM]->getValue() == 1.f);
      }
    };

    struct VegaOutputEORItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        vega->menu_release_gate_is_eor = !vega->menu_release_gate_is_eor;
      }
      void step() override {
        rightText = CHECKMARK(vega->menu_release_gate_is_eor);
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
        vega->paramQuantities[Vega::A_PARAM]->setValue(
            vega->paramQuantities[Vega::A_PARAM]->getValue() - .1);
        vega->paramQuantities[Vega::D_PARAM]->setValue(
            vega->paramQuantities[Vega::D_PARAM]->getValue() - .1);
        vega->paramQuantities[Vega::R_PARAM]->setValue(
            vega->paramQuantities[Vega::R_PARAM]->getValue() - .1);
      }
    };

    struct VegaIncTimeItem : MenuItem {
      Vega *vega;

      void onAction(const event::Action &e) override {
        vega->paramQuantities[Vega::A_PARAM]->setValue(
            vega->paramQuantities[Vega::A_PARAM]->getValue() + .1);
        vega->paramQuantities[Vega::D_PARAM]->setValue(
            vega->paramQuantities[Vega::D_PARAM]->getValue() + .1);
        vega->paramQuantities[Vega::R_PARAM]->setValue(
            vega->paramQuantities[Vega::R_PARAM]->getValue() + .1);
      }
    };

    menu->addChild(new MenuEntry);
    VegaOutputAltItem *altOutput =
        createMenuItem<VegaOutputAltItem>("Negative Out Dry");
    altOutput->vega = vega;
    menu->addChild(altOutput);

    // VegaOutputEORItem *eorOutput =
    // createMenuItem<VegaOutputEORItem>("Release Gate → EOR Trig");
    // eorOutput->vega = vega; menu->addChild(eorOutput);

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
        "MODULATION MODES:\nRED: Ring\nGREEN: Add\nBLUE: Add With Fade (A,D,R "
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