#pragma once
#include <array>
#include <memory>

// Keep independent voice state; traits can batch suitable DSP kernels in SIMD.
// Construct all storage off the audio thread. Channel zero is the actual module,
// so existing widgets, patch parameters and expander pointers remain valid.
template <class Core> struct PolyTraits {
  static int extraChannels(Core&) { return 0; }
  static int packedOutput() { return -1; }
  static void prepare(Core&, Core&, int) {}
  static void finish(Core&, int) {}
};

template <class Core> struct Polyphonic : Core {
  std::array<std::unique_ptr<Core>, 15> voices;
  std::vector<float> previousParams;
  bool firstFrame = true;

  Polyphonic() : previousParams(this->params.size()) {
    for (auto& voice : voices)
      voice.reset(new Core);
  }

  Core* polyVoice(int channel) override {
    return channel == 0 ? this : voices[channel - 1].get();
  }

  void process(const rack::engine::Module::ProcessArgs& args) override {
    int channels = std::max(1, PolyTraits<Core>::extraChannels(*this));
    for (auto& input : this->inputs)
      channels = std::max(channels, input.getChannels());
    channels = std::min(channels, 16);

    // Process additional voices before channel zero publishes button/state
    // changes. Only external parameter edits overwrite a voice's own state.
    for (size_t p = 0; p < this->params.size(); ++p) {
      float value = this->params[p].getValue();
      if (firstFrame || value != previousParams[p])
        for (auto& voice : voices)
          voice->params[p].setValue(value);
    }
    for (int c = 1; c < channels; ++c) {
      Core& voice = *voices[c - 1];
      for (size_t i = 0; i < this->inputs.size(); ++i) {
        auto& source = this->inputs[i];
        voice.inputs[i].channels = source.isConnected() ? 1 : 0;
        voice.inputs[i].setVoltage(source.getPolyVoltage(c));
      }
      for (size_t o = 0; o < this->outputs.size(); ++o)
        voice.outputs[o].channels = this->outputs[o].isConnected() ? 1 : 0;
      PolyTraits<Core>::prepare(*this, voice, c);
      voice.Core::process(args);
    }
    Core::process(args);
    for (size_t o = 0; o < this->outputs.size(); ++o) {
      // A packed bus already uses its channels for distinct gate bits.
      if ((int)o == PolyTraits<Core>::packedOutput())
        continue;
      this->outputs[o].setChannels(channels);
      for (int c = 1; c < channels; ++c)
        this->outputs[o].setVoltage(voices[c - 1]->outputs[o].getVoltage(), c);
    }
    PolyTraits<Core>::finish(*this, channels);
    for (size_t p = 0; p < this->params.size(); ++p)
      previousParams[p] = this->params[p].getValue();
    firstFrame = false;
  }
};
