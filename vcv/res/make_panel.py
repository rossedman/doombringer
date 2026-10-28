#!/usr/bin/env python3
"""Generates the DOOMBRINGER panel SVG and a matching C++ layout header from one table.

    python3 res/make_panel.py   # writes res/Doombringer.svg, res/panel-preview.svg and src/PanelLayout.hpp

Every control is marked with a sigil instead of a word; the README has the
decoder. Rack's SVG renderer ignores <text>, so the only lettering (the title)
is converted to outlines with matplotlib's TextPath. Dimensions are real
Eurorack (3U = 128.5 mm, 1 HP = 5.08 mm), so this is also a starting point for
a physical front panel. The look is shared with Death's Hand
(github.com/rossedman/deaths-hand), whose panel this one sits beside; the mark
is Portal Electronics', and the same function draws the docs' logo and favicon.
"""

import math
import random
from pathlib import Path

from matplotlib.font_manager import FontProperties
from matplotlib.path import Path as MPath
from matplotlib.textpath import TextPath

HP = 16
W, H = HP * 5.08, 128.5
COLS = [11.5, 30.8, 50.4, 69.8]

BG = "#0b090a"
BONE = "#d9cdb8"
BONE_DIM = "#6b6156"
BLOOD = "#7a0b10"
BLOOD_HI = "#c4141c"
BLOOD_DARK = "#2a0608"
CIRCLE = "#2b0b0e"  # the faint summoning circle behind the controls

# (kind, enum name, x, y, sigil)
#   kind: knob_l, knob_m, switch, button, light, in, out
# The layout mirrors the patch.Init: the cutoff and the three character macros
# on top, the shift-layer settings below, then the jacks. Sigils shared with
# Death's Hand mean the same thing on both (spiral = FM, slit eye = fine,
# pillars = restart, solid drop = the main output).
CONTROLS = [
    ("knob_l", "CUTOFF_PARAM", COLS[0], 27, "eclipse"),
    ("knob_l", "WRATH_PARAM", COLS[1], 27, "flame"),
    ("knob_l", "RUIN_PARAM", COLS[2], 27, "cracked"),
    ("knob_l", "DOOM_PARAM", COLS[3], 27, "chevrons"),
    ("knob_m", "MORPH_PARAM", COLS[0], 50, "serpent"),
    ("knob_m", "FM_PARAM", COLS[1], 50, "spiral"),
    ("knob_m", "HUNGER_PARAM", COLS[2], 50, "maw"),
    ("knob_m", "FINE_PARAM", COLS[3], 50, "slit_eye"),
    ("switch", "BASS_COMP_PARAM", COLS[0], 68, "trident"),
    ("light", "ENV_LIGHT", (COLS[1] + COLS[2]) / 2, 68, "maw"),
    ("button", "PING_PARAM", COLS[3], 68, "bolt"),
    ("in", "CUTOFF_INPUT", COLS[0], 89, "eclipse"),
    ("in", "WRATH_INPUT", COLS[1], 89, "flame"),
    ("in", "RUIN_INPUT", COLS[2], 89, "cracked"),
    ("in", "DOOM_INPUT", COLS[3], 89, "chevrons"),
    ("in", "AUDIO_INPUT", COLS[0], 103, "chalice"),
    ("in", "FM_INPUT", COLS[1], 103, "spiral"),
    ("in", "PING_INPUT", COLS[2], 103, "bolt"),
    ("in", "RESET_INPUT", COLS[3], 103, "pillars"),
    ("out", "ENV_OUTPUT", COLS[1], 117, "maw"),
    ("out", "LP4_OUTPUT", COLS[2], 117, "four_drop"),
    ("out", "MAIN_OUTPUT", COLS[3], 117, "drop"),
]

