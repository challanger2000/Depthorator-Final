#include "DepthoratorKnob.h"

#include "vstgui/lib/cdrawcontext.h"

#include <algorithm>
#include <cmath>

namespace Depthorator {

namespace {
constexpr VSTGUI::CColor kPanel {20, 22, 27, 255};
constexpr VSTGUI::CColor kShadow {0, 0, 0, 120};
constexpr VSTGUI::CColor kWhiteOuter {190, 194, 200, 255};
constexpr VSTGUI::CColor kWhiteInner {240, 241, 243, 255};
constexpr VSTGUI::CColor kMarker {70, 74, 84, 255};
constexpr VSTGUI::CColor kAccent {118, 104, 255, 255};
constexpr double kPi = 3.14159265358979323846;
}

DepthoratorKnob::DepthoratorKnob(const VSTGUI::CRect& size,
                                 VSTGUI::IControlListener* listener,
                                 int32_t tag,
                                 Style style)
: VSTGUI::CKnob(size, listener, tag, nullptr, nullptr), style_(style) {
    setStartAngle(static_cast<float>(135.0 / 180.0 * kPi));
    setRangeAngle(static_cast<float>(270.0 / 180.0 * kPi));
    setWantsFocus(true);
}

void DepthoratorKnob::draw(VSTGUI::CDrawContext* context) {
    auto r = getViewSize();
    const auto cx = r.getCenter().x;
    const auto cy = r.getCenter().y;
    const auto minDim = std::min(r.getWidth(), r.getHeight());
    const bool hero = style_ == Style::Hero;

    context->setDrawMode(VSTGUI::kAntiAliasing);
    context->setFillColor(kPanel);
    context->drawRect(r, VSTGUI::kDrawFilled);

    const auto outerRadius = minDim * 0.46;
    auto shadow = VSTGUI::CRect(cx - outerRadius, cy - outerRadius + (hero ? 6.0 : 4.0),
                               cx + outerRadius, cy + outerRadius + (hero ? 6.0 : 4.0));
    context->setFillColor(kShadow);
    context->drawEllipse(shadow, VSTGUI::kDrawFilled);

    auto outer = VSTGUI::CRect(cx - outerRadius, cy - outerRadius,
                              cx + outerRadius, cy + outerRadius);
    context->setFillColor(kWhiteOuter);
    context->drawEllipse(outer, VSTGUI::kDrawFilled);

    const auto innerRadius = minDim * 0.39;
    auto inner = VSTGUI::CRect(cx - innerRadius, cy - innerRadius - (hero ? 2.5 : 1.5),
                              cx + innerRadius, cy + innerRadius - (hero ? 2.5 : 1.5));
    context->setFillColor(kWhiteInner);
    context->setFrameColor({255, 255, 255, 110});
    context->setLineWidth(hero ? 1.5 : 1.0);
    context->drawEllipse(inner, VSTGUI::kDrawFilledAndStroked);

    const auto arcRadius = minDim * 0.44;
    auto arcRect = VSTGUI::CRect(cx - arcRadius, cy - arcRadius,
                                cx + arcRadius, cy + arcRadius);
    const auto value = std::clamp(static_cast<double>(getValueNormalized()), 0.0, 1.0);
    context->setFrameColor(kAccent);
    context->setLineWidth(hero ? 7.0 : 4.0);
    context->drawArc(arcRect, 135.f, static_cast<float>(135.0 + value * 270.0));

    const auto angle = (135.0 + value * 270.0) * kPi / 180.0;
    const auto markerOuter = minDim * 0.29;
    const auto markerInner = minDim * 0.18;
    VSTGUI::CPoint p1(cx + std::cos(angle) * markerInner,
                      cy + std::sin(angle) * markerInner);
    VSTGUI::CPoint p2(cx + std::cos(angle) * markerOuter,
                      cy + std::sin(angle) * markerOuter);
    context->setFrameColor(kMarker);
    context->setLineWidth(hero ? 4.5 : 3.0);
    context->drawLine(p1, p2);

    setDirty(false);
}

} // namespace Depthorator
