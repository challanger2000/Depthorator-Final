#!/usr/bin/env python3
"""Protect the Depthorator V1 character law used by V1.1.

This is intentionally a coefficient/mapping regression, not a redesign test.
"""
import math

def law(depth, curve, mix, feedback):
    shape = depth ** (2.35 - curve * 1.8)
    cutoff = 18000.0 * (0.18 ** shape) + 900.0 * shape
    return {
        "shape": shape,
        "cutoff": cutoff,
        "repeat_to_room": 0.12 + shape * 0.88,
        "room_output": 0.28 + shape * 0.72,
        "direct_echo": 1.0 - shape * 0.42,
        "room_into_feedback": shape * 0.16,
        "feedback": min(0.94, feedback * 0.94),
        "dry_gain": math.cos(mix * math.pi / 2.0),
        "wet_gain": math.sin(mix * math.pi / 2.0),
    }

def main():
    # Canonical points from the V1 formulas.
    for depth in (0.0, 0.25, 0.5, 0.75, 1.0):
        for curve in (0.0, 0.5, 1.0):
            x = law(depth, curve, 0.5, 0.7)
            shape = depth ** (2.35 - curve * 1.8)
            assert math.isclose(x["shape"], shape, abs_tol=1e-15)
            assert math.isclose(x["repeat_to_room"], 0.12 + shape * 0.88, abs_tol=1e-15)
            assert math.isclose(x["room_output"], 0.28 + shape * 0.72, abs_tol=1e-15)
            assert math.isclose(x["direct_echo"], 1.0 - shape * 0.42, abs_tol=1e-15)
            assert math.isclose(x["room_into_feedback"], shape * 0.16, abs_tol=1e-15)

    x = law(0.5, 0.5, 0.5, 0.7)
    assert math.isclose(x["feedback"], 0.658, abs_tol=1e-15)
    assert math.isclose(x["dry_gain"], math.sqrt(0.5), rel_tol=1e-15)
    assert math.isclose(x["wet_gain"], math.sqrt(0.5), rel_tol=1e-15)

    # Denormal guard threshold is intentionally far below audible signal.
    assert 1.0e-30 < 1.0e-20

    print("Depthorator V1.1 character regression: PASS")

if __name__ == "__main__":
    main()