# Where each control's sigil sits (offset above the control) and how big it is.
SIGIL_DY = {"knob_l": -9.4, "knob_m": -7.6, "switch": -6.8, "button": -6.6, "light": -4.4, "in": -7.0, "out": -7.0}
SIGIL_R = {"knob_l": 1.9, "knob_m": 1.7, "switch": 1.6, "button": 1.5, "light": 1.2, "in": 1.5, "out": 1.6}
CONTROL_R = {"knob_l": 6.3, "knob_m": 4.8, "switch": 3.8, "button": 3.2, "light": 1.6, "in": 4.0, "out": 4.0}

SW = 0.3  # sigil stroke width (mm)


# ----------------------------------------------------------------- primitives
def f(v):
    return f"{v:.3f}"


def stroke(d, color=BONE, w=SW, fill="none", cap="round"):
    return f'<path d="{d}" fill="{fill}" stroke="{color}" stroke-width="{w}" stroke-linecap="{cap}" stroke-linejoin="round"/>'


def circle(cx, cy, r, color=BONE, w=SW, fill="none"):
    return f'<circle cx="{f(cx)}" cy="{f(cy)}" r="{f(r)}" fill="{fill}" stroke="{color}" stroke-width="{w}"/>'


def dot(cx, cy, r, color=BONE):
    return f'<circle cx="{f(cx)}" cy="{f(cy)}" r="{f(r)}" fill="{color}"/>'


def poly(points, closed=False, **kw):
    d = "M" + " L".join(f"{f(x)},{f(y)}" for x, y in points) + (" Z" if closed else "")
    return stroke(d, **kw)


def polar(cx, cy, r, deg):
    a = math.radians(deg)
    return cx + r * math.sin(a), cy - r * math.cos(a)  # 0 deg = up, clockwise


# ----------------------------------------------------------------- sigils
# Each sigil is drawn inside a circle of radius r centred on (cx, cy).


def s_slit_eye(cx, cy, r, color=BONE):
    d = f"M{f(cx - r)},{f(cy)} Q{f(cx)},{f(cy - r * 1.05)} {f(cx + r)},{f(cy)} Q{f(cx)},{f(cy + r * 1.05)} {f(cx - r)},{f(cy)} Z"
    slit = f"M{f(cx)},{f(cy - r * 0.42)} Q{f(cx + r * 0.16)},{f(cy)} {f(cx)},{f(cy + r * 0.42)} Q{f(cx - r * 0.16)},{f(cy)} {f(cx)},{f(cy - r * 0.42)} Z"
    return [stroke(d, color), stroke(slit, color, w=0.01, fill=color)]


def s_trident(cx, cy, r, color=BONE):
    out = [poly([(cx, cy + r), (cx, cy - r)], color=color)]
    out.append(
        stroke(
            f"M{f(cx - r * 0.7)},{f(cy - r * 0.9)} Q{f(cx - r * 0.7)},{f(cy - r * 0.15)} {f(cx)},{f(cy - r * 0.15)} "
            f"Q{f(cx + r * 0.7)},{f(cy - r * 0.15)} {f(cx + r * 0.7)},{f(cy - r * 0.9)}",
            color,
        )
    )
    for x in (cx - r * 0.7, cx, cx + r * 0.7):  # barbed tips
        out.append(poly([(x - r * 0.18, cy - r * 0.72), (x, cy - r * 1.0), (x + r * 0.18, cy - r * 0.72)], color=color, w=SW * 0.9))
    out.append(poly([(cx - r * 0.3, cy + r * 0.55), (cx + r * 0.3, cy + r * 0.55)], color=color, w=SW * 0.9))
    return out


def s_serpent(cx, cy, r, color=BONE):
    d = (
        f"M{f(cx - r * 0.9)},{f(cy + r * 0.75)} C{f(cx - r * 0.2)},{f(cy + r * 1.1)} {f(cx + r * 0.9)},{f(cy + r * 0.4)} {f(cx)},{f(cy)} "
        f"S{f(cx - r * 0.6)},{f(cy - r * 0.9)} {f(cx + r * 0.65)},{f(cy - r * 0.75)}"
    )
    return [
        stroke(d, color),
        dot(cx + r * 0.72, cy - r * 0.78, r * 0.17, color),
        poly([(cx + r * 0.86, cy - r * 0.8), (cx + r * 1.1, cy - r * 0.95)], color=color, w=SW * 0.7),
    ]


