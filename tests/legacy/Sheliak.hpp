#include "plugin.hpp"

struct Beta : Module
{
	enum ParamId
	{
		G1A_PARAM,
		G2A_PARAM,
		G3A_PARAM,
		G4A_PARAM,
		G5A_PARAM,
		G6A_PARAM,
		G7A_PARAM,
		G8A_PARAM,
		G1TA_PARAM,
		G2TA_PARAM,
		G3TA_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		INPUTS_LEN
	};
	enum OutputId
	{
		MAIN_EIGHT_OUTPUT,
		FIRST_FOUR_OUTPUT,
		LAST_FOUR_FIFTH_OUTPUT,
		THREE_SIX_TWO_OCT_OUTPUT,
		FMOD_ONE_FOUR_DOWN_ONE_OUTPUT,
		EIGHT_TRIPLET_ARP_OUTPUT,
		TRIPLET_ONE_FOUR_IF_HIGH_OUTPUT,
		TRIPLET_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		MAIN_EIGHT_LIGHT,
		FIRST_FOUR_LIGHT,
		LAST_FOUR_FIFTH_LIGHT,
		THREE_SIX_TWO_OCT_LIGHT,
		FMOD_ONE_FOUR_DOWN_ONE_LIGHT,
		EIGHT_TRIPLET_ARP_LIGHT,
		TRIPLET_ONE_FOUR_IF_HIGH_LIGHT,
		TRIPLET_LIGHT,
		LIGHTS_LEN
	};

	Beta()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configOutput(MAIN_EIGHT_OUTPUT, "1-8");
		configOutput(FIRST_FOUR_OUTPUT, "1-4");
		configOutput(LAST_FOUR_FIFTH_OUTPUT, "5-8 + p5");
		configOutput(THREE_SIX_TWO_OCT_OUTPUT, "3-6 + 2oct");
		configOutput(FMOD_ONE_FOUR_DOWN_ONE_OUTPUT, "1-4 % oct, -1 oct");
		configOutput(EIGHT_TRIPLET_ARP_OUTPUT, "1-8, +oct per triplet bit");
		configOutput(TRIPLET_ONE_FOUR_IF_HIGH_OUTPUT, "(1-4 + 1t-3t) IF (g1 & gt1)");
		configOutput(TRIPLET_OUTPUT, "1t-3t");

		const float low_range = 0.f;
		const float high_range = 1.f;
		const float default_attenuation = 1.f / 12.f;

		configParam(G1A_PARAM, low_range, high_range, default_attenuation, "G1 Attenuversion");
		configParam(G2A_PARAM, low_range, high_range, default_attenuation, "G2 Attenuversion");
		configParam(G3A_PARAM, low_range, high_range, default_attenuation, "G3 Attenuversion");
		configParam(G4A_PARAM, low_range, high_range, default_attenuation, "G4 Attenuversion");
		configParam(G5A_PARAM, low_range, high_range, default_attenuation, "G5 Attenuversion");
		configParam(G6A_PARAM, low_range, high_range, default_attenuation, "G6 Attenuversion");
		configParam(G7A_PARAM, low_range, high_range, default_attenuation, "G7 Attenuversion");
		configParam(G8A_PARAM, low_range, high_range, default_attenuation, "G8 Attenuversion");

		configParam(G1TA_PARAM, low_range, high_range, default_attenuation, "Tripplet G1 Attenuversion");
		configParam(G2TA_PARAM, low_range, high_range, default_attenuation, "Tripplet G2 Attenuversion");
		configParam(G3TA_PARAM, low_range, high_range, default_attenuation, "Tripplet G3 Attenuversion");
	}

	void process(const ProcessArgs &args) override
	{
	}
};

struct Sheliak : Module
{
	enum ParamId
	{
		DELAY_PARAM,
		PARAMS_LEN
	};
	enum InputId
	{
		CLOCK_INPUT,
		RESET_INPUT,
		DATA_INPUT,
		XOR_INPUT,
		INPUTS_LEN
	};
	enum OutputId
	{
		G1_OUTPUT,
		G2_OUTPUT,
		G3_OUTPUT,
		G4_OUTPUT,
		G5_OUTPUT,
		G6_OUTPUT,
		G7_OUTPUT,
		G8_OUTPUT,
		POLY_OUTPUT,
		GT1_OUTPUT,
		GT2_OUTPUT,
		GT3_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId
	{
		G1L_LIGHT,
		G2L_LIGHT,
		G3L_LIGHT,
		G4L_LIGHT,
		G5L_LIGHT,
		G6L_LIGHT,
		G7L_LIGHT,
		G8L_LIGHT,
		GT1L_LIGHT,
		GT2L_LIGHT,
		GT3L_LIGHT,
		G1L_LIGHT_P,
		G2L_LIGHT_P,
		G3L_LIGHT_P,
		G4L_LIGHT_P,
		G5L_LIGHT_P,
		G6L_LIGHT_P,
		G7L_LIGHT_P,
		G8L_LIGHT_P,
		GT1L_LIGHT_P,
		GT2L_LIGHT_P,
		GT3L_LIGHT_P,
		CLOCKL_LIGHT,
		// RESETL_LIGHT,
		DATAL_LIGHT,
		XORL_LIGHT,
		LIGHTS_LEN
	};

private:
	uint8_t shiftRegister;
	uint8_t tripletShiftRegister;

