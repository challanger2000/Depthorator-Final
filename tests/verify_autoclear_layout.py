"""Geometry regression for the Auto Clear panel; no third-party dependencies."""
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "resource" / "Depthorator.uidesc"
CPP = ROOT / "src" / "DepthoratorController.cpp"

def vector(value):
    return tuple(int(v) for v in value.split(","))

def rectangle(element):
    x, y = vector(element.attrib["origin"])
    w, h = vector(element.attrib["size"])
    assert w > 0 and h > 0
    return x, y, x + w, y + h

def collides(a, b):
    return min(a[2], b[2]) > max(a[0], b[0]) and min(a[3], b[3]) > max(a[1], b[1])

def main():
    tree = ET.parse(UI)
    template = tree.getroot().find("./template")
    assert template is not None and vector(template.attrib["size"]) == (1000, 690)
    views = list(template.findall("./view"))
    new = [e for e in views if int(e.attrib["origin"].split(",")[1]) >= 491
           and e.attrib["class"] not in ("CViewContainer",)]
    old = [e for e in views if int(e.attrib["origin"].split(",")[1]) < 491
           and e.attrib["class"] not in ("CViewContainer",)]
    assert len(new) == 12, f"Expected 12 dedicated-panel views, found {len(new)}"

    # Four evenly spaced columns and three full-parameter rotary controls.
    button = [e for e in new if e.get("control-tag") == "AutoClearEnable"]
    assert len(button) == 1 and button[0].get("segment-names") == "OFF,ON"
    knobs = {}
    for field in ("Amount", "Threshold", "Fade"):
        matching = [e for e in new if e.get("custom-view-name") == "DepthKnobAutoClear" + field]
        assert len(matching) == 1 and vector(matching[0].attrib["size"]) == (84, 84)
        assert len([e for e in new if e.get("control-tag") == "AutoClear" + field]) == 1
        knobs[field] = matching[0]
    centers = [sum(rectangle(button[0])[::2]) / 2]
    centers += [sum(rectangle(knobs[name])[::2]) / 2 for name in ("Amount", "Threshold", "Fade")]
    assert all(205 <= centers[i + 1] - centers[i] <= 225 for i in range(3)), centers

    # All content stays within the new panel, with breathing room at edges.
    for element in new:
        x1, y1, x2, y2 = rectangle(element)
        assert 55 <= x1 and x2 <= 945 and 492 <= y1 and y2 <= 667, element.attrib
    # No label, knob, switch or editable value may collide with another one.
    for i, left in enumerate(new):
        for right in new[i + 1:]:
            assert not collides(rectangle(left), rectangle(right)), (
                f"Auto Clear overlap: {left.attrib} vs {right.attrib}")
        for right in old:
            assert not collides(rectangle(left), rectangle(right)), (
                f"Original control obscured by Auto Clear: {left.attrib} vs {right.attrib}")

    source = CPP.read_text(encoding="utf-8")
    assert "CPoint(1000.,690.),VSTGUI::CPoint(1000.,690.)" in source
    for field in ("Amount", "Threshold", "Fade"):
        assert 'DepthKnobAutoClear' + field in source
    assert 'DepthKnobAutoClear' not in source[:source.index('createCustomView')]
    print("AUTO_CLEAR_GUI_GEOMETRY_PASS: four aligned columns, 12 views, no collisions")

if __name__ == "__main__":
    try:
        main()
    except (AssertionError, ET.ParseError, ValueError) as error:
        print("AUTO_CLEAR_GUI_GEOMETRY_FAIL:", error, file=sys.stderr)
        raise