def s_spiral(cx, cy, r, color=BONE):
    pts = []
    for i in range(70):
        t = i / 69
        a = t * 3.1 * 360
        pts.append(polar(cx, cy, r * (0.1 + 0.9 * t), a))
    return [poly(pts, color=color)]


def s_pillars(cx, cy, r, color=BONE):
    return [
        poly([(cx - r * 0.75, cy - r), (cx - r * 0.75, cy + r)], color=color),
        poly([(cx + r * 0.75, cy - r), (cx + r * 0.75, cy + r)], color=color),
        poly([(cx + r * 0.3, cy - r * 0.85), (cx - r * 0.25, cy + r * 0.05), (cx + r * 0.25, cy - r * 0.05), (cx - r * 0.3, cy + r * 0.85)], color=color),
    ]


def drop_path(cx, cy, r):
    return (
        f"M{f(cx)},{f(cy - r * 1.1)} C{f(cx + r * 0.25)},{f(cy - r * 0.5)} {f(cx + r * 0.85)},{f(cy)} {f(cx + r * 0.7)},{f(cy + r * 0.45)} "
        f"A{f(r * 0.72)},{f(r * 0.72)} 0 1 1 {f(cx - r * 0.7)},{f(cy + r * 0.45)} "
        f"C{f(cx - r * 0.85)},{f(cy)} {f(cx - r * 0.25)},{f(cy - r * 0.5)} {f(cx)},{f(cy - r * 1.1)} Z"
    )


def s_drop(cx, cy, r, color=BLOOD_HI):
    return [stroke(drop_path(cx, cy, r), color=color, w=0.2, fill=color)]


def s_eclipse(cx, cy, r, color=BONE):
    """The cutoff: a light with a crescent of shadow eating into it."""
    rr = r * 0.9
    d = f"M{f(cx)},{f(cy - rr)} A{f(rr)},{f(rr)} 0 0 0 {f(cx)},{f(cy + rr)} A{f(rr * 1.3)},{f(rr * 1.3)} 0 0 1 {f(cx)},{f(cy - rr)} Z"
    return [circle(cx, cy, rr, color), stroke(d, color, w=0.05, fill=color)]


def s_flame(cx, cy, r, color=BONE):
    outer = (
        f"M{f(cx)},{f(cy - r)} C{f(cx + r * 0.15)},{f(cy - r * 0.45)} {f(cx + r * 0.85)},{f(cy - r * 0.2)} {f(cx + r * 0.7)},{f(cy + r * 0.4)} "
        f"A{f(r * 0.72)},{f(r * 0.72)} 0 0 1 {f(cx - r * 0.7)},{f(cy + r * 0.4)} "
        f"C{f(cx - r * 0.8)},{f(cy)} {f(cx - r * 0.4)},{f(cy - r * 0.2)} {f(cx - r * 0.25)},{f(cy - r * 0.55)} "
        f"C{f(cx - r * 0.1)},{f(cy - r * 0.3)} {f(cx)},{f(cy - r * 0.6)} {f(cx)},{f(cy - r)} Z"
    )
    inner = (
        f"M{f(cx)},{f(cy - r * 0.1)} C{f(cx + r * 0.35)},{f(cy + r * 0.2)} {f(cx + r * 0.3)},{f(cy + r * 0.6)} {f(cx)},{f(cy + r * 0.65)} "
        f"C{f(cx - r * 0.3)},{f(cy + r * 0.6)} {f(cx - r * 0.3)},{f(cy + r * 0.25)} {f(cx)},{f(cy - r * 0.1)} Z"
    )
    return [stroke(outer, color), stroke(inner, color, w=0.05, fill=color)]


