#pragma once

#include "vstgui/lib/cview.h"

namespace Depthorator {

class DepthoratorLogoView final : public VSTGUI::CView {
public:
    explicit DepthoratorLogoView(const VSTGUI::CRect& size);
    void draw(VSTGUI::CDrawContext* context) override;
};

} // namespace Depthorator
