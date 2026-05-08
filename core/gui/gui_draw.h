#pragma once

#include <gui/gui_system.h>

namespace Gui {

inline void GuiPanel::draw(GuiSystem& sys) {
    const Matrix3x3& pv = sys.getProjectionView();
    Matrix3x3 transform = pv * this->modelMatrix;

    if (bw > 0 && ba > 0) {
        sys.drawBorderedRect(transform, bw, w, h, r, g, b, a, br, bg, bb, ba);
    } else {
        sys.drawRect(transform, r, g, b, a);
    }
}

inline void GuiButton::draw(GuiSystem& sys) {
    const Matrix3x3& pv = sys.getProjectionView();
    Matrix3x3 transform = pv * this->modelMatrix;

    bool isHovered = sys.isHovered(this);
    bool isPressed = sys.isPressed(this);

    float cr = r, cg = g, cb = b;
    if (isPressed) { cr = pr; cg = pg; cb = pb; }
    else if (isHovered && enabled) { cr = hr; cg = hg; cb = hb; }

    float bwLocal = 0.002f;
    sys.drawBorderedRect(transform, bwLocal, w, h, cr, cg, cb, a,
                         0.1f, 0.1f, 0.1f, 0.5f);

    if (!text.empty()) {
        float tw = sys.measureText(text, fontSize);
        float tx = x + (w - tw) / 2.0f;
        float ty = y + (h - fontSize) / 2.0f;
        sys.drawText(text, tx, ty, fontSize, tr, tg, tb, ta, pv);
    }
}

inline void GuiLabel::draw(GuiSystem& sys) {
    if (!text.empty() && a > 0) {
        const Matrix3x3& pv = sys.getProjectionView();
        sys.drawText(text, x, y, fontSize, tr, tg, tb, ta, pv);
    }
}

inline void GuiSlider::draw(GuiSystem& sys) {
    const Matrix3x3& pv = sys.getProjectionView();
    Matrix3x3 transform = pv * this->modelMatrix;

    float trackH = 6;
    float ty = y + (h - trackH) / 2.0f;
    float tnx = x;
    float tny = ty;
    Matrix3x3 trackTransform = pv * Matrix3x3::translation(tnx, tny) * Matrix3x3::scale(w, trackH);
    sys.drawRect(trackTransform, trackR, trackG, trackB, 1.0f);

    float norm = (value - minVal) / (maxVal - minVal);
    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;

    float handleSize = 16;
    float hx = x + norm * w - handleSize / 2.0f;
    float hy = y + (h - handleSize) / 2.0f;

    bool isPressed = sys.isPressed(this);
    float hr = isPressed ? handleR * 0.7f : handleR;
    float hg = isPressed ? handleG * 0.7f : handleG;
    float hb = isPressed ? handleB * 0.7f : handleB;

    Matrix3x3 handleTransform = pv * Matrix3x3::translation(hx, hy) * Matrix3x3::scale(handleSize, handleSize);
    float bwHandle = 0.0015f;
    sys.drawBorderedRect(handleTransform, bwHandle, handleSize, handleSize,
                         hr, hg, hb, 1.0f, 0.2f, 0.2f, 0.2f, 0.8f);
}

inline void GuiCheckbox::draw(GuiSystem& sys) {
    const Matrix3x3& pv = sys.getProjectionView();
    Matrix3x3 transform = pv * this->modelMatrix;

    float bwBox = 0.0015f;
    sys.drawBorderedRect(transform, bwBox, w, h, r, g, b, a,
                         0.3f, 0.3f, 0.3f, 1.0f);

    if (checked) {
        float pad = 3;
        float cx = x + pad;
        float cy = y + pad;
        float cw = w - pad * 2;
        float ch = h - pad * 2;
        Matrix3x3 checkTransform = pv * Matrix3x3::translation(cx, cy) * Matrix3x3::scale(cw, ch);
        sys.drawRect(checkTransform, checkR, checkG, checkB, 1.0f);
    }

    if (!text.empty()) {
        float tx = x + w + 8;
        float ty = y + 2;
        sys.drawText(text, tx, ty, fontSize, tr, tg, tb, ta, pv);
    }
}

} // namespace Gui
