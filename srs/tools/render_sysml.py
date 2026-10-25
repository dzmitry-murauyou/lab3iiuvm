#!/usr/bin/env python3
"""Render the SysML v2 annotated views of KeyNest into PNG files.

The official SysML v2 notation has no dedicated GUI mock-up diagram type.
Therefore ``srs/sysml/keynest-views.sysml`` declares screens and diagrams as
SysML v2 elements annotated with structured metadata (@GuiScreen, @Widget,
@DiagramNode, @DiagramEdge). This small deterministic renderer reads those
annotations and draws the reproducible PNG files in ``srs/img``.

Usage (from the repository root):

    python3 -m venv .venv
    .venv/bin/pip install pillow
    .venv/bin/python srs/tools/render_sysml.py

Only Pillow and the Python standard library are required.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import math
import re
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
SRS = HERE.parent
DEFAULT_SOURCE = SRS / "sysml" / "keynest-views.sysml"
DEFAULT_OUT = SRS / "img"

FONT_DIR = Path("/usr/share/fonts/truetype/dejavu")
FONT_FILES = {
    (False, False): FONT_DIR / "DejaVuSans.ttf",
    (True, False): FONT_DIR / "DejaVuSans-Bold.ttf",
    (False, True): FONT_DIR / "DejaVuSansMono.ttf",
    (True, True): FONT_DIR / "DejaVuSansMono-Bold.ttf",
}
_FONT_CACHE: dict[tuple[int, bool, bool], ImageFont.FreeTypeFont] = {}


def font(size: int, bold: bool = False, mono: bool = False):
    key = (size, bold, mono)
    if key not in _FONT_CACHE:
        path = FONT_FILES[(bold, mono)]
        if path.exists():
            _FONT_CACHE[key] = ImageFont.truetype(str(path), size)
        else:  # pragma: no cover - fallback for minimal systems
            _FONT_CACHE[key] = ImageFont.load_default()
    return _FONT_CACHE[key]


def hx(value: str) -> tuple[int, int, int]:
    value = value.lstrip("#")
    return tuple(int(value[i : i + 2], 16) for i in (0, 2, 4))


C = {
    "bg": hx("EEF2F8"),
    "paper": hx("FFFFFF"),
    "ink": hx("16233A"),
    "muted": hx("64748B"),
    "line": hx("CBD5E1"),
    "line_light": hx("DCE4EF"),
    "dark": hx("0F172A"),
    "dark2": hx("1E293B"),
    "accent": hx("2563EB"),
    "accent_dark": hx("1D4ED8"),
    "accent_soft": hx("DBEAFE"),
    "teal": hx("0F766E"),
    "teal_soft": hx("CCFBF1"),
    "ok": hx("15803D"),
    "ok_soft": hx("DCFCE7"),
    "warn": hx("B45309"),
    "warn_soft": hx("FEF3C7"),
    "danger": hx("B91C1C"),
    "danger_soft": hx("FEE2E2"),
    "shadow": hx("C9D4E4"),
    "white": hx("FFFFFF"),
}


# ---------------------------------------------------------------------------
# SysML v2 annotation parsing
# ---------------------------------------------------------------------------
ANNOTATION = re.compile(r"@(?P<name>[A-Za-z_]\w*)\s*\{(?P<body>[^{}]*)\}", re.S)
FIELD = re.compile(
    r"(?P<key>[A-Za-z_]\w*)\s*=\s*(?P<val>\"(?:\\.|[^\"\\])*\"|true|false|-?\d+)"
)


def parse_fields(body: str) -> dict:
    out: dict = {}
    for match in FIELD.finditer(body):
        raw = match.group("val")
        if raw.startswith('"'):
            value = ast.literal_eval(raw)
        elif raw in ("true", "false"):
            value = raw == "true"
        else:
            value = int(raw)
        out[match.group("key")] = value
    return out


def strip_comments(source: str) -> str:
    """Remove /* ... */ and // ... comments but keep string literals intact.

    A naive regex would eat the ``//`` of URLs (https://... , otpauth://...)
    used in the GUI mock-up annotations.
    """
    out = []
    i, n = 0, len(source)
    in_string = in_doc = in_line = False
    while i < n:
        c = source[i]
        nxt = source[i + 1] if i + 1 < n else ""
        if in_line:
            if c == "\n":
                in_line = False
                out.append(c)
            i += 1
            continue
        if in_doc:
            if c == "*" and nxt == "/":
                in_doc = False
                out.append("  ")
                i += 2
                continue
            out.append("\n" if c == "\n" else " ")
            i += 1
            continue
        if in_string:
            out.append(c)
            if c == "\\" and i + 1 < n:
                out.append(source[i + 1])
                i += 2
                continue
            if c == '"':
                in_string = False
            i += 1
            continue
        if c == "/" and nxt == "*":
            in_doc = True
            out.append("  ")
            i += 2
            continue
        if c == "/" and nxt == "/":
            in_line = True
            out.append("  ")
            i += 2
            continue
        if c == '"':
            in_string = True
        out.append(c)
        i += 1
    return "".join(out)


def parse_views(path: Path):
    source = strip_comments(path.read_text(encoding="utf-8"))

    screens: list[dict] = []
    diagrams: dict[str, dict] = defaultdict(lambda: {"nodes": [], "edges": []})
    current_screen = None

    for match in ANNOTATION.finditer(source):
        name = match.group("name")
        fields = parse_fields(match.group("body"))
        if name == "GuiScreen":
            current_screen = dict(fields)
            current_screen["widgets"] = []
            screens.append(current_screen)
        elif name == "Widget":
            if current_screen is not None:
                current_screen["widgets"].append(fields)
        elif name == "DiagramNode":
            diagrams[fields["diagram"]]["nodes"].append(fields)
        elif name == "DiagramEdge":
            diagrams[fields["diagram"]]["edges"].append(fields)
    return diagrams, screens


# ---------------------------------------------------------------------------
# Generic drawing helpers
# ---------------------------------------------------------------------------
def box(node: dict) -> tuple[int, int, int, int]:
    x, y, w, h = node["x"], node["y"], node["w"], node["h"]
    return (x, y, x + w, y + h)


def rounded(draw, rect, fill, outline=None, width=2, radius=14, shadow=False):
    if shadow:
        d = 5
        shifted = (rect[0] + 3, rect[1] + d, rect[2] + 3, rect[3] + d)
        draw.rounded_rectangle(shifted, radius=radius, fill=C["shadow"])
    draw.rounded_rectangle(rect, radius=radius, fill=fill, outline=outline, width=width)


def dashed_line(draw, p1, p2, fill, width=2, dash=12, gap=8):
    x1, y1 = p1
    x2, y2 = p2
    length = math.hypot(x2 - x1, y2 - y1)
    if length < 1:
        return
    ux, uy = (x2 - x1) / length, (y2 - y1) / length
    pos = 0.0
    while pos < length:
        end = min(pos + dash, length)
        draw.line(
            (x1 + ux * pos, y1 + uy * pos, x1 + ux * end, y1 + uy * end),
            fill=fill,
            width=width,
        )
        pos += dash + gap


def dashed_rect(draw, rect, fill, width=2, dash=12, gap=8):
    x0, y0, x1, y1 = rect
    for a, b in (
        ((x0, y0), (x1, y0)),
        ((x1, y0), (x1, y1)),
        ((x1, y1), (x0, y1)),
        ((x0, y1), (x0, y0)),
    ):
        dashed_line(draw, a, b, fill, width, dash, gap)


def wrap_lines(text, fnt, max_width):
    lines = []
    for segment in str(text).split("|"):
        if segment == "":
            lines.append("")
            continue
        current = ""
        for word in segment.split(" "):
            candidate = (current + " " + word).strip()
            if fnt.getlength(candidate) <= max_width or not current:
                current = candidate
            else:
                lines.append(current)
                current = word
        lines.append(current)
    return lines


def draw_text(draw, rect, text, fnt, fill, align="left", valign="top", pad=12, spacing=6):
    x0, y0, x1, y1 = rect
    lines = wrap_lines(text, fnt, max(12, x1 - x0 - 2 * pad))
    line_h = int(fnt.size * 1.34) + spacing
    total = len(lines) * line_h
    if valign == "middle":
        y = y0 + max(pad, (y1 - y0 - total) / 2)
    elif valign == "bottom":
        y = y1 - total - pad
    else:
        y = y0 + pad
    for line in lines:
        tw = fnt.getlength(line)
        if align == "center":
            x = x0 + (x1 - x0 - tw) / 2
        elif align == "right":
            x = x1 - pad - tw
        else:
            x = x0 + pad
        draw.text((x, y), line, font=fnt, fill=fill)
        y += line_h


def draw_label_block(draw, rect, label, size, fill, subfill=None, align="center", pad=10):
    """Bilingual label: first '|' segment is bold, the rest is a translation."""
    x0, y0, x1, y1 = rect
    items = []
    for index, segment in enumerate(str(label).split("|")):
        fnt = font(size if index == 0 else max(11, size - 3), bold=(index == 0))
        col = fill if index == 0 else (subfill or fill)
        for line in wrap_lines(segment, fnt, x1 - x0 - 2 * pad):
            items.append((line, fnt, col))
    if not items:
        return
    heights = [int(f.size * 1.28) + 3 for _, f, _ in items]
    total = sum(heights)
    y = y0 + max(pad, (y1 - y0 - total) / 2)
    for (line, fnt, col), lh in zip(items, heights):
        tw = fnt.getlength(line)
        if align == "left":
            x = x0 + pad
        else:
            x = x0 + (x1 - x0 - tw) / 2
        draw.text((x, y), line, font=fnt, fill=col)
        y += lh


def arrow_head(draw, tip, direction, fill, size=11):
    dx, dy = direction
    norm = math.hypot(dx, dy) or 1.0
    ux, uy = dx / norm, dy / norm
    px, py = -uy, ux
    base = (tip[0] - ux * size, tip[1] - uy * size)
    draw.polygon(
        [
            tip,
            (base[0] + px * size * 0.52, base[1] + py * size * 0.52),
            (base[0] - px * size * 0.52, base[1] - py * size * 0.52),
        ],
        fill=fill,
    )


def border_point(node, towards, ellipse=False):
    x0, y0, x1, y1 = box(node)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    dx, dy = towards[0] - cx, towards[1] - cy
    if abs(dx) < 1e-6 and abs(dy) < 1e-6:
        return (cx, cy)
    if ellipse:
        rx, ry = max(1.0, (x1 - x0) / 2), max(1.0, (y1 - y0) / 2)
        t = 1.0 / math.sqrt((dx / rx) ** 2 + (dy / ry) ** 2)
        return (cx + dx * t, cy + dy * t)
    hw, hh = max(1.0, (x1 - x0) / 2), max(1.0, (y1 - y0) / 2)
    t = min(hw / (abs(dx) or 1e-9), hh / (abs(dy) or 1e-9))
    return (cx + dx * t, cy + dy * t)


def center(node):
    x0, y0, x1, y1 = box(node)
    return ((x0 + x1) / 2, (y0 + y1) / 2)


def edge_label(draw, p1, p2, text, fill):
    if not text:
        return
    fnt = font(16, True)
    mx, my = (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2 - 16
    tw = fnt.getlength(text)
    rect = (mx - tw / 2 - 9, my - 4, mx + tw / 2 + 9, my + fnt.size + 10)
    rounded(draw, rect, C["paper"], outline=C["line_light"], width=1, radius=8)
    draw.text((mx - tw / 2, my), text, font=fnt, fill=fill)


# ---------------------------------------------------------------------------
# Diagram nodes
# ---------------------------------------------------------------------------
def draw_actor(draw, node, label):
    x0, y0, x1, y1 = box(node)
    cx = (x0 + x1) / 2
    head_r = 17
    draw.ellipse((cx - head_r, y0 + 12, cx + head_r, y0 + 12 + 2 * head_r), outline=C["ink"], width=3)
    draw.line((cx, y0 + 46, cx, y0 + 96), fill=C["ink"], width=3)
    draw.line((cx - 26, y0 + 62, cx + 26, y0 + 62), fill=C["ink"], width=3)
    draw.line((cx, y0 + 96, cx - 22, y0 + 122), fill=C["ink"], width=3)
    draw.line((cx, y0 + 96, cx + 22, y0 + 122), fill=C["ink"], width=3)
    draw_label_block(draw, (x0, y0 + 112, x1, y1), label, 19, C["ink"], C["muted"])


def draw_node(draw, node):
    style = node.get("style", "default")
    rect = box(node)
    label = node.get("label", node.get("id", ""))

    if style == "boundary":
        rounded(draw, rect, hx("F8FAFD"), outline=None, radius=20)
        dashed_rect(draw, (rect[0] + 4, rect[1] + 4, rect[2] - 4, rect[3] - 4), C["accent"], width=2)
        draw_label_block(draw, (rect[0] + 20, rect[1] + 8, rect[2] - 20, rect[1] + 56), label, 20, C["accent_dark"], C["muted"], align="left")
    elif style == "actor":
        draw_actor(draw, node, label)
    elif style == "system":
        rounded(draw, rect, C["accent_soft"], outline=C["accent"], width=3, radius=20, shadow=True)
        draw_label_block(draw, rect, label, 25, C["dark"], C["accent_dark"])
        draw.rounded_rectangle((rect[0] + 24, rect[1] + 18, rect[0] + 30, rect[3] - 18), radius=3, fill=C["accent"])
    elif style in ("external", "external_dashed"):
        rounded(draw, rect, C["paper"], outline=None, radius=16, shadow=True)
        if style == "external_dashed":
            dashed_rect(draw, (rect[0] + 3, rect[1] + 3, rect[2] - 3, rect[3] - 3), C["muted"], width=2, dash=9, gap=7)
        else:
            draw.rounded_rectangle(rect, radius=16, outline=C["line"], width=2)
        draw_label_block(draw, rect, label, 20, C["ink"], C["muted"])
    elif style == "storage":
        x0, y0, x1, y1 = rect
        rounded(draw, (x0, y0 + 20, x1, y1), C["paper"], outline=C["line"], width=2, radius=14, shadow=True)
        draw.ellipse((x0, y0, x1, y0 + 44), fill=C["accent_soft"], outline=C["accent"], width=2)
        draw_label_block(draw, (x0, y0 + 42, x1, y1), label, 20, C["ink"], C["muted"])
    elif style == "usecase":
        draw.ellipse(rect, fill=C["paper"], outline=C["accent"], width=3)
        draw_label_block(draw, (rect[0] + 12, rect[1] + 12, rect[2] - 12, rect[3] - 12), label, 19, C["ink"], C["muted"])
    elif style == "goal":
        rounded(draw, rect, C["paper"], outline=C["line"], width=2, radius=14, shadow=True)
        draw.rounded_rectangle((rect[0], rect[1] + 12, rect[0] + 8, rect[3] - 12), radius=4, fill=C["teal"])
        draw_label_block(draw, (rect[0] + 18, rect[1], rect[2], rect[3]), label, 19, C["ink"], C["muted"], align="left")
    elif style == "req":
        rounded(draw, rect, C["paper"], outline=C["line_light"], width=2, radius=12)
        draw_label_block(draw, rect, label, 18, C["dark"], C["muted"], align="left")
    elif style == "release":
        rounded(draw, rect, C["teal"], outline=None, radius=14, shadow=True)
        draw_label_block(draw, rect, label, 20, C["white"], hx("A7F3D0"), align="left")
    elif style == "header":
        rounded(draw, rect, C["dark"], outline=None, radius=12)
        draw_label_block(draw, rect, label, 19, C["white"], hx("CBD5E1"))
    elif style == "legend":
        rounded(draw, rect, hx("F8FAFD"), outline=C["line"], width=2, radius=12)
        draw_label_block(draw, rect, label, 17, C["muted"], C["muted"], align="left")
    else:
        rounded(draw, rect, C["paper"], outline=C["line"], width=2, radius=12)
        draw_label_block(draw, rect, label, 18, C["ink"], C["muted"])


def draw_edge(draw, edge, nodes, bus_x=445):
    style = edge.get("style", "solid")
    src, dst = nodes.get(edge["src"]), nodes.get(edge["dst"])
    if src is None or dst is None:
        return
    sc, dc = center(src), center(dst)
    label = edge.get("label", "")

    if style == "bus":
        color = hx("A9B8CE")
        draw.line((sc[0] + 100, sc[1], bus_x, sc[1]), fill=color, width=2)
        draw.line((bus_x, sc[1], bus_x, dc[1]), fill=color, width=2)
        draw.line((bus_x, dc[1], dst["x"], dc[1]), fill=color, width=2)
        return

    if style in ("trace", "alloc"):
        p1 = border_point(src, dc)
        p2 = border_point(dst, sc)
        color = hx("B7C6DA") if style == "trace" else hx("8FB3B0")
        if style == "alloc":
            dashed_line(draw, p1, p2, color, width=2, dash=8, gap=7)
        else:
            draw.line((p1, p2), fill=color, width=2)
        return

    ellipse_src = src.get("style") == "usecase"
    ellipse_dst = dst.get("style") == "usecase"
    p1 = border_point(src, dc, ellipse_src)
    p2 = border_point(dst, sc, ellipse_dst)
    dashed = style == "dashed"
    color = C["muted"] if dashed else C["accent_dark"]
    width = 2 if dashed else 3
    if dashed:
        dashed_line(draw, p1, p2, color, width=width, dash=10, gap=7)
    else:
        draw.line((p1, p2), fill=color, width=width)
    arrow_head(draw, p2, (p2[0] - p1[0], p2[1] - p1[1]), color, size=12 if not dashed else 9)
    edge_label(draw, p1, p2, label, color)


def diagram_header(draw, title, subtitle, width):
    draw.rectangle((0, 0, width, 100), fill=C["dark"])
    draw.text((40, 16), title, font=font(30, True), fill=C["white"])
    draw.text((42, 58), subtitle, font=font(17), fill=hx("B6C4DA"))
    draw.rectangle((0, 100, width, 104), fill=C["accent"])


def diagram_footer(draw, width, height, text):
    y = height - 42
    draw.rectangle((0, y, width, height), fill=hx("E3EAF4"))
    draw.line((0, y, width, y), fill=C["line"], width=1)
    draw.text((40, y + 11), text, font=font(15), fill=C["muted"])


def render_diagram(name, data, out_dir: Path):
    width, height = 1600, 1000
    meta = {
        "context": (
            "KeyNest - System Context Diagram",
            "SysML v2 view 1/3 - actors, system boundary and external services",
            "context-diagram.png",
        ),
        "usecase": (
            "KeyNest - Use Case Diagram",
            "SysML v2 view 2/3 - UC-01..UC-10 inside the system boundary",
            "use-case-diagram.png",
        ),
        "requirements": (
            "KeyNest - Business Requirements Trace",
            "SysML v2 view 3/3 - goals to BR-01..BR-10 to release increments",
            "requirements-diagram.png",
        ),
    }[name]
    title, subtitle, filename = meta
    img = Image.new("RGB", (width, height), C["bg"])
    draw = ImageDraw.Draw(img)
    diagram_header(draw, title, subtitle, width)

    nodes = {n["id"]: n for n in data["nodes"]}
    edges = data["edges"]

    # Boundary first (behind everything).
    for node in nodes.values():
        if node.get("style") == "boundary":
            draw_node(draw, node)

    # Edges (lines only); labels are painted after nodes for readability.
    for edge in edges:
        draw_edge(draw, edge, nodes)

    for node in nodes.values():
        if node.get("style") != "boundary":
            draw_node(draw, node)

    for edge in edges:
        style = edge.get("style", "solid")
        if style in ("solid", "dashed") and edge.get("label"):
            src, dst = nodes.get(edge["src"]), nodes.get(edge["dst"])
            if src and dst:
                sc, dc = center(src), center(dst)
                p1 = border_point(src, dc, src.get("style") == "usecase")
                p2 = border_point(dst, sc, dst.get("style") == "usecase")
                edge_label(draw, p1, p2, edge["label"], C["muted"] if style == "dashed" else C["accent_dark"])

    diagram_footer(draw, width, height, f"{filename} - generated from srs/sysml/keynest-views.sysml")
    out = out_dir / filename
    img.save(out, optimize=True)
    print(f"  {out.relative_to(SRS.parent)}")


# ---------------------------------------------------------------------------
# GUI widgets
# ---------------------------------------------------------------------------
def widget_rect(w):
    return (w["x"], w["y"], w["x"] + w["w"], w["y"] + w["h"])


def draw_checkbox(draw, rect, checked, color=C["accent"]):
    x0, y0, _, _ = rect
    s = 22
    draw.rounded_rectangle((x0, y0 + 4, x0 + s, y0 + 4 + s), radius=5, fill=C["paper"], outline=color, width=2)
    if checked:
        draw.line((x0 + 5, y0 + 15, x0 + 9, y0 + 20, x0 + 18, y0 + 8), fill=color, width=3, joint="curve")


def draw_widget(draw, w):
    kind = w.get("kind", "label")
    style = w.get("style", "default")
    rect = widget_rect(w)
    x0, y0, x1, y1 = rect
    text = w.get("text", "")

    if kind == "titlebar":
        draw.rectangle(rect, fill=C["dark"])
        draw.rectangle((0, y1 - 3, x1, y1), fill=C["accent"])
        for i, col in enumerate((hx("F87171"), hx("FBBF24"), hx("34D399"))):
            draw.ellipse((x1 - 96 + i * 26, y0 + 24, x1 - 84 + i * 26, y0 + 36), fill=col)
        parts = str(text).split("|")
        draw.text((x0 + 28, y0 + 14), parts[0], font=font(23, True), fill=C["white"])
        if len(parts) > 1:
            draw.text((x0 + 210, y0 + 20), parts[1], font=font(17), fill=hx("A8B6CC"))

    elif kind == "panel":
        if style == "brand":
            rounded(draw, rect, C["dark"], outline=None, radius=0)
            # Two subtle diagonal bands, clipped to the panel rectangle.
            draw.polygon(
                [(x0, y1 - 250), (x1, y0 + 90), (x1, y0 + 148), (x0, y1 - 192)],
                fill=hx("152241"),
            )
            draw.polygon(
                [(x0, y1 - 150), (x1, y0 + 190), (x1, y0 + 214), (x0, y1 - 126)],
                fill=hx("172A4E"),
            )
            draw.rectangle((x0, y0, x1, y0 + 4), fill=C["accent"])
        elif style == "light":
            rounded(draw, rect, hx("F4F7FC"), outline=C["line_light"], width=2, radius=20, shadow=True)
        else:
            rounded(draw, rect, C["accent_soft"], outline=None, radius=16)

    elif kind == "logo":
        draw.ellipse(rect, fill=C["accent"], outline=hx("93C5FD"), width=4)
        draw.ellipse((x0 + 10, y0 + 10, x1 - 10, y1 - 10), outline=hx("BFDBFE"), width=2)
        fnt = font(46, True)
        tw = fnt.getlength(str(text))
        draw.text(((x0 + x1 - tw) / 2, y0 + 16), str(text), font=fnt, fill=C["white"])

    elif kind == "label":
        if style == "title":
            fnt, col = font(32, True), C["ink"]
        elif style == "hero_white":
            fnt, col = font(40, True), C["white"]
        elif style == "white":
            fnt, col = font(20), C["white"]
        elif style == "bold":
            fnt, col = font(19, True), C["ink"]
        elif style == "muted":
            fnt, col = font(17), C["muted"]
        else:
            fnt, col = font(19), C["ink"]
        if style == "white":
            for index, line in enumerate(str(text).split("|")):
                draw.ellipse((x0 + 4, y0 + 8 + index * 34, x0 + 12, y0 + 16 + index * 34), fill=hx("60A5FA"))
                draw.text((x0 + 26, y0 + index * 34), line, font=fnt, fill=col)
        else:
            draw_text(draw, rect, text, fnt, col, align="left", valign="middle", pad=0, spacing=4)

    elif kind == "chip":
        fills = {
            "accent": (C["accent_soft"], C["accent_dark"]),
            "ok": (C["ok_soft"], C["ok"]),
            "warn": (C["warn_soft"], C["warn"]),
            "danger": (C["danger_soft"], C["danger"]),
            "muted": (hx("E9EFF7"), C["muted"]),
        }
        fill, col = fills.get(style, fills["accent"])
        rounded(draw, rect, fill, outline=None, radius=(y1 - y0) // 2)
        draw_text(draw, rect, text, font(16, True), col, align="center", valign="middle", pad=10, spacing=0)

    elif kind == "button":
        if style == "primary":
            fill, outline, col = C["accent"], C["accent"], C["white"]
        elif style == "danger":
            fill, outline, col = C["paper"], C["danger"], C["danger"]
        elif style == "ghost":
            fill, outline, col = C["bg"], C["line"], C["muted"]
        else:
            fill, outline, col = C["paper"], C["line"], C["ink"]
        rounded(draw, rect, fill, outline=outline, width=2, radius=12, shadow=(style == "primary"))
        draw_text(draw, rect, text, font(20, True), col, align="center", valign="middle", pad=10, spacing=0)

    elif kind == "input":
        rounded(draw, rect, C["paper"], outline=C["line"], width=2, radius=12)
        parts = str(text).split("|")
        label = parts[0]
        value = parts[1] if len(parts) > 1 else ""
        draw.text((x0 + 18, y0 + 10), label, font=font(15), fill=C["muted"])
        if value:
            if style == "password":
                fnt = font(22, True, mono=True)
                draw.text((x0 + 18, y0 + 36), value, font=fnt, fill=C["ink"])
            else:
                fnt = font(21, mono=(style == "filled" and label.startswith("Секрет")))
                draw.text((x0 + 18, y0 + 36), value, font=fnt, fill=C["ink"])
        if style == "empty":
            draw.text((x0 + 18, y0 + 36), "", font=font(21), fill=C["ink"])
        if label.startswith("Категория"):
            draw.polygon(((x1 - 30, y0 + 34), (x1 - 18, y0 + 34), (x1 - 24, y0 + 43)), fill=C["muted"])

    elif kind == "code":
        digits = str(text).replace(" ", "")
        gap = 8
        bw = (w["w"] - gap * 5) / 6
        for i in range(6):
            bx = x0 + i * (bw + gap)
            rounded(draw, (bx, y0, bx + bw, y1), C["paper"], outline=C["accent"], width=2, radius=9)
            ch = digits[i] if i < len(digits) else "-"
            fnt = font(28, True, mono=True)
            tw = fnt.getlength(ch)
            draw.text((bx + (bw - tw) / 2, y0 + (w["h"] - 38) / 2), ch, font=fnt, fill=C["accent_dark"])

    elif kind == "countdown":
        draw.ellipse(rect, fill=C["accent_soft"], outline=C["accent"], width=3)
        fnt = font(22, True)
        tw = fnt.getlength(str(text))
        draw.text(((x0 + x1 - tw) / 2, y0 + (w["h"] - 30) / 2), str(text), font=fnt, fill=C["accent_dark"])

    elif kind == "checkbox":
        draw_checkbox(draw, rect, style == "checked", C["accent"] if style == "checked" else C["line"])
        draw.text((x0 + 34, y0 + 6), str(text), font=font(18), fill=C["ink"] if style == "checked" else C["muted"])

    elif kind == "note":
        fills = {
            "warn": (C["warn_soft"], C["warn"]),
            "light": (hx("EFF6FF"), C["accent_dark"]),
            "default": (hx("F1F5F9"), C["ink"]),
        }
        fill, col = fills.get(style, fills["light"])
        rounded(draw, rect, fill, outline=None, radius=12)
        parts = str(text).split("|")
        if len(parts) > 1:
            draw.text((x0 + 16, y0 + 10), parts[0], font=font(15, True), fill=col)
            draw_text(draw, (x0 + 16, y0 + 28, x1 - 16, y1 - 6), "|".join(parts[1:]), font(16), C["ink"], align="left", valign="top", pad=0, spacing=2)
        else:
            draw_text(draw, (x0 + 16, y0, x1 - 16, y1), text, font(17), C["ink"], align="left", valign="middle", pad=0, spacing=2)

    elif kind == "card":
        if style == "overlay":
            rounded(draw, rect, C["paper"], outline=C["line_light"], width=2, radius=22, shadow=True)
        elif style == "mono":
            rounded(draw, rect, hx("F1F5FF"), outline=C["accent"], width=2, radius=12)
            draw_text(draw, rect, text, font(26, True, mono=True), C["accent_dark"], align="center", valign="middle", pad=10, spacing=0)
        elif style == "light":
            rounded(draw, rect, hx("F5F8FD"), outline=C["line_light"], width=2, radius=12)
            parts = str(text).split("|")
            if parts and parts[0]:
                draw.text((x0 + 20, y0 + 16), parts[0], font=font(22, True), fill=C["ink"])
                draw_text(draw, (x0 + 20, y0 + 52, x1 - 20, y1 - 10), "|".join(parts[1:]), font(18), C["muted"], align="left", valign="top", pad=0, spacing=2)
        else:
            rounded(draw, rect, C["paper"], outline=C["line_light"], width=2, radius=18, shadow=True)

    elif kind == "sidebar":
        rounded(draw, rect, C["dark"], outline=None, radius=18)
        y = y0 + 22
        for line in str(text).split("|"):
            if line.startswith("#"):
                draw.text((x0 + 22, y + 6), line[1:], font=font(14, True), fill=hx("7EA6FF"))
                y += 34
            else:
                draw.ellipse((x0 + 24, y + 9, x0 + 32, y + 17), fill=hx("4B6A9E"))
                draw.text((x0 + 44, y + 2), line, font=font(18), fill=hx("E4EBF7"))
                y += 38

    elif kind == "search":
        rounded(draw, rect, C["paper"], outline=C["line"], width=2, radius=14)
        cx, cy = x0 + 32, y0 + w["h"] / 2 - 3
        draw.ellipse((cx - 11, cy - 11, cx + 11, cy + 11), outline=C["muted"], width=2)
        draw.line((cx + 8, cy + 8, cx + 17, cy + 17), fill=C["muted"], width=2)
        draw.text((x0 + 66, y0 + 14), str(text), font=font(19), fill=C["muted"])

    elif kind == "list":
        rounded(draw, rect, C["paper"], outline=C["line_light"], width=2, radius=16, shadow=True)
        rows = [r for r in str(text).split("|") if r]
        n = max(1, len(rows))
        row_h = (w["h"] - 16) / n
        for i, row in enumerate(rows):
            ry = y0 + 8 + i * row_h
            draw.rounded_rectangle((x0 + 12, ry + 10, x0 + 44, ry + 42), radius=9, fill=hx("E8EFFB"))
            draw.text((x0 + 23, ry + 15), "K", font=font(17, True), fill=C["accent_dark"])
            parts = row.split(" - ")
            draw.text((x0 + 60, ry + 10), parts[0], font=font(19, True), fill=C["ink"])
            if len(parts) > 1:
                draw.text((x0 + 60, ry + 35), parts[1], font=font(16), fill=C["muted"])
            draw.text((x1 - 140, ry + 22), "............", font=font(15, mono=True), fill=C["muted"])
            if i in (0, 1, 3):
                rounded(draw, (x1 - 58, ry + 16, x1 - 14, ry + 42), C["ok_soft"], radius=8)
                draw.text((x1 - 51, ry + 19), "2FA", font=font(12, True), fill=C["ok"])
            if i < n - 1:
                draw.line((x0 + 16, ry + row_h, x1 - 16, ry + row_h), fill=C["line_light"], width=1)

    elif kind == "strength":
        bar_w = w["w"] * 0.36
        seg = (bar_w - 18) / 4
        col = C["ok"] if style == "ok" else (C["warn"] if style == "warn" else C["accent"])
        for i in range(4):
            sx = x0 + i * (seg + 6)
            rounded(draw, (sx, y0 + 8, sx + seg, y1 - 8), col if i < 4 else C["line_light"], radius=6)
        draw_text(draw, (x0 + bar_w + 20, y0, x1, y1), text, font(17, True), col, align="left", valign="middle", pad=0, spacing=0)

    elif kind == "slider":
        cy = (y0 + y1) / 2
        draw.rounded_rectangle((x0, cy - 5, x1, cy + 5), radius=5, fill=C["line"])
        value = int(str(text)) if str(text).isdigit() else 20
        kx = x0 + (x1 - x0) * min(1.0, value / 64.0)
        draw.rounded_rectangle((x0, cy - 5, kx, cy + 5), radius=5, fill=C["accent"])
        draw.ellipse((kx - 13, cy - 13, kx + 13, cy + 13), fill=C["paper"], outline=C["accent"], width=3)

    elif kind == "qr":
        rounded(draw, rect, C["paper"], outline=C["line"], width=2, radius=8)
        n = 13
        pad = 8
        cell = (min(w["w"], w["h"]) - 2 * pad) / n
        seed = hashlib.sha256(str(text).encode()).digest()
        for r in range(n):
            for col_i in range(n):
                bit = seed[(r * n + col_i) % len(seed)] >> ((r + col_i) % 8) & 1
                if bit:
                    draw.rectangle(
                        (x0 + pad + col_i * cell, y0 + pad + r * cell, x0 + pad + (col_i + 1) * cell, y0 + pad + (r + 1) * cell),
                        fill=C["dark"],
                    )
        for fx, fy in ((0, 0), (n - 4, 0), (0, n - 4)):
            draw.rectangle((x0 + pad + fx * cell, y0 + pad + fy * cell, x0 + pad + (fx + 4) * cell, y0 + pad + (fy + 4) * cell), outline=C["accent"], width=2)

    elif kind == "hotkey":
        parts = [p.strip() for p in str(text).split("+")]
        fnt = font(17, True)
        widths = [fnt.getlength(p) + 26 for p in parts]
        total = sum(widths) + 12 * (len(parts) - 1)
        x = x0 + (w["w"] - total) / 2
        for i, part in enumerate(parts):
            rounded(draw, (x, y0 + 3, x + widths[i], y1 - 3), C["paper"], outline=C["line"], width=2, radius=8)
            draw.text((x + 13, y0 + 12), part, font=fnt, fill=C["ink"])
            x += widths[i]
            if i < len(parts) - 1:
                draw.text((x + 1, y0 + 12), "+", font=font(17, True), fill=C["muted"])
                x += 12

    elif kind == "window":
        rounded(draw, rect, C["paper"], outline=C["line"], width=2, radius=16, shadow=True)
        draw.rounded_rectangle((x0, y0, x1, y0 + 54), radius=16, fill=hx("EDF2F9"))
        draw.rectangle((x0, y0 + 38, x1, y0 + 54), fill=hx("EDF2F9"))
        for i, col in enumerate((hx("F87171"), hx("FBBF24"), hx("34D399"))):
            draw.ellipse((x0 + 22 + i * 24, y0 + 20, x0 + 34 + i * 24, y0 + 32), fill=col)
        rounded(draw, (x0 + 120, y0 + 12, x1 - 30, y0 + 42), C["white"], outline=C["line"], width=1, radius=9)
        draw.text((x0 + 136, y0 + 17), str(text), font=font(16), fill=C["muted"])

    elif kind == "divider":
        draw.line((x0, y0, x1, y0), fill=C["line"], width=2)


def render_screen(screen, out_dir: Path):
    width, height = int(screen["w"]), int(screen["h"])
    extra = 46
    img = Image.new("RGB", (width, height + extra), C["bg"])
    draw = ImageDraw.Draw(img)

    # Subtle desktop background.
    for i in range(0, width, 80):
        draw.line((i, height, i + 120, height + extra), fill=hx("E4EBF5"), width=2)

    for widget in screen["widgets"]:
        draw_widget(draw, widget)

    draw.rectangle((0, height, width, height + extra), fill=C["dark"])
    draw.rectangle((0, height, width, height + 3), fill=C["accent"])
    info = f"{screen['title']}  |  {screen['subtitle']}  |  {screen['file']}"
    draw.text((26, height + 12), info, font=font(16), fill=hx("CBD5E1"))
    draw.text((width - 330, height + 12), "generated from keynest-views.sysml", font=font(14), fill=hx("8FA3BF"))

    out = out_dir / screen["file"]
    img.save(out, optimize=True)
    print(f"  {out.relative_to(SRS.parent)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE, help="SysML v2 views file")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="PNG output directory")
    args = parser.parse_args()

    args.out.mkdir(parents=True, exist_ok=True)
    diagrams, screens = parse_views(args.source)
    print(f"Parsed {args.source}: {len(diagrams)} diagrams, {len(screens)} GUI screens")
    for name in ("context", "usecase", "requirements"):
        if name in diagrams:
            render_diagram(name, diagrams[name], args.out)
    for screen in screens:
        render_screen(screen, args.out)
    print("Done.")


if __name__ == "__main__":
    main()
