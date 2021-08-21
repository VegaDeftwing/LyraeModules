# Lyrae Modules

## Sulafat
![](./images/Sulafat.png)

Sulafat is a multi-mode wavefolder (and a bit more) with some internal modulation that diffeers between the two channels, with internal normalling of the left (top) input to the right (bottom) input


0. Bypass
1. Basic Wavefolder
2. 5-level Quantized Wavefolder
3. Tangent + Clamping (sorta still a wavefolder)
4. Split mix of 1 + low-passed 2. Split is inverted on second channel
5. Ring mod - uses internal LFOs if only one input
6. S&H-ish creates some artifacting depending on the input
7. Wavefolding + S&H + LFO mess. This one you just sort of need to hook up to a scope

There's also an easter egg LFO mode, if the top (left) input is left disconected the two outputs output the internal LFO's.
The left (top) output will be a basic LFO, while the bottow (right) output will be the two LFO's ringmodd'd together.

## γ Lyrae
![](./images/Gamma.png)

Gamma Lyrae is a companion module to Sulafat. With nothing connected, the middle knob will generate two DC voltage, with one being the inverse of the other. The knob can be turned both ways to swap which side is going negative. If a CV input is provided, the knob becomes an attenuverter, but the operation is still effectively the same.

The main inputs (at the bottom) are normalled. These inputs are multiplied (ring modulated) with the center input. This creates two output signals with opposite DC offsets. This will strongly impact the signal fed into the wavofolding modes of Sulafat.

## Vega

## Sheliak

## ζ Lyrae

## δ Lyrae
