#pragma once

#include <string>
#include <utils/math/vector.h>

namespace Gui {

class GuiSystem;

class GuiWidget {
public:
    enum Type { PANEL, BUTTON, LABEL, SLIDER, CHECKBOX };

    std::string id;
    float x, y, w, h;
    float r, g, b, a;
    bool visible;
    bool enabled;
    int zOrder;
    int luaOnClickRef;
    int luaOnChangeRef;

    float rotation;
    float originX, originY;
    Matrix3x3 modelMatrix;

    GuiWidget()
        : x(0), y(0), w(100), h(50),
          r(0.2f), g(0.2f), b(0.2f), a(1.0f),
          visible(true), enabled(true), zOrder(0),
          luaOnClickRef(-1), luaOnChangeRef(-1),
          rotation(0), originX(0), originY(0)
    {
        updateModelMatrix();
    }

    virtual ~GuiWidget() {}

    void updateModelMatrix() {
        modelMatrix = Matrix3x3::translation(x, y)
                    * Matrix3x3::rotation(rotation)
                    * Matrix3x3::scale(w, h)
                    * Matrix3x3::translation(-originX * w, -originY * h);
    }

    bool contains(float wx, float wy) const {
        return wx >= x && wx <= x + w && wy >= y && wy <= y + h;
    }

    virtual void draw(GuiSystem& sys) = 0;
    virtual Type getType() const = 0;
};

class GuiPanel : public GuiWidget {
public:
    float br, bg, bb, ba;
    float bw;
    GuiPanel() : br(0.08f), bg(0.08f), bb(0.08f), ba(0.6f), bw(0) {
        r = 0.12f; g = 0.12f; b = 0.12f; a = 0.85f;
        updateModelMatrix();
    }
    void draw(GuiSystem& sys) override;
    Type getType() const override { return PANEL; }
};

class GuiButton : public GuiWidget {
public:
    std::string text;
    int fontSize;
    float tr, tg, tb, ta;
    float hr, hg, hb;
    float pr, pg, pb;
    GuiButton() : fontSize(12), tr(1), tg(1), tb(1), ta(1),
                  hr(0.4f), hg(0.6f), hb(0.9f),
                  pr(0.15f), pg(0.35f), pb(0.65f) {
        r = 0.28f; g = 0.48f; b = 0.78f; a = 1.0f;
        w = 140; h = 36;
        updateModelMatrix();
    }
    void draw(GuiSystem& sys) override;
    Type getType() const override { return BUTTON; }
};

class GuiLabel : public GuiWidget {
public:
    std::string text;
    int fontSize;
    float tr, tg, tb, ta;
    GuiLabel() : fontSize(12), tr(1), tg(1), tb(1), ta(1) {
        r = 0; g = 0; b = 0; a = 0;
        w = 100; h = 24;
        updateModelMatrix();
    }
    void draw(GuiSystem& sys) override;
    Type getType() const override { return LABEL; }
};

class GuiSlider : public GuiWidget {
public:
    float minVal, maxVal, value;
    float handleR, handleG, handleB;
    float trackR, trackG, trackB;
    GuiSlider() : minVal(0), maxVal(100), value(50),
                  handleR(0.4f), handleG(0.7f), handleB(1.0f),
                  trackR(0.25f), trackG(0.25f), trackB(0.25f) {
        r = 0.15f; g = 0.15f; b = 0.15f; a = 0;
        w = 200; h = 24;
        updateModelMatrix();
    }
    void draw(GuiSystem& sys) override;
    Type getType() const override { return SLIDER; }
};

class GuiCheckbox : public GuiWidget {
public:
    std::string text;
    bool checked;
    int fontSize;
    float checkR, checkG, checkB;
    float tr, tg, tb, ta;
    GuiCheckbox() : checked(false), fontSize(12),
                    checkR(0.3f), checkG(0.7f), checkB(1.0f),
                    tr(1), tg(1), tb(1), ta(1) {
        r = 0.2f; g = 0.2f; b = 0.2f; a = 0.8f;
        w = 20; h = 20;
        updateModelMatrix();
    }
    void draw(GuiSystem& sys) override;
    Type getType() const override { return CHECKBOX; }
};

} // namespace Gui
