# Aladfar model and validation

Aladfar follows the functions and waveform diagrams of the Catalyst Audio /
Electroserf MIG VCO. It is an independent software interpretation, not an
endorsed reproduction or a claim to simulate the undisclosed hardware netlist.
The panel drawing helps identify signal flow; its amplifier symbols and lines
do not supply component values or unambiguous grid connections.

## Sources

- [MIG VCO manual, v1.0](https://www.catalystaudio.com/_files/ugd/069912_a25dfb0872ba4adcaef2aede744fbab1.pdf): alternating cores, offset/balance diagrams, two feedback modes,
  level-dependent sync, comparator, saturation/bias and four-stage folding.
- [Manufacturer](https://www.catalystaudio.com/migvco): identifies the three tubes
  as 1J24B / 1Zh24B / 1Ж24Б rod pentodes.
- [Scanned 1Ж24Б data and plate curves](https://patric-sokoll.de/Roehrenmuseum/Datenbank/PDF/Datenblatt%201%D0%9624%D0%91.pdf): anode curves at screen 45 V, grid -1/0/+1 V.
- [Scanned electrical specifications](https://www.kn34pc.com/galery/radio_tubes/tube_1j24b/1j24b.pdf): typical plate current 0.95 mA (+/-0.45), transconductance
  0.9 mA/V (+/-0.3), screen current <=0.1 mA at the stated nominal conditions,
  cathode limit 1.6 mA; input/output/reverse capacitances 3.6/2.95/<=0.008 pF.
- [Koren's tube-model discussion](https://www.normankoren.com/Audio/Tubemodspice_article.html): screen-controlled 3/2 current laws and plate knees.
  The implementation is a compact approximation, not Koren's full model.

## Alternating cores

The first core's local ramp reverses direction at the Balance breakpoint. The
second remains a rising ramp. Relative offset includes the balance-dependent
baseline shift necessary to reproduce the manual's six examples, including the
continuous V at minimum offset / maximum balance. At minimum balance, minimum
offset produces two ramps per full core cycle; middle offset joins them into
one full saw. Thus some settings emphasize the octave even though the pair's
complete cycle still follows the pitch input.

The integrated phase represents the active timing-capacitor charge and which
core is active. With zero resonance the calibration is idealized and linear.
With resonance it advances according to the modeled tube current. The two
halves mutually hand over when their normalized charge reaches its threshold;
the inactive plate recovers with an estimated 25 us discharge time. There is
no auxiliary state-variable resonator, fixed resonance multiple of pitch,
noise injection or random pitch drift.

## 1J24B current approximation

Voltages are in volts and currents in milliamperes:

```
e = max(Vg1 + Vg2 / (45/1.65), 0)
Iemission = (0.88 / 1.65^(3/2)) * e^(3/2)
Ia = Iemission * Va / (Va + 4)
```

The approximation is smoothly limited above 1.2 mA toward 1.6 mA. This is a
numerical guard near the rated cathode current, not a claim that a maximum
rating determines the physical emission ceiling. The rational plate knee
is fitted by inspection to the scan, not to a precision digitization. It gives
these approximate checkpoints at G2=45 V, G1=0 V:

| Plate voltage | Scanned curve, approximately | Model |
|---|---|---|
| 5 V | 0.49 mA | 0.489 mA |
| 10 V | 0.63 mA | 0.629 mA |
| 20 V | 0.73 mA | 0.733 mA |
| 40 V | 0.80 mA | 0.800 mA |
| 60 V | 0.82 mA | 0.825 mA |
| 80 V | 0.84 mA | 0.838 mA |

At 60 V it gives approximately 0.204 mA at G1=-1 V and 1.47 mA at G1=+1 V.
Transconductance at zero grid is about 0.75 mA/V, within the nominal range.
The scan's averaged curve and the nominal 0.95 mA entry are not identical.
Screen current includes a low baseline and a fraction of the current diverted
as plate voltage falls; extra positive-grid loading is approximate. It is not
fitted to a complete family of screen-current or grid-current measurements.
Extrapolation down to the estimated low operating voltages is consequently less
certain than the published 45 V screen conditions.

## Feedback and memory

Mode I returns the combined loaded output with inverted grid injection, making
its inverting plate stage regenerative. Mode II cross-couples the opposite
plate signals and includes a positive return through the paired gain path. These are plausible interpretations,
not verified switch wiring. Each path has a coupling-capacitor/grid-leak state;
positive grid conduction shifts its stored charge. A common screen supply droops
under screen current and recovers, so a reset affects the following part of the
waveform rather than just adding a static distortion curve.

A midpoint integration step updates the RC states and charging current. A bounded load-line iteration accounts for the plate load inside the feedback
path and starts on the preceding conduction branch. This retains regenerative
hysteresis rather than reinitializing the stage at every sample.
The programmed DC charging source is supplemented by a bounded portion of the
plate-current variation, so the regenerative plate stage can enter cutoff or
saturation without latching the timing capacitor. Grid feedback remains gentle
at low resonance and rises more steeply in the upper quarter. Strong feedback
can produce repeated fast conduction transitions and RC recovery bursts, giving
a much brighter, screechier signal. It is generated in the returned grid/plate
path, not by adding an unrelated audio resonator.

Current is bounded to preserve numerical stability and reliable recovery; this
is not a general SPICE solver. High resonance can curve ramps, emphasize reset
transients and initially change the cycle period. An automatic tuning loop
compensates the latter: it integrates requested pitch/FM over each complete
free-running cycle and trims the charging-current scale until the actual cycle
advance agrees with the reference. An accumulated phase-error term also removes
long-term tuning bias when the regenerative stage has unequal successive cycles. Within-cycle current variation remains, so
the waveform still changes. Forced sync resets the estimator rather than being
interpreted as tuning error. At zero resonance the calibrated scale is unity.
Compensation converges over several cycles after a sudden parameter change;
it is not an instantaneous resynthesis of a different ideal waveform.

Unverified circuit assumptions, kept explicit in code:

| Quantity | Estimate |
|---|---|
| Core plate swing | 5–12 V |
| Screen supply | 10 V |
| Screen feed resistance / recovery | 22 kohm / 3 ms |
| Grid feedback depth | 0.28 R + 1.8 R^4 V per normalized output unit |
| Plate-return load | 0.22 + 0.18 R^2 normalized units |
| Cross-path regenerative return | 0.8 of the plate-return load |
| Coupling/grid-leak time constant | 220 us |
| Reset recovery | 25 us |
| Third-stage B+ / screen / load | 12 V / 9 V / 82 kohm |
| Third-stage input resistance | 220 kohm |
| Shape shelf | 1 kHz, estimated external tone network |

The known tube capacitances constrain the third-stage input pole. The tone shelf
is separate; it is not misrepresented as a large intrinsic pentode Miller pole.
The third-stage plate voltage is the positive quadratic solution of its load
line, including the modeled plate-current knee. Shape adjusts grid bias as well
as the high/low emphasis described in the manual. At zero drive Smooth and Fold
are silent. Four smooth transistor-like reflection stages form the folder;
those stages are behavioral, not a fitted discrete-transistor circuit.

A hardware schematic, operating voltages, and oscilloscope/audio captures across
resonance/offset/balance settings would allow these assumptions to be replaced
and the model validated against the actual instrument. Published tube type
alone cannot establish the behavior of the complete oscillator.

## Antialiasing and limits

Core steps and slope corners use host-bandwidth BLEP/BLAMP corrections, evaluated
at the internal rate. Sync discontinuities and comparator transitions use Rack
minBLEP corrections. Nonlinear feedback, saturation and folding run at 8x;
four folding stages run at 16x and additionally use first-order antiderivative
antialiasing, with a separate inner decimator.
A 256-tap FIR decimates each output. CV is interpolated between host samples.
All these states are independent in each of the four SIMD lanes.

This suppresses aliasing rather than promising zero aliases under arbitrary
hard sync, extreme audio-rate parameter changes, or strong nonlinear feedback.
Pitch plus linear FM is limited to 0.01 Hz–0.4 sample rate; feedback-dependent
charging speed is also bounded below Nyquist. Raw/Smooth/Fold have a 5 Hz coupling highpass and soft output-buffer headroom
compression at the oversampled rate (knee +/-8 V, asymptote +/-9.5 V). This
leaves room for FIR overshoot rather than creating the resonant texture with
host-rate hard clipping. Outputs retain a final +/-12 V emergency safety bound. Pulse is bipolar
+/-5 V before the FIR and may overshoot slightly around edges.

## Panel source

`res/Aladfar.svg` contains separate Background, Control areas, and Lettering
layers. Its background is plain; future artistic patterns can be placed there.
Serif glyphs are outlined, so no unavailable system font can hide the controls.
`res/generate_aladfar_panel.py` reproduces the layout with fonttools and a local
DejaVu Serif TTF; fonttools is a development tool, not a plugin dependency.
