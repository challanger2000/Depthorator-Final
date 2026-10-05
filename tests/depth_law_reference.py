#!/usr/bin/env python3
"""Deterministic reference checks for the Depthorator V2 depth law.

This does not replace rendered-audio measurements. It protects the intended control
law from accidental non-monotonic edits before full plugin QA is run.
"""
import math

DEPTH_POINTS = (0.0, 0.25, 0.50, 0.75, 1.0)
CURVE_POINTS = (0.0, 0.25, 0.50, 0.75, 1.0)

def law(depth: float, curve: float):
    depth_amount = depth ** 1.15
    progression_rate = 0.10 + 0.90 * (curve ** 1.35)
    target_cutoff = 18500.0 * (0.12 ** depth_amount) + 900.0 * depth_amount
    cutoff = 20000.0 + (target_cutoff - 20000.0) * (depth_amount * progression_rate)
    repeat_to_room = 0.10 + depth_amount * (0.25 + 0.65 * progression_rate)
    room_output = 0.22 + depth_amount * 0.58
    direct_echo = 1.0 - depth_amount * 0.15
    room_into_feedback = depth_amount * progression_rate * 0.18
    return {
        "cutoff": cutoff,
        "repeat_to_room": repeat_to_room,
        "room_output": room_output,
        "direct_echo": direct_echo,
        "room_into_feedback": room_into_feedback,
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

    for depth in DEPTH_POINTS[1:]:
        rows = [law(depth, curve) for curve in CURVE_POINTS]
        assert_non_increasing([x["cutoff"] for x in rows], f"cutoff/curve depth={depth}")
        assert_non_decreasing([x["repeat_to_room"] for x in rows], f"room send/curve depth={depth}")
        assert_non_decreasing([x["room_into_feedback"] for x in rows], f"room feedback/curve depth={depth}")
        # CURVE must not redefine the endpoint-only output terms.
        room_outputs = [x["room_output"] for x in rows]
        direct_echoes = [x["direct_echo"] for x in rows]
        assert max(room_outputs) - min(room_outputs) < 1e-12
        assert max(direct_echoes) - min(direct_echoes) < 1e-12

    neutral = law(0.0, 1.0)
    assert math.isclose(neutral["cutoff"], 20000.0, abs_tol=1e-12)
    assert math.isclose(neutral["room_into_feedback"], 0.0, abs_tol=1e-12)
    assert math.isclose(neutral["direct_echo"], 1.0, abs_tol=1e-12)

    print("Depthorator V2 depth-law reference: PASS")

if __name__ == "__main__":
    main()
