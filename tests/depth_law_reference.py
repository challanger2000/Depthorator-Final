#!/usr/bin/env python3
"""Deterministic reference checks for the Depthorator V2 musical depth law."""

import math

DEPTH_POINTS = (0.0, 0.25, 0.50, 0.75, 1.0)
CURVE_POINTS = (0.0, 0.25, 0.50, 0.75, 1.0)

def law(depth: float, curve: float):
    depth_amount = depth ** 1.15
    creative_amount = depth_amount * depth_amount
    progression_rate = 0.10 + 0.90 * (curve ** 1.35)
    target_cutoff = 18500.0 * (0.12 ** depth_amount) + 900.0 * depth_amount
    cutoff_blend = depth_amount * (0.22 + 0.78 * progression_rate)
    cutoff = 20000.0 + (target_cutoff - 20000.0) * cutoff_blend
    repeat_to_room = (
        0.08
        + depth_amount * (0.22 + 0.55 * progression_rate)
        + creative_amount * progression_rate * 0.15
    )
    room_output = 0.20 + depth_amount * 0.55 + creative_amount * 0.20
    direct_echo = 1.0 - depth_amount * 0.10 - creative_amount * 0.22
    room_into_feedback = progression_rate * (
        depth_amount * 0.08 + creative_amount * 0.10
    )
    source_to_room = 0.14 + depth_amount * 0.04
    return {
        "cutoff": cutoff,
        "repeat_to_room": repeat_to_room,
        "room_output": room_output,
        "direct_echo": direct_echo,
        "room_into_feedback": room_into_feedback,
        "source_to_room": source_to_room,
    }

def assert_non_decreasing(values, name):
    assert all(b >= a - 1e-12 for a, b in zip(values, values[1:])), (name, values)

def assert_non_increasing(values, name):
    assert all(b <= a + 1e-12 for a, b in zip(values, values[1:])), (name, values)

def main():
    for curve in CURVE_POINTS:
        rows = [law(depth, curve) for depth in DEPTH_POINTS]
        assert_non_increasing([x["cutoff"] for x in rows], f"cutoff/depth curve={curve}")
        assert_non_decreasing([x["repeat_to_room"] for x in rows], f"room send/depth curve={curve}")
        assert_non_decreasing([x["room_output"] for x in rows], f"room output/depth curve={curve}")
        assert_non_increasing([x["direct_echo"] for x in rows], f"direct echo/depth curve={curve}")
        assert_non_decreasing([x["room_into_feedback"] for x in rows], f"room feedback/depth curve={curve}")
        assert_non_decreasing([x["source_to_room"] for x in rows], f"source room/depth curve={curve}")

    for depth in DEPTH_POINTS[1:]:
        rows = [law(depth, curve) for curve in CURVE_POINTS]
        assert_non_increasing([x["cutoff"] for x in rows], f"cutoff/curve depth={depth}")
        assert_non_decreasing([x["repeat_to_room"] for x in rows], f"room send/curve depth={depth}")
        assert_non_decreasing([x["room_into_feedback"] for x in rows], f"room feedback/curve depth={depth}")
        room_outputs = [x["room_output"] for x in rows]
        direct_echoes = [x["direct_echo"] for x in rows]
        assert max(room_outputs) - min(room_outputs) < 1e-12
        assert max(direct_echoes) - min(direct_echoes) < 1e-12

    neutral = law(0.0, 1.0)
    assert math.isclose(neutral["cutoff"], 20000.0, abs_tol=1e-12)
    assert math.isclose(neutral["room_into_feedback"], 0.0, abs_tol=1e-12)
    assert math.isclose(neutral["direct_echo"], 1.0, abs_tol=1e-12)

    maximum = law(1.0, 1.0)
    assert maximum["room_into_feedback"] <= 0.18 + 1e-12
    assert maximum["direct_echo"] >= 0.65

    print("Depthorator V2 musical depth-law reference: PASS")

if __name__ == "__main__":
    main()
