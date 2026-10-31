#include "../src/plugin.hpp"
#include <cstring>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include "../src/Gamma.cpp"
#include "../src/Delta.cpp"
#include "../src/Sulafat.cpp"
#include "../src/Vega.cpp"
#include "../src/Sheliak.cpp"
#include "../src/Zeta.cpp"
Plugin* pluginInstance = nullptr;
namespace legacy {
#include "legacy/Gamma.hpp"
#include "legacy/Delta.hpp"
#include "legacy/Sulafat.hpp"
#include "legacy/Vega.hpp"
#include "legacy/Sheliak.hpp"
#include "legacy/Zeta.hpp"
}

static size_t comparisons = 0;
static size_t approximateComparisons = 0;
static float maxSimdError = 0.f;
void near(float actual, float expected, const std::string& where) {
  ++approximateComparisons;
  if (actual == expected) return;
  float error = std::fabs(actual - expected);
  maxSimdError = std::max(maxSimdError, error);
  if (!std::isfinite(actual) || !std::isfinite(expected) ||
      error > 2e-5f + 2e-6f * std::fabs(expected))
    throw std::runtime_error(where + ": SIMD error " + std::to_string(error));
}
void equal(float a, float b, const std::string& where) {
  ++comparisons;
  if (std::memcmp(&a, &b, sizeof(float)))
    throw std::runtime_error(where + ": " + std::to_string(a) + " != " + std::to_string(b));
}
void connectOutputs(Module& m) {
  for (auto& output : m.outputs) output.channels = 1;
}
float signal(int frame, int input, int channel) {
  // Independent clocks, bipolar audio and held CV; no random global state.
  if (input % 3 == 0) return ((frame / (7 + channel * 3 + input)) % 2) ? 10.f : 0.f;
  return float((frame * (channel + 3) + input * 23) % 193 - 96) / 13.f;
}

template<class New, class Old>
void mono(const char* name, int mode = -1) {
  for (float rate : {44100.f, 48000.f, 96000.f}) {
    New current;
    Old original;
    connectOutputs(current); connectOutputs(original);
    // Original Sheliak registers were uninitialized; reset before clocking.
    for (int frame = 0; frame < 16000; ++frame) {
      for (size_t p = 0; p < current.params.size(); ++p) {
        if (frame % 997 == 0 && current.paramQuantities[p]) {
          auto* q = current.paramQuantities[p];
          float value = q->minValue + (q->maxValue - q->minValue) * ((p + frame / 997) % 5) / 4.f;
          // Keep Sulafat's legacy integer modulo divisors nonzero.
          if (std::string(name) == "Sulafat" && p >= 3) value = 3.f + p * .1f;
          if (mode >= 0 && p == 0) value = mode;
          if (std::string(name) == "Sheliak") value = std::min(value, 9.f);
          current.params[p].setValue(value); original.params[p].setValue(value);
        }
      }
      for (size_t i = 0; i < current.inputs.size(); ++i) {
        bool connected = frame < 300 ? true : ((frame / 701 + i) % 4 != 0);
        float voltage = signal(frame, i, 0);
        if (std::string(name) == "Sheliak" && i == 1 && frame < 3)
          voltage = frame == 1 ? 10.f : 0.f;
        current.inputs[i].channels = original.inputs[i].channels = connected ? 1 : 0;
        current.inputs[i].setVoltage(connected ? voltage : 0.f);
        original.inputs[i].setVoltage(connected ? voltage : 0.f);
      }
      Module::ProcessArgs args{rate, 1.f / rate, frame};
      current.process(args); original.process(args);
      for (size_t o = 0; o < current.outputs.size(); ++o) {
        int count = original.outputs[o].getChannels();
        if (current.outputs[o].getChannels() != count) throw std::runtime_error(std::string(name) + " mono channel count");
        for (int c = 0; c < count; ++c)
          equal(current.outputs[o].getVoltage(c), original.outputs[o].getVoltage(c), std::string(name) + " mono frame " + std::to_string(frame));
      }
    }
  }
  std::cout << name << " mono mode " << mode << " passed\n";
}

