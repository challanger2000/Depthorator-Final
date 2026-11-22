"""Auto Clear UI contract: one ON/OFF switch, zero additional knobs."""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "resource" / "Depthorator.uidesc"
CONTROLLER = ROOT / "src" / "DepthoratorController.cpp"
PROCESSOR = ROOT / "src" / "DepthoratorProcessor.cpp"
IDS = ROOT / "src" / "DepthoratorIDs.h"

def rect(element):
    x, y = (int(i) for i in element.attrib["origin"].split(","))
    w, h = (int(i) for i in element.attrib["size"].split(","))
    assert w > 0 and h > 0
    return (x, y, x + w, y + h)

def overlaps(a, b):
    return (min(a[2], b[2]) > max(a[0], b[0]) and
            min(a[3], b[3]) > max(a[1], b[1]))

def main():
    root = ET.parse(UI).getroot()
    template = root.find("./template")
    assert template is not None and template.get("size") == "1000,500"
    views = template.findall("./view")

    # Owner-approved UI contract: there is exactly one Auto Clear control.
    new = [e for e in views if e.get("control-tag") == "AutoClearEnable"]
    labels = [e for e in views if e.get("title") == "AUTO CLEAR"]
    assert len(new) == 1 and len(labels) == 1
    button = new[0]
    label = labels[0]
    assert button.get("class") == "CSegmentButton"
    assert button.get("segment-names") == "OFF,ON"
    assert rect(button) == (76,449,208,476)
    assert rect(label) == (75,430,210,447)
    assert not overlaps(rect(button), rect(label))

    assert len(root.findall("./control-tags/control-tag[@name='AutoClearEnable']")) == 1
    assert not any(e.get("control-tag","").startswith("AutoClear") and e is not button for e in views)
    assert not any(e.get("custom-view-name","").startswith("DepthKnobAutoClear") for e in views)
    for element in views:
        if element is button or element is label or element.get("class") == "CViewContainer":
            continue
        assert not overlaps(rect(button),rect(element)), ("Switch overlap",element.attrib)
        assert not overlaps(rect(label),rect(element)), ("Label overlap",element.attrib)

    controller = CONTROLLER.read_text(encoding="utf-8")
    processor = PROCESSOR.read_text(encoding="utf-8")
    ids = IDS.read_text(encoding="utf-8")
    assert "CPoint(1000.,500.),VSTGUI::CPoint(1000.,500.)" in controller
    for name in ("kClearAmount","kClearThreshold","kClearFade","DepthKnobAutoClear"):
        assert name not in controller and name not in processor and name not in ids, name
    assert 'parameters.addParameter(STR16("Auto Clear")' in controller
    assert 'autoClear_.configure(values_[12]>=0.5,1.0,-30.0,40.0)' in processor
    assert "std::array<double, 13> values_" in (ROOT/"src"/"DepthoratorProcessor.h").read_text()
    print("AUTO_CLEAR_GUI_PASS: exactly 1 switch, 0 added knobs, no overlaps, original 1000x500 size")

if __name__ == "__main__":
    try:
        main()
    except (AssertionError, ET.ParseError) as exc:
        print("AUTO_CLEAR_GUI_FAIL:",exc,file=sys.stderr)
        raise
