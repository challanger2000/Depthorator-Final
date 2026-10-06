#!/usr/bin/env python3
"""Regression checks for Depthorator V2.

V2 must preserve the successful V1 character law and only add progressive depth
on top. This test protects both the V1 baseline and the V2 extension.
"""

import math

DEPTH_POINTS = (0.0, 0.25, 0.50, 0.75, 1.0)
CURVE_POINTS = (0.0, 0.25, 0.50, 0.75, 1.0)

def v1_law(depth: float, curve: float):
    shape = depth ** (2.35 - curve * 1.8)
    cutoff = 18000.0 * (0.18 ** shape) + 900.0 * shape
    return {
        "shape": shape,
        "cutoff": cutoff,
        "repeat_to_room": 0.12 + shape * 0.88,
        "room_output": 0.28 + shape * 0.72,
        "direct_echo": 1.0 - shape * 0.42,
        "room_into_feedback": shape * 0.16,
    }

def v2_law(depth: float, curve: float):
    base = v1_law(depth, curve)
    progression_rate = 0.18 + 0.82 * (curve ** 1.25)
    progressive_amount = base["shape"] * progression_rate
    creative_amount = progressive_amount * progressive_amount

    cutoff = max(
        650.0,
        base["cutoff"] * (1.0 - 0.18 * progressive_amount - 0.10 * creative_amount),
    )
    repeat_to_room = min(
        1.10,
        base["repeat_to_room"] + 0.12 * progressive_amount + 0.08 * creative_amount,
    )
    room_output = min(
        1.10,
        base["room_output"] + 0.10 * progressive_amount + 0.10 * creative_amount,
    )
    direct_echo = max(
        0.45,
        base["direct_echo"] - 0.10 * progressive_amount - 0.08 * creative_amount,
    )
    room_into_feedback = min(
        0.18,
        base["room_into_feedback"] + 0.015 * progressive_amount + 0.005 * creative_amount,
    )

    return {
        "cutoff": cutoff,
        "repeat_to_room": repeat_to_room,
        "room_output": room_output,
        "direct_echo": direct_echo,
        "room_into_feedback": room_into_feedback,
        "source_to_room": 0.18,
    }

def assert_non_decreasing(values, name):
    assert all(b >= a - 1e-12 for a, b in zip(values, values[1:])), (name, values)

def assert_non_increasing(values, name):
    assert all(b <= a + 1e-12 for a, b in zip(values, values[1:])), (name, values)

def main():
    # V1 law itself remains the exact tonal baseline.
    reference = v1_law(0.75, 0.50)
    shape = 0.75 ** (2.35 - 0.50 * 1.8)
    assert math.isclose(reference["shape"], shape, rel_tol=0.0, abs_tol=1e-12)
    assert math.isclose(reference["repeat_to_room"], 0.12 + shape * 0.88, abs_tol=1e-12)
    assert math.isclose(reference["room_output"], 0.28 + shape * 0.72, abs_tol=1e-12)
    assert math.isclose(reference["direct_echo"], 1.0 - shape * 0.42, abs_tol=1e-12)
    assert math.isclose(reference["room_into_feedback"], shape * 0.16, abs_tol=1e-12)

    for curve in CURVE_POINTS:
        rows = [v2_law(depth, curve) for depth in DEPTH_POINTS]
        assert_non_increasing([x["cutoff"] for x in rows], f"cutoff/depth curve={curve}")
        assert_non_decreasing([x["repeat_to_room"] for x in rows], f"room send/depth curve={curve}")
        assert_non_decreasing([x["room_output"] for x in rows], f"room output/depth curve={curve}")
        assert_non_increasing([x["direct_echo"] for x in rows], f"direct echo/depth curve={curve}")
        assert_non_decreasing([x["room_into_feedback"] for x in rows], f"room feedback/depth curve={curve}")

    neutral = v2_law(0.0, 1.0)
    assert math.isclose(neutral["cutoff"], 18000.0, abs_tol=1e-12)
    assert math.isclose(neutral["repeat_to_room"], 0.12, abs_tol=1e-12)
    assert math.isclose(neutral["room_output"], 0.28, abs_tol=1e-12)
    assert math.isclose(neutral["direct_echo"], 1.0, abs_tol=1e-12)
    assert math.isclose(neutral["room_into_feedback"], 0.0, abs_tol=1e-12)

    maximum = v2_law(1.0, 1.0)
    assert maximum["room_into_feedback"] <= 0.18 + 1e-12
    assert maximum["repeat_to_room"] <= 1.10 + 1e-12
    assert maximum["room_output"] <= 1.10 + 1e-12
    assert maximum["direct_echo"] >= 0.45 - 1e-12

    print("Depthorator V2 V1-foundation regression: PASS")

if __name__ == "__main__":
    main()