template<class Core>
void poly(const char* name, int mode = -1) {
  Polyphonic<Core> current;
  std::array<Core, 16> reference;
  connectOutputs(current);
  for (auto& voice : reference) connectOutputs(voice);
  for (int frame = 0; frame < 5000; ++frame) {
    const int widths[] = {16, 4, 1, 2, 8, 16};
    int channels = widths[std::min(frame / 800, 5)];
    for (size_t p = 0; p < current.params.size(); ++p) {
      if (frame == 0) {
        float value = current.params[p].getValue();
        if (mode >= 0 && p == 0) value = mode;
        current.params[p].setValue(value);
        for (auto& voice : reference) voice.params[p].setValue(value);
      }
    }
    if (frame > 0 && frame % 1100 == 0) {
      for (size_t p = 0; p < current.params.size(); ++p) {
        auto* q = current.paramQuantities[p];
        if (!q || (mode >= 0 && p == 0)) continue;
        float value = q->minValue + (q->maxValue - q->minValue) * ((frame / 1100 + p) % 5) / 4.f;
        if (std::string(name) == "Sulafat" && p >= 3) value = 3.5f;
        // An unchanged panel value is not an external edit to voice state.
        if (value == current.params[p].getValue()) continue;
        current.params[p].setValue(value);
        for (auto& voice : reference) voice.params[p].setValue(value);
      }
    }
    for (size_t i = 0; i < current.inputs.size(); ++i) {
      // Exercise broadcast, disconnected normals and shorter poly cables.
      int width = i % 4 == 0 ? channels : i % 4 == 1 ? 1 : i % 4 == 2 ? 0 : std::min(3, channels);
      current.inputs[i].channels = width;
      for (int c = 0; c < 16; ++c) current.inputs[i].setVoltage(c < width ? signal(frame, i, c) : 0.f, c);
      for (int c = 0; c < channels; ++c) {
        reference[c].inputs[i].channels = width ? 1 : 0;
        reference[c].inputs[i].setVoltage(width == 1 ? current.inputs[i].getVoltage() : current.inputs[i].getVoltage(c));
      }
    }
    Module::ProcessArgs args{48000.f, 1.f / 48000.f, frame};
    current.process(args);
    for (int c = 0; c < channels; ++c) reference[c].process(args);
    for (size_t o = 0; o < current.outputs.size(); ++o) {
      if ((int)o == PolyTraits<Core>::packedOutput()) continue;
      if (current.outputs[o].getChannels() != channels) throw std::runtime_error(std::string(name) + " poly channel count");
      for (int c = 0; c < channels; ++c)
        if (std::is_same<Core, Vega>::value || std::is_same<Core, Sheliak>::value)
          equal(current.outputs[o].getVoltage(c), reference[c].outputs[o].getVoltage(), std::string(name) + " voice " + std::to_string(c));
        else
          near(current.outputs[o].getVoltage(c), reference[c].outputs[o].getVoltage(), std::string(name) + " voice " + std::to_string(c) + " frame " + std::to_string(frame));
    }
  }
  std::cout << name << " poly mode " << mode << " passed\n";
}