def s_cracked(cx, cy, r, color=BONE):
    """RUIN: a broken ring with a crack through it."""
    rr = r * 0.88

    def arc(a0, a1):
        x0, y0 = polar(cx, cy, rr, a0)
        x1, y1 = polar(cx, cy, rr, a1)
        return stroke(f"M{f(x0)},{f(y0)} A{f(rr)},{f(rr)} 0 {1 if a1 - a0 > 180 else 0} 1 {f(x1)},{f(y1)}", color)

    crack = [polar(cx, cy, rr * 1.12, 30)]
    for i, t in enumerate((0.75, 0.5, 0.25, 0.0, -0.25, -0.5, -0.75)):
        x, y = polar(cx, cy, rr * t, 30)
        dx, dy = polar(0, 0, rr * 0.16 * (1 if i % 2 else -1), 120)
        crack.append((x + dx, y + dy))
    crack.append(polar(cx, cy, rr * 1.12, 210))
    return [arc(50, 190), arc(230, 370), poly(crack, color=color, w=SW * 0.9)]


def s_chevrons(cx, cy, r, color=BONE):
    """DOOM: three chevrons falling, each smaller: an octave down, and another."""
    out = []
    for y, w in ((-0.8, 1.0), (-0.1, 0.72), (0.5, 0.46)):
        yy = cy + y * r
        out.append(poly([(cx - w * r, yy - 0.3 * w * r), (cx, yy + 0.3 * w * r), (cx + w * r, yy - 0.3 * w * r)], color=color))
    out.append(dot(cx, cy + r * 1.0, r * 0.11, color))
    return out


def s_maw(cx, cy, r, color=BONE):
    """HUNGER: open jaws with teeth. On the envelope output it's the same mouth, in red."""
    out = []
    for side in (-1, 1):  # -1 upper jaw (teeth down), 1 lower jaw (teeth up)
        base = cy + side * r * 0.14
        pts = [(cx + r, base)]
        for i in range(5):
            x0 = cx + r * (0.8 - i * 0.4)
            pts += [(x0 - r * 0.2, base - side * r * 0.32), (x0 - r * 0.4, base)]
        d = f"M{f(cx - r)},{f(base)} Q{f(cx)},{f(base + side * r * 1.15)} {f(cx + r)},{f(base)}"
        out.append(stroke(d, color))
        out.append(poly(pts, color=color, w=SW * 0.8))
    return out


def s_chalice(cx, cy, r, color=BONE):
    """The audio input: a cup to pour the sound into."""
    cup = (
        f"M{f(cx - r * 0.75)},{f(cy - r * 0.75)} Q{f(cx - r * 0.7)},{f(cy + r * 0.1)} {f(cx)},{f(cy + r * 0.12)} "
        f"Q{f(cx + r * 0.7)},{f(cy + r * 0.1)} {f(cx + r * 0.75)},{f(cy - r * 0.75)}"
    )
    return [
        stroke(cup, color),
        poly([(cx - r * 0.75, cy - r * 0.75), (cx + r * 0.75, cy - r * 0.75)], color=color),
        poly([(cx, cy + r * 0.12), (cx, cy + r * 0.8)], color=color),
        poly([(cx - r * 0.45, cy + r * 0.9), (cx + r * 0.45, cy + r * 0.9)], color=color),
    ]


def s_bolt(cx, cy, r, color=BONE):
    """PING: a strike."""
    pts = [(0.3, -1), (-0.45, 0.12), (0.02, 0.12), (-0.3, 1), (0.45, -0.12), (-0.02, -0.12)]
    pts = [(cx + r * x, cy + r * y) for x, y in pts]
    return [poly(pts, closed=True, color=color, w=0.1, fill=color)]


