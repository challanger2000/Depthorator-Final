#pragma once

#include "vstgui/lib/controls/cknob.h"

#include <cstdint>

namespace Depthorator {

class DepthoratorKnob final : public VSTGUI::CKnob {
public:
    enum class Style { Small, Hero };

    DepthoratorKnob(const VSTGUI::CRect& size,
                    VSTGUI::IControlListener* listener,
                    int32_t tag,
                    Style style);

    void draw(VSTGUI::CDrawContext* context) override;

private:
    Style style_ {Style::Small};
};

} // namespace Depthorator
