# Lyrae Modules

## Polyphony

Gamma, Delta, Sulafat, and Vega support up to 16 independent voices. The
widest connected input determines the output channel count (at least one).
Mono inputs broadcast to every voice; missing channels on shorter polyphonic
inputs supply 0 V. Existing disconnected-input normalizations apply per voice.
Knobs and buttons control all voices; trigger, envelope, filter, and sequencing
state is independent. Panel lights show voice 1. Inactive voices pause and
retain their state when the channel count shrinks.

BD383238 supports polyphonic CV for each Vega voice, and its inputs also
contribute to Vega's channel count. Sheliak's individual gate outputs and its
Beta expander support 16 independent sequences. Sheliak's packed **Polyphonic
Gate** output keeps its original layout: eight main bits followed by three
triplet bits, from voice 1. A Rack cable cannot carry all eleven bits from all
sixteen voices. Zeta is a blank panel with no signal inputs or outputs.

Gamma uses Rack's four-lane SIMD for polyphonic signal arithmetic. Delta
batches output saturation across voices; Sulafat vectorizes its fold and
tangent waveshaping across oversamples. Stateful control flow remains scalar,
and parameter edits are distributed only when they change. SIMD polyphonic
math may differ by a few microvolts from scalar processing.

Existing mono DSP arithmetic and timing are preserved. Sheliak now initializes
its registers to zero and safely supports its maximum 10-sample delay; its
previous startup contents and out-of-bounds delay behavior were undefined.

## Building and testing

