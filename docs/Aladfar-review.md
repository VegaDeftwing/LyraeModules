# Aladfar review notes

These existing edge cases were reproduced during the documentation/formatting
review of checkpoint `8f9491f`. This pass intentionally does not fix them or
change the oscillator's sound. Both should be addressed as separate behavioral
changes with regressions.

## Very slow phase wraps can be missed by pitch compensation

Location: `Bank::process()` in `src/dsp/Aladfar.hpp`, low-frequency phase
accumulation and tuning-loop wrap detection.

The oscillator uses `lowPhase` doubles when the increment is too small for its
float phase view. However, the tuning loop detects a wrap from
`double(oldPhase[lane]) + actual >= 1`, using the old float value rather than
the double accumulator. The float view is capped at `0.99999994f`. When an
increment is below the remaining gap to 1, the double can wrap while this
condition stays false. The integrals are then not reset and the tuning gain is
not updated for that cycle.

A focused probe at 0.01 Hz / 192 kHz, with 80% resonance, seeded `lowPhase`
just below 1 and its float view at the cap. After one host frame:

- The actual phase wrapped to approximately `7.09e-8`.
- `fullCycle` remained false and `observedCycles` remained approximately 1.
- The completed cycle should instead have reset the integrals and marked a
  subsequent cycle as complete.

This affects very slow/LFO operation; the existing audio-rate tuning tests pass.
A future fix should derive wrap detection from the phase path that actually
advanced, including the double accumulator, and check both low-rate and normal
phase bookkeeping.

## Reconnecting Pulse resumes pending corrections from before disconnection

Location: `Bank::process()` in `src/dsp/Aladfar.hpp`, `outputMask & 2` block and
output decimation.

Output masking pauses the Pulse comparator, BLEP generator, and FIR while Raw
can continue advancing the oscillator. If Pulse is disconnected with a pending
edge correction, reconnecting it later resumes that old correction at the new
phase. The paused FIR also retains its earlier samples. This can add a short
reconnection transient; it does not affect Raw during normal operation.

A 440 Hz probe advanced Raw alone for 10,000 frames after a Pulse transition.
The BLEP position remained 8, and a pending correction of approximately
`-0.044576` was unchanged. On reconnection, isolating just this retained BLEP
tail produced approximately 33 mV peak difference at the Pulse output over
128 samples. This is a small artifact in that fixture, not evidence of a
persistent fault or of clipping.

The output-skipping optimization is intentional. A future fix needs an explicit
reconnection policy: keep relevant histories current, or reset/prime the
reconnected path. It should preserve Raw and active-voice independence and test
connection transitions separately from steady-state output comparisons.

## Cosmetic-pass verification

The cleanup was compared with checkpoint `8f9491f` using Clang's raw lexer:
all 11,093 non-comment/non-whitespace C++ tokens in the module, UI, DSP headers
and oscillator harness were unchanged. The panel generator's Python AST was
also unchanged. The rebuilt `Aladfar.cpp.o` matched the checkpoint object
byte-for-byte after removing debug information. Full collection and oscillator
tests passed; neither issue above was silently fixed or excluded from existing
coverage by changing tolerances.