void expanders() {
  // Both original expanders must stay sample-exact in a mono patch.
  Polyphonic<Vega> v;
  legacy::Vega oldV;
  BD383238 bd;
  legacy::BD383238 oldBD;
  v.model = oldV.model = modelVega;
  bd.rightExpander.module = &v;
  oldBD.rightExpander.module = &oldV;
  v.leftExpander.module = &bd;
  bd.model = modelBD383238;
  connectOutputs(v); connectOutputs(oldV);
  Polyphonic<Sheliak> sh;
  legacy::Sheliak oldSh;
  Beta beta;
  legacy::Beta oldBeta;
  beta.model = oldBeta.model = modelBeta;
  sh.rightExpander.module = &beta;
  oldSh.rightExpander.module = &oldBeta;
  connectOutputs(sh); connectOutputs(oldSh);
  connectOutputs(beta); connectOutputs(oldBeta);
  for (int frame = 0; frame < 12000; ++frame) {
    for (size_t i = 0; i < bd.inputs.size(); ++i) {
      int width = (frame / 700 + i) % 3 ? 1 : 0;
      bd.inputs[i].channels = oldBD.inputs[i].channels = width;
      float voltage = width ? signal(frame, i, 0) * .02f : 0.f;
      bd.inputs[i].setVoltage(voltage); oldBD.inputs[i].setVoltage(voltage);
    }
    v.inputs[Vega::GATE_INPUT].channels = oldV.inputs[Vega::GATE_INPUT].channels = 1;
    float gate = (frame / 1700) % 2 ? 10.f : 0.f;
    v.inputs[Vega::GATE_INPUT].setVoltage(gate); oldV.inputs[Vega::GATE_INPUT].setVoltage(gate);
    for (size_t i = 0; i < sh.inputs.size(); ++i) {
      sh.inputs[i].channels = oldSh.inputs[i].channels = 1;
      float voltage = i == 1 ? (frame == 1 ? 10.f : 0.f) : signal(frame, i, 0);
      sh.inputs[i].setVoltage(voltage); oldSh.inputs[i].setVoltage(voltage);
    }
    Module::ProcessArgs args{48000.f, 1.f / 48000.f, frame};
    bd.process(args); oldBD.process(args); v.process(args); oldV.process(args);
    sh.process(args); oldSh.process(args);
    for (size_t o = 0; o < v.outputs.size(); ++o)
      equal(v.outputs[o].getVoltage(), oldV.outputs[o].getVoltage(), "BD383238 mono");
    for (size_t o = 0; o < beta.outputs.size(); ++o)
      equal(beta.outputs[o].getVoltage(), oldBeta.outputs[o].getVoltage(), "Beta mono");
  }

  // Expander CV alone can determine Vega's channel count.
  std::array<Vega, 16> envelopes;
  for (auto& voice : envelopes) connectOutputs(voice);
  Polyphonic<Vega> host;
  BD383238 expander;
  host.model = modelVega;
  expander.model = modelBD383238;
  host.leftExpander.module = &expander;
  expander.rightExpander.module = &host;
  connectOutputs(host);
  for (int frame = 0; frame < 6000; ++frame) {
    for (size_t i = 0; i < expander.inputs.size(); ++i) {
      expander.inputs[i].channels = 16;
      for (int c = 0; c < 16; ++c)
        expander.inputs[i].setVoltage(signal(frame, i, c) * .01f, c);
    }
    host.inputs[Vega::GATE_INPUT].channels = 1;
    host.inputs[Vega::GATE_INPUT].setVoltage((frame / 800) % 2 ? 10.f : 0.f);
    Module::ProcessArgs args{48000.f, 1.f / 48000.f, frame};
    expander.process(args);
    for (int c = 0; c < 16; ++c) {
      Vega& ref = envelopes[c];
      ref.inputs[Vega::GATE_INPUT] = host.inputs[Vega::GATE_INPUT];
      ref.attack_time_from_expander = expander.inputs[0].getVoltage(c) / 10;
      ref.attack_curve_from_expander = expander.inputs[1].getVoltage(c) / 10;
      ref.decay_time_from_expander = expander.inputs[2].getVoltage(c) / 10;
      ref.decay_curve_from_expander = expander.inputs[3].getVoltage(c) / 10;
      ref.sustain_level_from_expander = expander.inputs[4].getVoltage(c) / 10;
      ref.release_time_from_expander = expander.inputs[5].getVoltage(c) / 10;
      ref.release_curve_from_expander = expander.inputs[6].getVoltage(c) / 10;
      ref.process(args);
    }
    host.process(args);
    for (size_t o = 0; o < host.outputs.size(); ++o) {
      if (host.outputs[o].getChannels() != 16) throw std::runtime_error("BD383238 channel count");
      for (int c = 0; c < 16; ++c)
        equal(host.outputs[o].getVoltage(c), envelopes[c].outputs[o].getVoltage(), "BD383238 poly");
    }
  }

  Polyphonic<Sheliak> hostSh;
  Beta out;
  out.model = modelBeta;
  hostSh.rightExpander.module = &out;
  connectOutputs(hostSh); connectOutputs(out);
  std::array<Sheliak, 16> shifts;
  std::array<Beta, 16> sums;
  for (int c = 0; c < 16; ++c) {
    shifts[c].rightExpander.module = &sums[c];
    sums[c].model = modelBeta;
    connectOutputs(shifts[c]); connectOutputs(sums[c]);
  }
  for (int frame = 0; frame < 6000; ++frame) {
    hostSh.params[Sheliak::DELAY_PARAM].setValue(10.f); // Former buffer overrun.
    for (size_t i = 0; i < hostSh.inputs.size(); ++i) {
      hostSh.inputs[i].channels = 16;
      for (int c = 0; c < 16; ++c) {
        float voltage = signal(frame, i, c);
        hostSh.inputs[i].setVoltage(voltage, c);
        shifts[c].inputs[i].channels = 1;
        shifts[c].inputs[i].setVoltage(voltage);
        shifts[c].params[Sheliak::DELAY_PARAM].setValue(10.f);
      }
    }
    Module::ProcessArgs args{48000.f, 1.f / 48000.f, frame};
    hostSh.process(args);
    for (int c = 0; c < 16; ++c) shifts[c].process(args);
    for (size_t o = 0; o < out.outputs.size(); ++o) {
      if (out.outputs[o].getChannels() != 16) throw std::runtime_error("Beta channel count");
      for (int c = 0; c < 16; ++c)
        equal(out.outputs[o].getVoltage(c), sums[c].outputs[o].getVoltage(), "Beta poly");
    }
    for (int bit = 0; bit < hostSh.outputs[Sheliak::POLY_OUTPUT].getChannels(); ++bit)
      equal(hostSh.outputs[Sheliak::POLY_OUTPUT].getVoltage(bit), shifts[0].outputs[Sheliak::POLY_OUTPUT].getVoltage(bit), "packed gate bus");
  }
  std::cout << "Expander mono/poly and packed gate bus passed\n";
}