def s_four_drop(cx, cy, r, color=BLOOD_HI):
    """The four-pole output: a drop holding four poles."""
    out = [stroke(drop_path(cx, cy, r), color=color, w=SW)]
    for x in (-0.36, -0.12, 0.12, 0.36):
        out.append(dot(cx + x * r, cy + r * 0.42, r * 0.09, color))
    return out


SIGILS = {name[2:]: fn for name, fn in globals().items() if name.startswith("s_")}


# ----------------------------------------------------------------- decoration
FONT = FontProperties(family="DejaVu Serif", weight="bold")


def text_path(s, x, y, size, color, tracking=0.0):
    """Text as filled outlines, centred on x, baseline y, with letter spacing (mm)."""
    glyphs, cursor = [], 0.0
    bar = TextPath((0, 0), "|", size=size, prop=FONT).get_extents().width
    for ch in s:
        if ch == " ":
            cursor += size * 0.35 + tracking
            continue
        glyphs.append((TextPath((0, 0), ch, size=size, prop=FONT), cursor))
        adv = TextPath((0, 0), ch + "|", size=size, prop=FONT).get_extents().width - bar
        cursor += adv + tracking
    total = cursor - tracking
    out = []
    for tp, off in glyphs:
        d, i, v, c = [], 0, tp.vertices, tp.codes
        if c is None or len(c) == 0:
            continue

        def pt(k, off=off, v=v):
            return f"{f(x - total / 2 + off + v[k][0])},{f(y - v[k][1])}"

        while i < len(c):
            if c[i] == MPath.MOVETO:
                d.append("M" + pt(i))
                i += 1
            elif c[i] == MPath.LINETO:
                d.append("L" + pt(i))
                i += 1
            elif c[i] == MPath.CURVE3:
                d.append("Q" + pt(i) + " " + pt(i + 1))
                i += 2
            elif c[i] == MPath.CURVE4:
                d.append("C" + pt(i) + " " + pt(i + 1) + " " + pt(i + 2))
                i += 3
            elif c[i] == MPath.CLOSEPOLY:
                d.append("Z")
                i += 1
            else:
                i += 1
        out.append(f'<path d="{"".join(d)}" fill="{color}"/>')
    return out, total


def rune(cx, cy, rng, color=BLOOD, w=0.22):
    """An invented rune: 2-4 strokes between points of a 3x4 grid."""
    gx = [cx - 0.7, cx, cx + 0.7]
    gy = [cy - 1.1, cy - 0.37, cy + 0.37, cy + 1.1]
    out = [poly([(cx, cy - 1.1), (cx, cy + 1.1)], color=color, w=w)]  # stave
    for _ in range(rng.randint(1, 3)):
        a = (rng.choice(gx), rng.choice(gy))
        b = (rng.choice(gx), rng.choice(gy))
        if a != b:
            out.append(poly([a, b], color=color, w=w))
    return out


def summoning_circle(cx, cy, r):
    out = [circle(cx, cy, r, CIRCLE, w=0.45), circle(cx, cy, r - 1.4, CIRCLE, w=0.25)]
    pts = [polar(cx, cy, r - 1.4, i * 360 / 7) for i in range(7)]
    star = [pts[(i * 3) % 7] for i in range(8)]  # {7/3} heptagram
    out.append(poly(star, color=CIRCLE, w=0.35))
    rng = random.Random(13)
    for i in range(28):  # runes in the band between the two rings
        a = i * 360 / 28 + 6
        x, y = polar(cx, cy, r - 0.7, a)
        out.append(
            f'<g transform="rotate({f(a)} {f(x)} {f(y)}) translate({f(x)} {f(y)}) scale(0.32) translate({f(-x)} {f(-y)})">'
            + "".join(rune(x, y, rng, color=CIRCLE, w=0.6))
            + "</g>"
        )
    return out