Download the [Rack SDK](https://vcvrack.com/downloads/) for your platform, then:

```sh
make -j4 RACK_DIR=/path/to/Rack-SDK
make test RACK_DIR=/path/to/Rack-SDK
make benchmark RACK_DIR=/path/to/Rack-SDK
```

The headless regression suite uses the real Rack engine and needs no running
Rack application or audio device. See [tests/README.md](tests/README.md) for
coverage and the frozen mono reference implementation.

## Sulafat
> Sulafat has exhausted the supply of hydrogen at its core, and so burns much brighter.
> Sulafat is a wavefolder, and so will make your sound much brighter too!

<img src="./images/Sulafat.png" alt="alt text" width="300">

Sulafat is a multi-mode wavefolder (and a bit more) with some internal modulation that differs between the two channels, with internal normalizing of the left (top) input to the right (bottom) input


0. Bypass
1. Basic Wavefolder
2. 5-level Quantized Wavefolder
3. Tangent + Clamping (sorta still a wavefolder)
4. Split mix of 1 + low-passed 2. Split is inverted on second channel
5. Ring mod - uses internal LFOs if only one input
6. S&H-ish creates some artifacting depending on the input
7. Wavefolding + S&H + LFO mess. This one you just sort of need to hook up to a scope

There's also an easter egg LFO mode, if the top (left) input is left disconnected the two outputs output the internal LFO's.
The left (top) output will be a basic LFO, while the bottom (right) output will be the two LFO's ringmodd'd together.

## γ Lyrae
> It bore the traditional names Sulafat (Sulaphat), from the Arabic السلحفاة al-sulḥafāt "turtle" 
> This module will put a DC offset shell around your signal

<img src="./images/Gamma.png" alt="alt text" width="300">

Gamma Lyrae is a companion module to Sulafat. With nothing connected, the middle knob will generate two DC voltages, with one being the inverse of the other. The knob can be turned both ways to swap which side is going negative. If a CV input is provided, the knob becomes an attenuverter, but the operation is still effectively the same.

The main inputs (at the bottom) are normalled. These inputs are multiplied (ring modulated) with the center input. This creates two output signals with opposite DC offsets. This will strongly impact the signal fed into the wavofolding modes of Sulafat.

## Vega
> Vega is rotating rapidly with a velocity of 236 km/s at the equator. This causes the equator to bulge outward due to centrifugal effects, and, as a result, there is a variation of temperature across the star's photosphere that reaches a maximum at the poles.
> This module will spin the segments of an envelope generator at high velocities to help you reach new sonic territory

DEMO: https://www.youtube.com/watch?v=aSt3DRtH8tI

<img src="./images/Vega.png" alt="Vega ADSR" width="220">

Vega is an ADSR with multiple per stage modulation options.

Each stage has a **basic** control section, setting the time (or level, in the case of Sustain) and curve (the smaller knobs), ranging from Logarithmic, to Linear, to Exponential. With only the gate input connected and out+ output connected, Vega behaves as a normal ADSR.

Things are made interesting with the **mod + adv** controls. The controls in order from left to right (or top to bottom, as they're angled) are
* Attenuversion for the modulation input
* The modulation input
* Modulation input mode
* Advance trigger input
* Manual advance trigger

This section offers 4 modes: Ring, Add, Self-Enveloped, and Inverse Ring
* Ring will apply a basic multiplication between the input signal and the respective envelope segment. This has the downside of the 'peak' of the attack causing a very strong modulation, which may transition awkwardly, even with interpolation.
  
  > Note, technically the full math is [(ring_input * attenuverter) * envelope_stage **+ enevelope_stage**] this addition is done so that the ring modulation is still centered around the evelope stage
* Addition will do a basic addition (unity sum) of the input signal and the respective envelope segment. This keeps the level of modulation constant, but may mean that the start of the attack stage and end of the release stage can have gaps or extra bumps.
* Self-Enveloped Addition will cause the input signal to be the same as the ring modulated input for the first 20% of the stage. This gives a gental fade in of the modulation, without making it overtake the signal
* Inverse Ring multiplies the modulation signal with the inverse, offset envelope. This can be used to, for example, make the modulation very strong at the start of the attack stage but 0 at the peak.

There is also a global ring input. This will **only** function in multiplication (basic ring modulation) mode and is done **post** all the other modulation.

The anger knob controls how the modulation inputs are crossfaded between stages, to prevent 'clicks' in the envelope.

Next up, the Advance trigger input and manual trigger button input: Both are to used to force the module to go to the next stage. Note that when going from attack to decay, decay will always start at 10V and that release will start from the level sustain is set to, regardless of if the sustain stage actually happens (if you self patch the sustain gate out to the advance input) 

The module also has per-stage outputs. By default, these will output the current stage's segment **without** modulation. The button will change this though. There's a special mode for the decay stage which outputs the decay stage minus the sustain level, removing the DC offset.
There is also a per-stage gate output, which does what you'd expect.

Because with modulation it is quite easy for the output to get to extreme values, the module hard clips the output to ±12V. If you'd like to avoid some of this clipping, the global ring offset knob (to the left of the anger knob) acts as a global attenuator by default. It is done with the offset knob and not the attenutation knob as this makes sense if you treat the global ring input as just having no signal by default, and a ringmod with DC (offset) acts as a VCA.

For some extra fun, there's a track-and-hold-ish section at the bottom of the module under the anger knob, play with it and see what happens

## Sheliak

<img src="./images/Sheliak.png" alt="Sheliak LFSR" width="220">

> Sheliak is a semidetached binary system made up of a stellar class B6-8 primary star and a secondary that is probably also a B-type star. The fainter, less massive star in the system was once the more massive member of the pair, which caused it to evolve away from the main sequence first and become a giant star. Because the pair are in a close orbit, as this star expanded into a giant it filled its Roche lobe and transferred most of its mass over to its companion.

Sheliak is an analog shift register with some extra tricks. It sholud be fed a x4 (or x8, x12, x16, etc.) clock (and, optionally, reset) and some data, just like any other shift register. The top data input is the main input, the bottom data input will be XOR'd with it so that it can be used for LFSR pseudo-random patterns via self patching. The left 8 outputs are the main outputs and will be shifted every 4th clock pulse. 

The Three outputs going up the left side from the bottom are triplet outputs, reflecting the XOR'd clocke'd in data (just like the main shift register) but this will update every 3rd clock pulse.

### β Lyrae

Beta Lyrae is the same star as Sheliak, just a different name.

This module is an expander. It provides 8 outputs which are derived from the gates produced by Sheliak and the attenuators it provides.

Output operations, from top to bottom:

1. Σ1-8
2. Σ1-4
3. Σ5-8 + perfect fifth (7/12 volt)
4. Σ3-6 + 2 octaves (2 volts)
5. (Σ1-4 % 1V) - 1V (Contrained to the same octave then down 1 octave)
6. Σ1-8 PLUS 1 octave per triplet output that is high MINUS 2 octaves
7. IF the first gate output AND the first triplet gate output are high, update to be the Σ1-4 + Σ1t-3t
8. Σ1t-3t

## ζ Lyrae

<img src="./images/Zeta.png" alt="Zeta Blank" width="660">

> Zeta Lyrae is a binary star in the northern constellation of Lyra.

It's a blank, but it has 'flip' option in the right click menu. Entire panel renders as a light.

## δ1 Lyra

> The star is radiating about 3,620 times the Sun's luminosity from its photosphere at an effective temperature of 20,350 K

DEMO: https://www.youtube.com/watch?v=FvsLZEobSp0

<img src="./images/Delta1.png" alt="alt text" width="90">

This module, frankly, is pretty useless. The top section is 3 cascaded ring-mod's, that is if you label them from top to bottom as A, B, C the operation is Left = Left * -(A * (B * C)) Right = Right * (A * (B * C)) Each stage, A, B, C has it's own attenuator (left) and offset (right). If you would like to bypass this stage (other than inverting the left signal), just set all the offset knobs to their max value. if you do not set the offset high or provide input for each of the 3 stages you will not get any output!

Next, is the clock-selected ring mod section. The upper, middle input is the clock input. The LED on the upper left will show what output is being used- RED for the left input, GREEN for the middle input, and BLUE for the right input.

Finally, in the last section, the two inputs are actually gate inputs and toggle channel cross-ring-mod. That is, if the top input of the bottom section is brought high, the right channel will be multiplied with the left, and if the bottom input is brought high, the left channel multiplied with the right.

It's pretty easy to get a really high output from this module, or to just get awful sounds. So, watch your levels and probably use a clipping VCA like ZZC's used here, and try to keep input content harmonically related. Octaves tend to work well.

## Aladfar

A 24 HP, 16-voice oscillator inspired by the Catalyst Audio / Electroserf [MIG
VCO manual](https://www.catalystaudio.com/_files/ugd/069912_a25dfb0872ba4adcaef2
aede744fbab1.pdf). The plain panel uses the collection's serif labels, hexagonal
knobs and custom jacks, organized into dark control pills and light CV areas.
Its SVG has editable background, control-area and outlined-lettering layers for
adding artwork later.

Two alternating ramp cores produce the **Raw** signal. **Balance** bends one
half from rising ramp through triangle to falling ramp; **Offset** shifts the
two halves relative to one another. **Resonance** returns the loaded signal to
the cores' grid paths. The I/II switch selects common-output or cross-coupled
feedback. Grid-charge recovery, shared screen loading and a compact 1J24B
current model change charging speed and waveform curvature. Automatic pitch
compensation trims the charging scale to retain the requested tuning as
resonance changes. The upper quarter of Resonance now enters stronger
regenerative feedback with cutoff/saturation and RC recovery bursts, for a
brighter, screechier tone in both modes. Compensation settles over several free-
running cycles and respects pitch/FM and external sync. Resonance is not a
resonant filter applied to a finished oscillator.

- **Tune** sets frequency; **Fine** spans +/-1 semitone. Pitch tracks 1 V/oct.
  Linear FM adds C4 Hz per volt at full amount. Negative total frequency clamps
  at 0.01 Hz; this is not a through-zero FM oscillator.
- **PWM** is the Raw comparator threshold and affects only **Pulse**. Offset and
  Balance modulation also make complex pulse shapes.
- **Smooth / Fold** drives the third modeled pentode stage. Zero mutes both
  processed outputs; higher drive moves through amplification into asymmetric
  saturation. **Shape** changes bias and tilts high/low emphasis.
- **Smooth** is the saturated signal. **Fold** passes it through four smooth
  reflection stages with antiderivative antialiasing.
- **Sync level** attenuates the sync input. Weak positive/negative crossings
  address the corresponding core; stronger signals can take over either half.
  Zero disables sync. Bipolar signals can synchronize both halves.
- Offset, Balance, PWM, Resonance, Shape and Drive have CV attenuverters;
  10 V at full amount spans one normalized unit. Drive's attenuverter defaults
  to +100%; the others default to zero. FM has a separate unipolar amount.

All nine inputs support polyphony. The widest input sets the width of all four
outputs; mono cables broadcast and missing lanes in shorter polyphonic cables
are zero. Four voices share Rack SIMD arithmetic. No audio-thread allocation is
needed. Core BLEP/BLAMP, sync/comparator minBLEP, 8x nonlinear processing (16x
folding) and 256-tap output FIRs suppress aliasing; extreme modulation can still
alias.

The 1J24B current approximation is checked against published curves. The full
hardware schematic is unavailable: feedback routing and external component
values are estimates, not a circuit-exact or sound-matched clone. See [the model
notes](docs/Aladfar-model.md) for sources, equations and assumptions. A neutral
starting point is Tune=C4, Balance=0%, Offset=50%, Resonance=0%,
Smooth/Fold=35%, Shape=0. At minimum Balance and minimum Offset the two
identical ramps emphasize twice the pair's cycle frequency, as in the reference
diagrams.
