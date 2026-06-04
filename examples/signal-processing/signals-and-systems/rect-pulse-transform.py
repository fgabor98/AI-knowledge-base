#!/usr/bin/env python3
"""Generate a rectangular pulse and Fourier-transform magnitude SVG."""

from __future__ import annotations

import math
from pathlib import Path


WIDTH = 920
HEIGHT = 380
MARGIN = 54
GAP = 54
PANEL_W = (WIDTH - 2 * MARGIN - GAP) / 2
PANEL_H = HEIGHT - 2 * MARGIN


def map_point(x: float, y: float, xr: tuple[float, float], yr: tuple[float, float], x0: float) -> tuple[float, float]:
    sx = x0 + (x - xr[0]) / (xr[1] - xr[0]) * PANEL_W
    sy = MARGIN + (yr[1] - y) / (yr[1] - yr[0]) * PANEL_H
    return sx, sy


def polyline(points: list[tuple[float, float]], css_class: str) -> str:
    data = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
    return f'<polyline class="{css_class}" points="{data}" />'


def line(x1: float, y1: float, x2: float, y2: float, css_class: str = "axis") -> str:
    return f'<line class="{css_class}" x1="{x1:.2f}" y1="{y1:.2f}" x2="{x2:.2f}" y2="{y2:.2f}" />'


def text(x: float, y: float, value: str, css_class: str = "label") -> str:
    return f'<text class="{css_class}" x="{x:.2f}" y="{y:.2f}">{value}</text>'


def sinc(x: float) -> float:
    if abs(x) < 1e-12:
        return 1.0
    return math.sin(x) / x


def main() -> None:
    out_dir = Path(__file__).resolve().parents[3] / "docs" / "assets" / "signal-processing" / "signals-and-systems"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_file = out_dir / "rect-pulse-transform.svg"

    left_x0 = MARGIN
    right_x0 = MARGIN + PANEL_W + GAP
    elements: list[str] = []

    # Left panel: rectangular pulse x(t), T = 2.
    tx = (-3.0, 3.0)
    ty = (-0.2, 1.25)
    x_axis_y = map_point(0, 0, tx, ty, left_x0)[1]
    y_axis_x = map_point(0, 0, tx, ty, left_x0)[0]
    elements.append(line(left_x0, x_axis_y, left_x0 + PANEL_W, x_axis_y))
    elements.append(line(y_axis_x, MARGIN, y_axis_x, MARGIN + PANEL_H))
    step = [(-3, 0), (-1, 0), (-1, 1), (1, 1), (1, 0), (3, 0)]
    elements.append(polyline([map_point(x, y, tx, ty, left_x0) for x, y in step], "signal"))
    for tick, label in [(-1, "-T/2"), (0, "0"), (1, "T/2")]:
        px, py = map_point(tick, 0, tx, ty, left_x0)
        elements.append(line(px, py - 4, px, py + 4, "tick"))
        elements.append(text(px - 12, py + 20, label, "tick-label"))
    elements.append(text(left_x0 + PANEL_W / 2 - 46, MARGIN - 18, "rectangular pulse x(t)", "title"))
    elements.append(text(left_x0 + PANEL_W - 12, x_axis_y + 28, "t", "label"))
    elements.append(text(y_axis_x + 8, MARGIN + 14, "1", "label"))

    # Right panel: normalized Fourier-transform magnitude |X(j omega)| / T.
    wx = (-12.0, 12.0)
    wy = (-0.05, 1.1)
    x_axis_y = map_point(0, 0, wx, wy, right_x0)[1]
    y_axis_x = map_point(0, 0, wx, wy, right_x0)[0]
    elements.append(line(right_x0, x_axis_y, right_x0 + PANEL_W, x_axis_y))
    elements.append(line(y_axis_x, MARGIN, y_axis_x, MARGIN + PANEL_H))
    spectrum_points = []
    pulse_width = 2.0
    for idx in range(700):
        omega = wx[0] + idx * (wx[1] - wx[0]) / 699
        mag = abs(sinc(omega * pulse_width / 2))
        spectrum_points.append(map_point(omega, mag, wx, wy, right_x0))
    elements.append(polyline(spectrum_points, "spectrum"))
    for tick, label in [(-math.pi, "-2pi/T"), (0, "0"), (math.pi, "2pi/T")]:
        px, py = map_point(tick, 0, wx, wy, right_x0)
        elements.append(line(px, py - 4, px, py + 4, "tick"))
        elements.append(text(px - 22, py + 20, label, "tick-label"))
    elements.append(text(right_x0 + PANEL_W / 2 - 86, MARGIN - 18, "normalized magnitude spectrum", "title"))
    elements.append(text(right_x0 + PANEL_W - 52, x_axis_y + 28, "omega", "label"))
    elements.append(text(y_axis_x + 8, MARGIN + 14, "|X(j omega)| / T", "label"))

    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{HEIGHT}" viewBox="0 0 {WIDTH} {HEIGHT}" role="img" aria-labelledby="title desc">
  <title id="title">Rectangular pulse and Fourier-transform magnitude</title>
  <desc id="desc">A rectangular pulse in time and its normalized sinc-shaped Fourier-transform magnitude.</desc>
  <style>
    .axis {{ stroke: #30343b; stroke-width: 1.4; }}
    .tick {{ stroke: #30343b; stroke-width: 1; }}
    .signal {{ fill: none; stroke: #0f766e; stroke-width: 3; stroke-linejoin: round; }}
    .spectrum {{ fill: none; stroke: #7c2d12; stroke-width: 2.5; }}
    .title {{ font: 600 15px sans-serif; fill: #111827; }}
    .label {{ font: 13px sans-serif; fill: #111827; }}
    .tick-label {{ font: 11px sans-serif; fill: #374151; }}
  </style>
  <rect width="100%" height="100%" fill="#ffffff" />
  {"\n  ".join(elements)}
</svg>
"""
    out_file.write_text(svg, encoding="utf-8")
    print(out_file)


if __name__ == "__main__":
    main()