void simdEdges() {
  Sulafat scalar, vector;
  scalar.foldr = vector.foldr = 3.3f;
  vector.vectorizedShaping = true;
  for (int multiple = -30; multiple <= 30; ++multiple) {
    float center = multiple * scalar.foldr;
    float a[] = {center, std::nextafter(center, -INFINITY),
                 std::nextafter(center, INFINITY), -center};
    float b[4];
    std::memcpy(b, a, sizeof(a));
    scalar.foldBuffer(a, 1.66); vector.foldBuffer(b, 1.66);
    for (int lane = 0; lane < 4; ++lane) near(b[lane], a[lane], "SIMD signed fold boundary");
  }
  std::cout << "SIMD signed fold boundaries passed\n";
}

// Use the same voice adapter with scalar cores as the performance baseline.
struct ScalarGamma : Gamma {};
struct ScalarDelta : Delta {};
struct ScalarSulafat : Sulafat {};
volatile float benchmarkSink = 0.f;
template<class ModuleType>
double benchmarkModule(int mode) {
  ModuleType module;
  connectOutputs(module);
  for (size_t i = 0; i < module.inputs.size(); ++i) {
    module.inputs[i].channels = 16;
    for (int c = 0; c < 16; ++c)
      module.inputs[i].setVoltage(signal(123, i, c), c);
  }
  if (mode >= 0) module.params[0].setValue(mode);
  Module::ProcessArgs args{48000.f, 1.f / 48000.f, 0};
  for (int frame = 0; frame < 1000; ++frame) module.process(args);
  auto start = std::chrono::steady_clock::now();
  for (int frame = 0; frame < 100000; ++frame) {
    args.frame = frame;
    module.inputs[0].setVoltage(float(frame % 73) / 13.f);
    module.process(args);
    benchmarkSink = module.outputs[0].getVoltage();
  }
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}
template<class Vector, class Scalar>
void benchmarkPair(const char* name, int mode = -1) {
  double vector = 1e9, scalar = 1e9;
  for (int trial = 0; trial < 3; ++trial) {
    scalar = std::min(scalar, benchmarkModule<Scalar>(mode));
    vector = std::min(vector, benchmarkModule<Vector>(mode));
  }
  std::cout << name << " 16 voices: " << scalar / vector << "x faster, "
            << vector * 1e4 << " ns/sample SIMD vs " << scalar * 1e4
            << " ns/sample scalar\n";
}

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--benchmark") {
      benchmarkPair<Polyphonic<Gamma>, Polyphonic<ScalarGamma>>("Gamma");
      benchmarkPair<Polyphonic<Delta>, Polyphonic<ScalarDelta>>("Delta");
      benchmarkPair<Polyphonic<Sulafat>, Polyphonic<ScalarSulafat>>("Sulafat fold", 1);
      benchmarkPair<Polyphonic<Sulafat>, Polyphonic<ScalarSulafat>>("Sulafat tangent", 3);
      return 0;
    }
    mono<Polyphonic<Gamma>, legacy::Gamma>("Gamma");
    mono<Polyphonic<Delta>, legacy::Delta>("Delta");
    mono<Polyphonic<Vega>, legacy::Vega>("Vega");
    mono<Polyphonic<Sheliak>, legacy::Sheliak>("Sheliak");
    mono<Zeta, legacy::Zeta>("Zeta");
    for (int mode = 0; mode < 8; ++mode) mono<Polyphonic<Sulafat>, legacy::Sulafat>("Sulafat", mode);
    simdEdges();
    expanders();
    poly<Gamma>("Gamma"); poly<Delta>("Delta"); poly<Vega>("Vega"); poly<Sheliak>("Sheliak");
    for (int mode = 0; mode < 8; ++mode) poly<Sulafat>("Sulafat", mode);
    std::cout << comparisons << " bit-exact sample comparisons passed\n";
    std::cout << approximateComparisons << " SIMD comparisons passed; maximum absolute error " << maxSimdError << " V\n";
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