	int clockCounter;

	bool clockShiftBuffer[10];

	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger xorTrigger;

public:
	Sheliak()
	{
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(DATA_INPUT, "Data");
		configInput(XOR_INPUT, "XOR");

		configOutput(G1_OUTPUT, "G1");
		configOutput(G2_OUTPUT, "G2");
		configOutput(G3_OUTPUT, "G3");
		configOutput(G4_OUTPUT, "G4");
		configOutput(G5_OUTPUT, "G5");
		configOutput(GT1_OUTPUT, "triplet G1");
		configOutput(G6_OUTPUT, "G6");
		configOutput(GT2_OUTPUT, "triplet G2");
		configOutput(G7_OUTPUT, "G7");
		configOutput(GT3_OUTPUT, "triplet G3");
		configOutput(G8_OUTPUT, "G8");

		configOutput(POLY_OUTPUT, "Polyphonic Gate");

		configParam(DELAY_PARAM, 0.0f, 10.0f, 0.0f, "Delay Samples");

		clockCounter = 0;

		// Initialize the buffer
		std::fill_n(clockShiftBuffer, 10, 0.0f);
	}

	void process(const ProcessArgs &args) override
	{
		// Read delay parameter value
		float delaySamples = params[DELAY_PARAM].getValue();

		// Read inputs
		bool clockInput = false;

		if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage()))
		{
			clockInput = true;
		}

		bool resetInput = false;

		if (resetTrigger.process(inputs[RESET_INPUT].getVoltage()))
		{
			resetInput = true;
		}

		// lights[RESETL_LIGHT].value = resetInput;

		bool dataInput = inputs[DATA_INPUT].getVoltage() > 1.f;

		lights[DATAL_LIGHT].value = dataInput;

		bool xorInput = inputs[XOR_INPUT].getVoltage() > 1.f;

		lights[XORL_LIGHT].value = xorInput;

		// Delay the clock input
		int delaySamplesInt = static_cast<int>(delaySamples);
		bool delayedClockInput = clockShiftBuffer[delaySamplesInt];

		for (int i = 9; i > 0; --i)
		{
			clockShiftBuffer[i] = clockShiftBuffer[i - 1];
		}

		clockShiftBuffer[0] = clockInput;

		// Handle reset
		if (resetInput)
		{
			shiftRegister = 0;
			tripletShiftRegister = 0;
			clockCounter = 0;
		}

