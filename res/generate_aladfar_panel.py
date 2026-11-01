#!/usr/bin/env python3
"""Regenerate the plain Aladfar panel; development dependency: fonttools.

Usage: python res/generate_aladfar_panel.py [path/to/DejaVuSerif.ttf]
Labels are glyph outlines because Rack's NanoSVG does not render SVG text.
Named Inkscape layers keep the background, control areas, and lettering editable.
The generated SVG is the only runtime asset; the font is not redistributed.
"""

from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.ttLib import TTFont

SVG = "http://www.w3.org/2000/svg"
INK = "http://www.inkscape.org/namespaces/inkscape"
ET.register_namespace("", SVG)
ET.register_namespace("inkscape", INK)
font = TTFont(sys.argv[1] if len(sys.argv) > 1 else "/usr/share/fonts/TTF/DejaVuSerif.ttf")
glyphs = font.getGlyphSet()
cmap = font.getBestCmap()
units = font["head"].unitsPerEm
DARK, LIGHT, BASE, INK_COLOR = "#191919", "#f3f2ef", "#c9c7c3", "#292827"


def element(parent, tag, **attributes):
    return ET.SubElement(parent, f"{{{SVG}}}{tag}", {k: str(v) for k, v in attributes.items()})


root = ET.Element(f"{{{SVG}}}svg", width="360", height="380", viewBox="0 0 360 380")
element(root, "title").text = "Aladfar — core controls layout"
element(root, "desc").text = "24 HP. Plain background, control groups, CV areas, and outlined serif labels."


def layer(name):
    layer_id = re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")
    return element(root, "g", id=layer_id, **{f"{{{INK}}}groupmode": "layer", f"{{{INK}}}label": name})


base = layer("Background")
element(base, "rect", x=0, y=0, width=360, height=380, fill=BASE)
element(base, "rect", x=1, y=1, width=358, height=378, fill="none", stroke="#8d8b88", **{"stroke-width": 1})
areas = layer("Control areas")
labels = layer("Lettering — outlined DejaVu Serif")


def rect(parent, name, x, y, w, h, fill, radius=15):
    return element(parent, "rect", id=name, x=x, y=y, width=w, height=h, rx=radius, fill=fill)


def text(name, x, baseline, value, size=12, color=LIGHT):
    scale = size / units
    advance = sum(glyphs[cmap[ord(c)]].width for c in value)
    cursor = x - advance * scale / 2
    pen = SVGPathPen(glyphs, ntos=lambda n: f"{n:.3f}".rstrip("0").rstrip("."))
    for c in value:
        glyph = glyphs[cmap[ord(c)]]
        glyph.draw(TransformPen(pen, (scale, 0, 0, -scale, cursor, baseline)))
        cursor += glyph.width * scale
    group = element(labels, "g", id=name, **{f"{{{INK}}}label": value})
    element(group, "title").text = value
    element(group, "path", d=pen.getCommands(), fill=color)


def divider(name, y, label):
    text(name, 180, y, label, 8.2, INK_COLOR)
    for start, end in [(12, 137), (223, 348)]:
        element(areas, "path", d=f"M{start} {y - 3}H{end}", fill="none", stroke="#888682", **{"stroke-width": .6})


# Tuning, tracking, and two external modulation inputs.
rect(areas, "Tuning", 12, 17, 101, 66, DARK)
rect(areas, "Pitch-input", 119, 17, 49, 66, LIGHT)
rect(areas, "Linear-FM", 178, 17, 80, 66, LIGHT)
rect(areas, "Sync", 268, 17, 80, 66, LIGHT)
text("Tune-label", 43, 34, "Tune", 13)
text("Fine-label", 89, 34, "Fine", 11)
text("Pitch-label", 143, 34, "1 V/oct", 9.5, INK_COLOR)
text("FM-label", 218, 34, "Lin FM", 12, INK_COLOR)
text("Sync-label", 308, 34, "Sync level", 11.5, INK_COLOR)

divider("Core-section", 101, "CORE")
divider("Shaping-section", 212, "SHAPING")


def control(name, center, top, label, size=13):
    rect(areas, f"{name}-control", center - 53, top, 106, 49, DARK)
    rect(areas, f"{name}-CV", center - 53, top + 51, 106, 40, LIGHT)
    text(f"{name}-label", center, top + 15, label, size)
    text(f"{name}-CV-label", center - 20, top + 62, "CV", 8, INK_COLOR)
    text(f"{name}-amount-label", center + 20, top + 62, "− / +", 8, INK_COLOR)


control("Offset", 65, 109, "Offset")
control("Balance", 180, 109, "Balance")
control("Resonance", 295, 109, "Resonance", 12)
# CKSS is vertical: the upper position is mode II, the lower is mode I.
text("Mode-II-label", 336, 140, "II", 7)
text("Mode-I-label", 336, 152, "I", 7)
control("Smooth-fold", 65, 220, "Smooth / Fold", 11.5)
control("Shape", 180, 220, "Shape")
control("PWM", 295, 220, "PWM")

rect(areas, "Outputs", 12, 320, 336, 41, DARK, 17)
for x, label in [(55, "Raw"), (138, "Pulse"), (222, "Smooth"), (305, "Fold")]:
    text(f"{label}-output-label", x, 332, label, 10.5)
text("Module-name", 180, 376, "Aladfar", 17, INK_COLOR)

ET.indent(root, space="  ")
Path(__file__).with_name("Aladfar.svg").write_text(ET.tostring(root, encoding="unicode") + "\n")
