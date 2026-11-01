# Working on Aladfar

## File map

| File | Responsibility |
|---|---|
| `src/Aladfar.cpp` | Stable Rack parameter/port IDs, CV normalization, channel count and four SIMD banks |
| `src/AladfarUI.hpp` | Control widgets and coordinates; no DSP |
| `src/dsp/Tube1J24B.hpp` | Approximate device currents in mA, electrode voltages in V, and loaded-plate solution |
| `src/dsp/Aladfar.hpp` | Core waveform, feedback state integration, sync, tuning compensation, saturation, folding and output conditioning |
| `tests/aladfar.cpp` | Independent device/waveform checks, routing, pitch, texture, spectra and benchmarks |
| `res/Aladfar.svg` | Runtime panel with separate background, control-area and outlined-lettering layers |
| `res/generate_aladfar_panel.py` | Development-only panel generator using fonttools and DejaVu Serif |

The user-facing controls are described in the repository README. Equations,
sources, estimated circuit values, and antialiasing limits are in
[the model notes](Aladfar-model.md). Confirmed review findings are tracked
separately in [the review notes](Aladfar-review.md).

## DSP flow and state

Each `Bank` contains four independent voices in Rack `float_4` lanes. Default
processing is 8x the host rate; the four folding stages run at 16x. The output
order is Raw, Pulse, Smooth, Fold, with connection-mask bits 1, 2, 4, 8.

`Bank::process()` keeps these steps in order:

1. Interpolate host-frame controls; take a midpoint step through the coupling
   capacitors, screen supply and charging-current model.
2. Advance active-core charge and apply level-sensitive external sync.
3. Accumulate requested/observed cycle advance and update tuning compensation.
4. Recover inactive cores, retain each conduction branch and read Raw.
5. Compute the optional Pulse comparator and edge corrections.
6. Compute the optional biased Smooth stage, then the additional Fold stages.
7. Remove DC, apply internal-rate headroom compression and decimate outputs.

The charging phase and RC states remain independent of which processed outputs
are connected. Optional paths pause their own histories when disconnected;
when all outputs are disconnected the Rack adapter returns without processing.
The effect of reconnection is covered in the review notes.

Parameter and port enum ordering is saved in Rack patches. Preserve those IDs,
control defaults, and widget positions unless deliberately changing the module
interface. The helper `Controls` values are normalized except for frequency in
Hz and the level-scaled sync signal in V. Device current functions return mA;
the loaded-plate resistance is in kohm to match those units.

## Formatting

The format is scoped to Aladfar rather than imposing a style on older modules:

```sh
clang-format --style=file:docs/Aladfar-format.yaml -i \
  src/Aladfar.cpp src/AladfarUI.hpp \
  src/dsp/Aladfar.hpp src/dsp/Tube1J24B.hpp tests/aladfar.cpp
```

The style keeps include order and string literals intact, expands short control
flow, and uses four-space indentation with a 100-column limit. Keep DSP
expressions and their evaluation order unchanged during a cosmetic pass:
regenerative feedback is sensitive to small floating-point changes. Changes to
solver ordering, clamps, literals, state update timing or sample-rate behavior
belong in a separate behavioral change with validation.

## Verification

From the repository root, with the Rack SDK path set:

```sh
make -j4 RACK_DIR=/path/to/Rack-SDK
make test RACK_DIR=/path/to/Rack-SDK
make benchmark-aladfar RACK_DIR=/path/to/Rack-SDK
make test-aladfar-sanitized RACK_DIR=/path/to/Rack-SDK
```

`make test` runs both the collection's frozen mono comparisons and Aladfar's
separate oscillator harness. The sanitizer target instruments the oscillator
harness for address, undefined-behavior and leak checks. High-resonance tuning
uses a longer averaging interval because successive regenerative cycles vary;
this makes the full sanitizer run relatively slow. Tests are headless and do
not need a running Rack application or audio device.

The optional diagnostic command prints texture measurements for both modes:

```sh
LD_LIBRARY_PATH=/path/to/Rack-SDK build/tests/aladfar --resonance-report
```

See [the test notes](../tests/README.md) for tolerances and coverage. The portable
[demo patch](../tests/patches/aladfar.vcv) uses Core and Fundamental alongside
Aladfar; select an audio device, then use the computer keyboard to play chords.

## Panel edits

Edit the SVG's Background layer directly to add artwork. The generator rebuilds
and replaces the entire SVG, including that layer, so use regeneration when
intentionally rebuilding the layout:

```sh
python res/generate_aladfar_panel.py /path/to/DejaVuSerif.ttf
```

The generator needs fonttools only during development. Outlined labels render
in Rack without shipping a TTF or relying on a system serif font. The runtime
widget coordinates must be kept aligned with the SVG if the layout changes.