def tick_ring(cx, cy, r, n, color=BLOOD, long_every=None):
    out = []
    for i in range(n):
        a = -150 + 300 * i / (n - 1)
        long = long_every and i % long_every == 0
        r0, r1 = r + 0.9, r + (2.0 if long else 1.5)
        out.append(poly([polar(cx, cy, r0, a), polar(cx, cy, r1, a)], color=color, w=0.28 if long else 0.2))
    return out


def drips(x0, x1, y, rng, keep_clear=()):
    out = [poly([(x0, y), (x1, y)], color=BLOOD, w=0.5)]
    x = x0 + 1.5
    while x < x1 - 1.5:
        if any(abs(x - k) < 3.0 for k in keep_clear):  # don't bleed over the sigils
            x += 0.8
            continue
        ln = rng.uniform(1.0, 4.2)
        wd = rng.uniform(0.35, 0.6)
        out.append(
            stroke(
                f"M{f(x - wd)},{f(y)} L{f(x - wd * 0.4)},{f(y + ln)} A{f(wd * 0.75)},{f(wd * 0.75)} 0 1 0 {f(x + wd * 0.4)},{f(y + ln)} L{f(x + wd)},{f(y)} Z",
                color=BLOOD,
                w=0.05,
                fill=BLOOD,
            )
        )
        x += rng.uniform(2.2, 4.5)
    return out


# The Portal Electronics mark (docs/assets/portal.png): a doorway with an eye in a
# triangle, light pouring from it down to the threshold. Traced in the source
# image's pixel space, where the door with its threshold spans about 326 x 425.
PORTAL_CENTER = (541, 495)
PORTAL_HEIGHT = 425


def portal_elements(color, eye):
    """The mark's strokes in its own pixel space; the pupil ring is in `eye`."""
    out = []

    def line(pts, w=13):
        d = "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in pts)
        out.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linecap="round" stroke-linejoin="round"/>')

    def spot(x, y, r):
        out.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{color}"/>')

    line([(420, 697), (420, 289), (660, 289), (660, 697)])  # outer frame
    line([(446, 697), (446, 313), (635, 313), (635, 697)])  # inner frame
    line([(378, 702), (704, 702)])  # threshold
    spot(390, 665, 6.5)
    spot(691, 665, 6.5)
    line([(452, 436), (540, 322), (629, 436)])  # the triangle
    for x, y in ((495, 340), (586, 340), (540, 358), (467, 381), (613, 381)):
        spot(x, y, 6)
    eye_d = "M475,433 C490,360 590,360 606,433 C588,474 492,474 475,433 Z"
    out.append(f'<path d="{eye_d}" fill="none" stroke="{color}" stroke-width="13" stroke-linejoin="round"/>')
    out.append(f'<circle cx="540" cy="425" r="29" fill="none" stroke="{color}" stroke-width="13"/>')
    out.append(f'<circle cx="540" cy="425" r="14" fill="none" stroke="{eye}" stroke-width="9"/>')

    def lid(x):  # just below the lower lid
        u = min(1.0, abs(x - 540) / 66)
        return 438 + 30 * math.sqrt(1 - u * u)

    # Rays from along the lower lid out to the frame and the threshold.
    starts = (478, 490, 503, 515, 527, 540)
    ends = ((452, 500), (452, 568), (452, 642), (482, 697), (507, 697), (526, 697))
    for x0, (x1, y1) in zip(starts, ends, strict=True):
        for side in (-1, 1):
            line([(540 + side * (x0 - 540), lid(x0)), (540 + side * (x1 - 540), y1)], w=9)
    return out


def portal_mark(cx, cy, size, color=BLOOD, eye=BLOOD_HI):
    """The mark, `size` mm tall, centred on (cx, cy)."""
    k = size / PORTAL_HEIGHT
    ox, oy = PORTAL_CENTER
    return [f'<g transform="translate({f(cx - ox * k)} {f(cy - oy * k)}) scale({k:.6f})">', *portal_elements(color, eye), "</g>"]


