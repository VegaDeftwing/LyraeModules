# DSP regression tests

Run `make test RACK_DIR=/path/to/Rack-SDK` from the repository root. The tests
link the SDK's real libRack and compile with the plugin's compiler flags.
The supplied runner sets `LD_LIBRARY_PATH` for Linux.

`legacy/` contains the original module DSP definitions from commit
`b6dde88e4d780de0207ece26c5b7de23d61c84e6`, with widgets and model factories
removed. Keep these snapshots unchanged: they provide an independent baseline
rather than comparing the new implementation against itself. They retain the
repository's GPL-3.0-or-later license.

Mono regressions and scalar polyphonic paths compare float bit patterns without
a tolerance. SIMD polyphonic paths allow an absolute error of 20 microvolts
plus 2 parts per million of the expected voltage. The suite reports the maximum
observed SIMD error and covers:

- Mono output samples and channel counts at 44.1, 48, and 96 kHz, including
  parameter edits, changing connections, trigger edges and all Sulafat modes.
- Mono Vega/BD383238 and Sheliak/Beta expander operation against original DSP.
- Independent voices against separate mono instances, including mono input
  broadcasting, disconnected inputs, shorter polyphonic inputs, and channel
  counts changing through 16, 4, 1, 2, 8, and 16.
- Polyphonic CV from BD383238, including CV determining Vega's channel count.
- SIMD folding immediately around positive and negative discontinuities.
- Beta's eight polyphonic outputs, Sheliak's preserved packed gate layout,
  and the maximum clock delay.

The original Sheliak registers are uninitialized until reset. Mono reference
comparisons reset before the first clock and restrict delay to 0–9, the range
that had defined behavior. The corrected delay of 10 is tested separately.
Sulafat's pre-existing integer modulo modes are tested with nonzero divisors;
zero fold settings in those modes were already undefined and are outside the
sample-exact compatibility claim.

A sanitizer run on Linux can be built with:

```sh
g++ -std=c++11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Isrc -I"$RACK_DIR/include" -I"$RACK_DIR/dep/include" \
  tests/regression.cpp -L"$RACK_DIR" -lRack -o build/tests/regression-sanitized
LD_LIBRARY_PATH="$RACK_DIR" ASAN_OPTIONS=detect_leaks=0 build/tests/regression-sanitized
```

Leak checking is disabled because the test includes process-lifetime Rack model
factories. Address and undefined-behavior checks remain enabled.

Run `make benchmark RACK_DIR=/path/to/Rack-SDK` to compare the SIMD paths
against the same 16-voice adapter using scalar cores. The benchmark reports the
best of three 100,000-sample runs with fixed parameters and changing audio;
results depend on CPU, compiler, inputs and mode. This isolates the SIMD
changes rather than measuring Rack GUI or audio driver performance.

## Aladfar oscillator

`make test` also runs the separate `tests/aladfar.cpp` harness. It checks the
manual's ramp/offset/balance diagrams; 16 SIMD voices against independent mono
instances; mono broadcast and short cables; channel-count changes; PWM isolation;
zero-drive mute; both resonance routes; sync level/polarity; malformed CV; five
sample rates; and measured 1 V/oct pitch tracking. Coherent FFT tests compare
inharmonic energy against an uncorrected 1x version of the same signal path.
These numerical checks do not establish a match to a physical MIG VCO.

Use `make benchmark-aladfar RACK_DIR=/path/to/Rack-SDK` to measure one and sixteen
voices with resonance enabled. This nonlinear model costs more than a basic
saw oscillator; performance depends on sample rate and host CPU.

Aladfar also checks the 1J24B curve checkpoints and transconductance, load-line
residuals, the ADAA integral against independent double-precision antiderivatives,
low-frequency phase accumulation, unused-output independence, and compensated
resonance tuning across five octaves, both feedback modes and 48/96 kHz.
Run `make test-aladfar-sanitized RACK_DIR=/path/to/Rack-SDK` for address,
undefined-behavior and leak checking of the oscillator harness.

High-resonance texture is checked using normalized energy above 3 kHz for a
110 Hz oscillator in both modes. This ensures the upper range adds audible
high-frequency content rather than simply raising output gain. The pitch tests
continue to cover maximum resonance, including the regenerative region. The
maximum setting is measured over a longer interval because successive bursty
cycles vary; the compensated mean carrier must retain the same 0.1% tolerance.
Texture tests require at least 6 dB more normalized high-frequency energy and
check that no samples reach the final host-rate safety clamp in that fixture.