		if (delayedClockInput)
		{
			clockCounter = (clockCounter + 1) % 12;

			// Shift the main register every 4 clock pulses (16th notes)
			if (clockCounter % 4 == 0)
			{
				// Update the clock light
				lights[CLOCKL_LIGHT].value = 1.f;

				// Shift the main register to the left
				shiftRegister = ((shiftRegister << 1) | ((int)dataInput & 1)) ^ ((int)xorInput & 1);

				// Update the main outputs based on the shifted values
				outputs[G1_OUTPUT].setVoltage((shiftRegister & 0x01) ? 10.0f : 0.0f);
				outputs[G2_OUTPUT].setVoltage((shiftRegister & 0x02) ? 10.0f : 0.0f);
				outputs[G3_OUTPUT].setVoltage((shiftRegister & 0x04) ? 10.0f : 0.0f);
				outputs[G4_OUTPUT].setVoltage((shiftRegister & 0x08) ? 10.0f : 0.0f);
				outputs[G5_OUTPUT].setVoltage((shiftRegister & 0x10) ? 10.0f : 0.0f);
				outputs[G6_OUTPUT].setVoltage((shiftRegister & 0x20) ? 10.0f : 0.0f);
				outputs[G7_OUTPUT].setVoltage((shiftRegister & 0x40) ? 10.0f : 0.0f);
				outputs[G8_OUTPUT].setVoltage((shiftRegister & 0x80) ? 10.0f : 0.0f);

				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x01) ? 10.0f : 0.0f, 0);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x02) ? 10.0f : 0.0f, 1);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x04) ? 10.0f : 0.0f, 2);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x08) ? 10.0f : 0.0f, 3);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x10) ? 10.0f : 0.0f, 4);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x20) ? 10.0f : 0.0f, 5);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x40) ? 10.0f : 0.0f, 6);
				outputs[POLY_OUTPUT].setVoltage((shiftRegister & 0x80) ? 10.0f : 0.0f, 7);

				// Update the output lights
				lights[G1L_LIGHT].value = (shiftRegister & 0x01) ? 1.0f : 0.0f;
				lights[G2L_LIGHT].value = (shiftRegister & 0x02) ? 1.0f : 0.0f;
				lights[G3L_LIGHT].value = (shiftRegister & 0x04) ? 1.0f : 0.0f;
				lights[G4L_LIGHT].value = (shiftRegister & 0x08) ? 1.0f : 0.0f;
				lights[G5L_LIGHT].value = (shiftRegister & 0x10) ? 1.0f : 0.0f;
				lights[G6L_LIGHT].value = (shiftRegister & 0x20) ? 1.0f : 0.0f;
				lights[G7L_LIGHT].value = (shiftRegister & 0x40) ? 1.0f : 0.0f;
				lights[G8L_LIGHT].value = (shiftRegister & 0x80) ? 1.0f : 0.0f;

				lights[G1L_LIGHT_P].value = (shiftRegister & 0x01) ? 1.0f : 0.0f;
				lights[G2L_LIGHT_P].value = (shiftRegister & 0x02) ? 1.0f : 0.0f;
				lights[G3L_LIGHT_P].value = (shiftRegister & 0x04) ? 1.0f : 0.0f;
				lights[G4L_LIGHT_P].value = (shiftRegister & 0x08) ? 1.0f : 0.0f;
				lights[G5L_LIGHT_P].value = (shiftRegister & 0x10) ? 1.0f : 0.0f;
				lights[G6L_LIGHT_P].value = (shiftRegister & 0x20) ? 1.0f : 0.0f;
				lights[G7L_LIGHT_P].value = (shiftRegister & 0x40) ? 1.0f : 0.0f;
				lights[G8L_LIGHT_P].value = (shiftRegister & 0x80) ? 1.0f : 0.0f;
			}
			else
			{
				lights[CLOCKL_LIGHT].value = 0.f;
			}

			// Shift the triplet register every 3 clock pulses (triplets)
			if (clockCounter % 3 == 0)
			{
				// Shift the triplet register to the left
				tripletShiftRegister = ((tripletShiftRegister << 1) | ((int)dataInput & 1)) ^ ((int)xorInput & 1);

				// Update the triplet outputs based on the shifted values
				outputs[GT1_OUTPUT].setVoltage((tripletShiftRegister & 0x01) ? 10.0f : 0.0f);
				outputs[GT2_OUTPUT].setVoltage((tripletShiftRegister & 0x02) ? 10.0f : 0.0f);
				outputs[GT3_OUTPUT].setVoltage((tripletShiftRegister & 0x04) ? 10.0f : 0.0f);

				outputs[POLY_OUTPUT].setVoltage((tripletShiftRegister & 0x01) ? 10.0f : 0.0f, 8);
				outputs[POLY_OUTPUT].setVoltage((tripletShiftRegister & 0x02) ? 10.0f : 0.0f, 9);
				outputs[POLY_OUTPUT].setVoltage((tripletShiftRegister & 0x04) ? 10.0f : 0.0f, 10);

				// Update the triplet output lights
				lights[GT1L_LIGHT].value = (shiftRegister & 0x01) ? 1.0f : 0.0f;
				lights[GT2L_LIGHT].value = (shiftRegister & 0x02) ? 1.0f : 0.0f;
				lights[GT3L_LIGHT].value = (shiftRegister & 0x04) ? 1.0f : 0.0f;

				lights[GT1L_LIGHT_P].value = (shiftRegister & 0x01) ? 1.0f : 0.0f;
				lights[GT2L_LIGHT_P].value = (shiftRegister & 0x02) ? 1.0f : 0.0f;
				lights[GT3L_LIGHT_P].value = (shiftRegister & 0x04) ? 1.0f : 0.0f;
			}

			outputs[POLY_OUTPUT].setChannels(11);

			Module *rightModule = getRightExpander().module;

			if (rightModule && rightModule->model == modelBeta)
			{
				// We are now certain that rightModule is a Beta.
				// We can process its params, ports, and lights.
				// This probably could be done a lot cleaner than making a ton of new variables
				// but it's not like I'll be adding more later, and this is easy to read.
				float g1att = rightModule->params[Beta::G1A_PARAM].getValue() * (shiftRegister & 0x01 ? 1.0 : 0.0);
				float g2att = rightModule->params[Beta::G2A_PARAM].getValue() * (shiftRegister & 0x02 ? 1.0 : 0.0);
				float g3att = rightModule->params[Beta::G3A_PARAM].getValue() * (shiftRegister & 0x04 ? 1.0 : 0.0);
				float g4att = rightModule->params[Beta::G4A_PARAM].getValue() * (shiftRegister & 0x08 ? 1.0 : 0.0);
				float g5att = rightModule->params[Beta::G5A_PARAM].getValue() * (shiftRegister & 0x10 ? 1.0 : 0.0);
				float g6att = rightModule->params[Beta::G6A_PARAM].getValue() * (shiftRegister & 0x20 ? 1.0 : 0.0);
				float g7att = rightModule->params[Beta::G7A_PARAM].getValue() * (shiftRegister & 0x40 ? 1.0 : 0.0);
				float g8att = rightModule->params[Beta::G8A_PARAM].getValue() * (shiftRegister & 0x80 ? 1.0 : 0.0);

				float g1tatt = rightModule->params[Beta::G1TA_PARAM].getValue() * (tripletShiftRegister & 0x01 ? 1.0 : 0.0);
				float g2tatt = rightModule->params[Beta::G2TA_PARAM].getValue() * (tripletShiftRegister & 0x02 ? 1.0 : 0.0);
				float g3tatt = rightModule->params[Beta::G3TA_PARAM].getValue() * (tripletShiftRegister & 0x04 ? 1.0 : 0.0);

				// Get needed sums
				float first_four = g1att + g2att + g3att + g4att;
				float last_four = g5att + g6att + g7att + g8att;
				float all_eight = first_four + last_four;
				float three_through_six = g3att + g4att + g5att + g6att;

				// 1-8 sum
				rightModule->getOutput(Beta::MAIN_EIGHT_OUTPUT).setVoltage(all_eight);
				rightModule->getLight(Beta::MAIN_EIGHT_LIGHT).value = (all_eight / 10.f);

				// 1-4 sum
				rightModule->getOutput(Beta::FIRST_FOUR_OUTPUT).setVoltage(first_four);
				rightModule->getLight(Beta::FIRST_FOUR_LIGHT).value = (first_four / 10.f);

				// 5-8 sum + p5
				rightModule->getOutput(Beta::LAST_FOUR_FIFTH_OUTPUT).setVoltage(last_four + (7.f / 12.f));
				rightModule->getLight(Beta::LAST_FOUR_FIFTH_LIGHT).value = (last_four / 10.f);

				// 3-6 sum + 2oct
				rightModule->getOutput(Beta::THREE_SIX_TWO_OCT_OUTPUT).setVoltage(three_through_six + 2.f);
				rightModule->getLight(Beta::THREE_SIX_TWO_OCT_LIGHT).value = (three_through_six / 10.f);

				// 1-4, down an octave, only updates when 1 and 2 are high, octave constrained. (bass)
				if (shiftRegister & 0x3)
				{
					rightModule->getOutput(Beta::FMOD_ONE_FOUR_DOWN_ONE_OUTPUT).setVoltage(rack::simd::fmod(first_four, 1.f) - 1);
					rightModule->getLight(Beta::FMOD_ONE_FOUR_DOWN_ONE_LIGHT).value = (rack::simd::fmod(first_four, 1.f) / 10.f);
				}

				// 1-8 + 1 oct for each high triplet bit (arp)
				rightModule->getOutput(Beta::EIGHT_TRIPLET_ARP_OUTPUT).setVoltage(all_eight + (tripletShiftRegister & 0x3) - 2.f);
				rightModule->getLight(Beta::EIGHT_TRIPLET_ARP_LIGHT).value = ((all_eight + (tripletShiftRegister & 0x3)) / 10.f);

				// triplet + 1-4 sum, only updates if both triplet 1 and main bit 1 are high
				if ((tripletShiftRegister & 0x01) && (shiftRegister & 0x01))
				{
					rightModule->getOutput(Beta::TRIPLET_ONE_FOUR_IF_HIGH_OUTPUT).setVoltage(g1tatt + g2tatt + g3tatt + first_four);
					rightModule->getLight(Beta::TRIPLET_ONE_FOUR_IF_HIGH_LIGHT).value = ((g1tatt + g2tatt + g3tatt + first_four) / 10.f);
				}

				// triplet sum
				rightModule->getOutput(Beta::TRIPLET_OUTPUT).setVoltage(g1tatt + g2tatt + g3tatt);
				rightModule->getLight(Beta::TRIPLET_LIGHT).value = ((g1tatt + g2tatt + g3tatt) / 10.f);
			}
		}
	}
};