def portal_logo(color, eye):
    """The mark alone as a square SVG, for the docs' logo and favicon."""
    ox, oy = PORTAL_CENTER
    half = PORTAL_HEIGHT / 2 + 8
    box = f"{ox - half:.0f} {oy - half:.0f} {2 * half:.0f} {2 * half:.0f}"
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{box}">' + "".join(portal_elements(color, eye)) + "</svg>\n"


# ----------------------------------------------------------------- panel
SPEC = {
    "title": "DOOMBRINGER",
    "flank": "chevrons",
    "controls": CONTROLS,
    "seed": 1666,
    "mark_x": COLS[0],  # the mark sits left of the altar
    "altar": (COLS[1], COLS[3]),
    # Where each control sits at its default (0..1 of the 300 degree sweep), for the preview.
    "defaults": {"CUTOFF_PARAM": 0.6, "FM_PARAM": 0.5, "HUNGER_PARAM": 0.5, "FINE_PARAM": 0.5},
}


def svg(spec):
    rng = random.Random(spec["seed"])
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{f(W)}mm" height="{H}mm" viewBox="0 0 {f(W)} {H}">',
        f'<rect width="{f(W)}" height="{H}" fill="{BG}"/>',
        f'<rect x="1.4" y="1.4" width="{f(W - 2.8)}" height="{H - 2.8}" fill="none" stroke="{BLOOD}" stroke-width="0.35"/>',
        f'<rect x="2.1" y="2.1" width="{f(W - 4.2)}" height="{H - 4.2}" fill="none" stroke="{BLOOD_DARK}" stroke-width="0.2"/>',
    ]
    out += summoning_circle(W / 2, 47, 31.5)

    # Edge runes, both sides, between the screws.
    y = 15.0
    while y < 113.0:
        out += rune(3.75, y, rng)
        out += rune(W - 3.75, y, rng)
        y += 3.55

    # Title with flanking marks.
    title, tw = text_path(spec["title"], W / 2, 10.6, size=3.5, color=BONE, tracking=0.55)
    out += title
    for side in (-1, 1):
        x = W / 2 + side * (tw / 2 + 3.2)
        out += SIGILS[spec["flank"]](x, 9.4, 1.15, color=BLOOD_HI)

    # Divider between controls and jacks: a chain of diamonds.
    for i in range(23):
        x = 6.2 + i * (W - 12.4) / 22
        out.append(poly([(x, 79.3), (x + 0.5, 80), (x, 80.7), (x - 0.5, 80), (x, 79.3)], color=BLOOD, w=0.2))

    # The mark, in the space left of the altar.
    out += portal_mark(spec["mark_x"], 116.5, 15.0)

    # Output altar.
    ax0, ax1 = spec["altar"][0] - 7.5, spec["altar"][1] + 7.5
    outs = tuple(x for kind, _, x, _, _ in spec["controls"] if kind == "out")
    out.append(f'<rect x="{f(ax0)}" y="108.6" width="{f(ax1 - ax0)}" height="16.4" rx="1.0" fill="{BLOOD_DARK}" stroke="{BLOOD}" stroke-width="0.35"/>')
    out += drips(ax0 + 0.6, ax1 - 0.6, 108.6, random.Random(7), keep_clear=outs)

    for kind, _, x, y0, sigil in spec["controls"]:
        cr = CONTROL_R[kind]
        if kind in ("knob_l", "knob_m"):
            out += tick_ring(x, y0, cr, 11, long_every=5)
        elif kind == "in":
            out.append(circle(x, y0, cr + 1.0, BONE_DIM, w=0.25))
        elif kind == "out":
            out.append(circle(x, y0, cr + 1.1, BLOOD_HI, w=0.45, fill="#4a0a0d"))
        elif kind == "button":
            out.append(circle(x, y0, cr + 0.9, BONE_DIM, w=0.25))
        elif kind == "light":
            out.append(circle(x, y0, cr + 0.6, BLOOD, w=0.25))
        elif kind == "switch":
            # Up = on (bass compensation): a small closed triangle above, open below.
            out.append(poly([(x - 0.7, y0 - 4.6), (x, y0 - 5.4), (x + 0.7, y0 - 4.6)], closed=True, color=BLOOD_HI, w=0.2, fill=BLOOD_HI))
            out.append(poly([(x - 0.7, y0 + 4.6), (x, y0 + 5.4), (x + 0.7, y0 + 4.6)], closed=True, color=BLOOD, w=0.2))
        color = BLOOD_HI if kind == "out" else BONE
        out += SIGILS[sigil](x, y0 + SIGIL_DY[kind], SIGIL_R[kind], color)

    out.append("</svg>")
    return "\n".join(out)


def preview(spec):
    """The panel with stand-in knobs, jacks and the switch drawn on, for the README and docs."""
    out = [svg(spec).replace("</svg>", "")]
    for kind, name, x, y, _ in spec["controls"]:
        if kind in ("knob_l", "knob_m"):
            r = CONTROL_R[kind] * 0.82
            a = -150 + 300 * spec["defaults"].get(name, 0.0)
            tip = polar(x, y, r * 0.78, a)
            out.append(circle(x, y, r, "#3a3536", w=0.35, fill="#1b1819"))
            out.append(circle(x, y, r * 0.86, "#262223", w=0.25, fill="#1b1819"))
            out.append(poly([polar(x, y, r * 0.2, a), tip], color="#e8e2d6", w=r * 0.13))
        elif kind in ("in", "out"):
            out.append(circle(x, y, 3.2, "#8f8a84", w=0.3, fill="#c9c6c1"))
            out.append(circle(x, y, 1.75, "#5a5652", w=0.25, fill="#121011"))
        elif kind == "switch":
            out.append(f'<rect x="{f(x - 1.5)}" y="{f(y - 3.2)}" width="3" height="6.4" rx="0.6" fill="#9a9792" stroke="#5a5652" stroke-width="0.25"/>')
            out.append(f'<rect x="{f(x - 1.0)}" y="{f(y - 2.7)}" width="2" height="2.6" rx="0.4" fill="#e8e2d6"/>')
        elif kind == "button":
            out.append(circle(x, y, 2.6, "#5a5652", w=0.3, fill="#9a9792"))
            out.append(circle(x, y, 1.9, "#7a7672", w=0.2, fill="#c9c6c1"))
        elif kind == "light":
            out.append(circle(x, y, 1.1, "#3a0a0c", w=0.2, fill="#5a0b10"))
    out.append("</svg>")
    return "\n".join(out)


def header(spec):
    ns = "panel"
    lines = [
        "// Generated by res/make_panel.py — edit the table there, not here.",
        "#pragma once",
        "",
        f"namespace {ns} {{",
        "struct Pos { float x, y; }; // millimetres from the top-left",
    ]
    for _kind, name, x, y, _ in spec["controls"]:
        lines.append(f"constexpr Pos {name} = {{{x:.2f}f, {y:.2f}f}};")
    lines.append(f"}} // namespace {ns}")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    root = Path(__file__).resolve().parent.parent
    (root / "res" / "Doombringer.svg").write_text(svg(SPEC))
    (root / "res" / "panel-preview.svg").write_text(preview(SPEC))
    (root / "src" / "PanelLayout.hpp").write_text(header(SPEC))
    print(f"wrote {root / 'res/Doombringer.svg'}, {root / 'res/panel-preview.svg'} and {root / 'src/PanelLayout.hpp'} ({HP} HP)")
    site = root.parent / "docs" / "assets" / "site"
    if site.is_dir():  # not in make panel-check's scratch copy
        logo = portal_logo("#efe3cb", "#e8363f")  # bone and fresh blood, the site's own colours
        (site / "logo.svg").write_text(logo)
        (site / "favicon.svg").write_text(logo)
        print(f"wrote {site / 'logo.svg'} and {site / 'favicon.svg'}")
