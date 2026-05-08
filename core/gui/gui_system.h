#pragma once

#include <gui/gui_font.h>
#include <gui/gui_widget.h>
#include <utils/math/vector.h>
#include <utils/input/input.h>
#include <glad/glad.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>

namespace Gui {

class GuiSystem {
private:
    std::vector<GuiWidget*> m_widgets;
    std::unordered_map<std::string, GuiWidget*> m_widgetMap;
    GuiFont* m_font;
    Shader* m_quadShader;
    Shader* m_textShader;
    bool m_initialized;
    GuiWidget* m_hovered;
    GuiWidget* m_pressed;
    std::string m_fontPath;
    int m_fontSize;
    bool m_prevMouseDown;
    lua_State* m_lua;
    int m_nextWidgetId;
    int m_screenWidth;
    int m_screenHeight;
    Matrix3x3 m_projectionView;

    Shader* getQuadShader() {
        if (!m_quadShader) {
            const char* vs = R"(
                #version 330 core
                layout (location = 0) in vec2 aPos;
                uniform mat3 uTransform;
                void main() {
                    vec3 transformed = uTransform * vec3(aPos, 1.0);
                    gl_Position = vec4(transformed.xy, 0.0, 1.0);
                }
            )";
            const char* fs = R"(
                #version 330 core
                out vec4 FragColor;
                uniform vec4 uColor;
                void main() {
                    FragColor = uColor;
                }
            )";
            m_quadShader = new Shader(vs, fs);
        }
        return m_quadShader;
    }

    Shader* getTextShader() {
        if (!m_textShader) {
            const char* vs = R"(
                #version 330 core
                layout (location = 0) in vec2 aPos;
                layout (location = 1) in vec2 aTexCoord;
                out vec2 TexCoord;
                uniform mat3 uTransform;
                void main() {
                    vec3 transformed = uTransform * vec3(aPos, 1.0);
                    gl_Position = vec4(transformed.xy, 0.0, 1.0);
                    TexCoord = aTexCoord;
                }
            )";
            const char* fs = R"(
                #version 330 core
                out vec4 FragColor;
                in vec2 TexCoord;
                uniform sampler2D uTexture;
                uniform vec4 uColor;
                void main() {
                    float alpha = texture(uTexture, TexCoord).r;
                    FragColor = vec4(uColor.rgb, uColor.a * alpha);
                }
            )";
            m_textShader = new Shader(vs, fs);
        }
        return m_textShader;
    }

    void callLuaFunc(int ref) {
        if (ref < 0 || !m_lua) return;
        lua_rawgeti(m_lua, LUA_REGISTRYINDEX, ref);
        if (lua_isfunction(m_lua, -1)) {
            if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
                std::cerr << "[GUI Lua] " << lua_tostring(m_lua, -1) << std::endl;
                lua_pop(m_lua, 1);
            }
        } else {
            lua_pop(m_lua, 1);
        }
    }

public:
    GuiSystem()
        : m_font(nullptr), m_quadShader(nullptr), m_textShader(nullptr),
          m_initialized(false),
          m_hovered(nullptr), m_pressed(nullptr),
          m_fontPath("engine/assets/arial.ttf"), m_fontSize(24),
          m_prevMouseDown(false), m_lua(nullptr), m_nextWidgetId(1),
          m_screenWidth(1920), m_screenHeight(1080)
    {}

    ~GuiSystem() {
        clear();
        delete m_font;
        delete m_quadShader;
        delete m_textShader;
    }

    void setLuaState(lua_State* L) { m_lua = L; }
    void setScreenSize(int width, int height) {
        m_screenWidth = width;
        m_screenHeight = height;
    }
    void setProjectionView(const Matrix3x3& pv) { m_projectionView = pv; }
    const Matrix3x3& getProjectionView() const { return m_projectionView; }
    int getScreenWidth() const { return m_screenWidth; }
    int getScreenHeight() const { return m_screenHeight; }
    void setFont(const std::string& path, int size) {
        m_fontPath = path;
        m_fontSize = size;
        if (m_font) {
            delete m_font;
            m_font = nullptr;
        }
    }

    GuiFont* getFont() {
        if (!m_font && !m_fontPath.empty()) {
            m_font = new GuiFont();
            if (!m_font->load(m_fontPath, m_fontSize)) {
                delete m_font;
                m_font = nullptr;
                std::cerr << "[GUI] Failed to load font: " << m_fontPath << std::endl;
            }
        }
        return m_font;
    }

    // ---- Widget creation ----
    GuiPanel* addPanel(const std::string& id, float px, float py, float pw, float ph) {
        GuiPanel* w = new GuiPanel();
        w->id = id; w->x = px; w->y = py; w->w = pw; w->h = ph;
        w->updateModelMatrix();
        m_widgets.push_back(w);
        m_widgetMap[id] = w;
        return w;
    }

    GuiButton* addButton(const std::string& id, float px, float py, float pw, float ph, const std::string& txt) {
        GuiButton* w = new GuiButton();
        w->id = id; w->x = px; w->y = py; w->w = pw; w->h = ph; w->text = txt;
        w->updateModelMatrix();
        m_widgets.push_back(w);
        m_widgetMap[id] = w;
        return w;
    }

    GuiLabel* addLabel(const std::string& id, float px, float py, const std::string& txt) {
        GuiLabel* w = new GuiLabel();
        w->id = id; w->x = px; w->y = py; w->text = txt;
        w->w = getFont() ? m_font->measure(txt, w->fontSize / (float)m_fontSize) + 10 : 100;
        w->updateModelMatrix();
        m_widgets.push_back(w);
        m_widgetMap[id] = w;
        return w;
    }

    GuiSlider* addSlider(const std::string& id, float px, float py, float pw, float minV, float maxV, float val) {
        GuiSlider* w = new GuiSlider();
        w->id = id; w->x = px; w->y = py; w->w = pw;
        w->minVal = minV; w->maxVal = maxV; w->value = val;
        w->updateModelMatrix();
        m_widgets.push_back(w);
        m_widgetMap[id] = w;
        return w;
    }

    GuiCheckbox* addCheckbox(const std::string& id, float px, float py, const std::string& txt, bool chk) {
        GuiCheckbox* w = new GuiCheckbox();
        w->id = id; w->x = px; w->y = py; w->text = txt; w->checked = chk;
        w->updateModelMatrix();
        m_widgets.push_back(w);
        m_widgetMap[id] = w;
        return w;
    }

    // ---- Removal ----
    void remove(const std::string& id) {
        auto it = m_widgetMap.find(id);
        if (it == m_widgetMap.end()) return;
        GuiWidget* w = it->second;
        m_widgetMap.erase(it);
        auto vecIt = std::find(m_widgets.begin(), m_widgets.end(), w);
        if (vecIt != m_widgets.end()) m_widgets.erase(vecIt);
        if (m_hovered == w) m_hovered = nullptr;
        if (m_pressed == w) m_pressed = nullptr;
        if (w->luaOnClickRef >= 0 && m_lua) luaL_unref(m_lua, LUA_REGISTRYINDEX, w->luaOnClickRef);
        if (w->luaOnChangeRef >= 0 && m_lua) luaL_unref(m_lua, LUA_REGISTRYINDEX, w->luaOnChangeRef);
        delete w;
    }

    void clear() {
        m_hovered = nullptr;
        m_pressed = nullptr;
        for (auto& pair : m_widgetMap) {
            GuiWidget* w = pair.second;
            if (w->luaOnClickRef >= 0 && m_lua) luaL_unref(m_lua, LUA_REGISTRYINDEX, w->luaOnClickRef);
            if (w->luaOnChangeRef >= 0 && m_lua) luaL_unref(m_lua, LUA_REGISTRYINDEX, w->luaOnChangeRef);
            delete w;
        }
        m_widgets.clear();
        m_widgetMap.clear();
    }

    GuiWidget* get(const std::string& id) {
        auto it = m_widgetMap.find(id);
        return it != m_widgetMap.end() ? it->second : nullptr;
    }

    GuiWidget* findAt(float wx, float wy) {
        for (int i = (int)m_widgets.size() - 1; i >= 0; i--) {
            GuiWidget* w = m_widgets[i];
            if (w->visible && w->enabled && w->contains(wx, wy))
                return w;
        }
        return nullptr;
    }

    // ---- Update (call once per frame before Lua Draw) ----
    void update() {
        float mx = (float)Input::getInstance().getMouseX();
        float my = (float)Input::getInstance().getMouseY();
        bool down = Input::getInstance().isMouseButtonPressed(0);
        bool justPressed = Input::getInstance().isMouseButtonJustPressed(0);
        bool justReleased = m_prevMouseDown && !down;
        m_prevMouseDown = down;

        m_hovered = findAt(mx, my);

        if (justPressed && m_hovered && m_hovered->enabled)
            m_pressed = m_hovered;

        if (justReleased) {
            if (m_pressed && m_pressed->contains(mx, my)) {
                callLuaFunc(m_pressed->luaOnClickRef);
                if (m_pressed->getType() == GuiWidget::CHECKBOX) {
                    GuiCheckbox* cb = static_cast<GuiCheckbox*>(m_pressed);
                    cb->checked = !cb->checked;
                    callLuaFunc(cb->luaOnChangeRef);
                }
            }
            m_pressed = nullptr;
        }

        if (m_pressed && m_pressed->getType() == GuiWidget::SLIDER) {
            GuiSlider* s = static_cast<GuiSlider*>(m_pressed);
            float norm = (mx - s->x) / s->w;
            if (norm < 0) norm = 0;
            if (norm > 1) norm = 1;
            float oldVal = s->value;
            s->value = s->minVal + norm * (s->maxVal - s->minVal);
            if (s->value != oldVal)
                callLuaFunc(s->luaOnChangeRef);
        }
    }

    // ---- Drawing helpers ----
    void drawRect(const Matrix3x3& transform, float cr, float cg, float cb, float ca) {
        if (ca <= 0) return;
        std::vector<Mesh2D::Vertex> verts = {
            {0, 0, 0, 0},
            {1, 0, 0, 0},
            {1, 1, 0, 0},
            {0, 1, 0, 0}
        };
        std::vector<GLuint> idx = {0, 1, 2, 2, 3, 0};
        Mesh2D mesh;
        mesh.setData(verts, idx);
        Shader* s = getQuadShader();
        s->use();
        s->setVec4("uColor", cr, cg, cb, ca);
        s->setMat3("uTransform", &transform.m[0][0]);
        mesh.draw();
    }

    void drawRect(const Matrix3x3& transform, float x1, float y1, float x2, float y2,
                  float cr, float cg, float cb, float ca)
    {
        if (ca <= 0) return;
        std::vector<Mesh2D::Vertex> verts = {
            {x1, y1, 0, 0},
            {x2, y1, 0, 0},
            {x2, y2, 0, 0},
            {x1, y2, 0, 0}
        };
        std::vector<GLuint> idx = {0, 1, 2, 2, 3, 0};
        Mesh2D mesh;
        mesh.setData(verts, idx);
        Shader* s = getQuadShader();
        s->use();
        s->setVec4("uColor", cr, cg, cb, ca);
        s->setMat3("uTransform", &transform.m[0][0]);
        mesh.draw();
    }

    void drawBorderedRect(const Matrix3x3& transform, float bw, float w, float h,
                          float cr, float cg, float cb, float ca,
                          float br, float bg, float bb, float ba)
    {
        if (bw <= 0 || ba <= 0) {
            drawRect(transform, 0, 0, 1, 1, cr, cg, cb, ca);
            return;
        }
        float bX = bw / w;
        float bY = bw / h;
        drawRect(transform, -bX, -bY, 1 + bX, 0, br, bg, bb, ba);
        drawRect(transform, -bX, 1, 1 + bX, 1 + bY, br, bg, bb, ba);
        drawRect(transform, -bX, 0, 0, 1, br, bg, bb, ba);
        drawRect(transform, 1, 0, 1 + bX, 1, br, bg, bb, ba);
        drawRect(transform, 0, 0, 1, 1, cr, cg, cb, ca);
    }

    void drawText(const std::string& txt, float px, float py, int fontSize,
                  float cr, float cg, float cb, float ca,
                  const Matrix3x3& transform)
    {
        GuiFont* f = getFont();
        if (!f) return;
        float scale = fontSize / (float)m_fontSize;
        f->render(txt, px, py, scale, cr, cg, cb, ca, transform, getTextShader(),
                  m_screenWidth, m_screenHeight);
    }

    float measureText(const std::string& txt, int fontSize) {
        GuiFont* f = getFont();
        if (!f) return 0;
        return f->measure(txt, fontSize / (float)m_fontSize);
    }

    // ---- Draw all widgets (sorted by zOrder) ----
    void draw() {
        if (m_widgets.empty()) return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        std::vector<GuiWidget*> sorted = m_widgets;
        std::sort(sorted.begin(), sorted.end(), [](GuiWidget* a, GuiWidget* b) {
            return a->zOrder < b->zOrder;
        });

        for (GuiWidget* w : sorted) {
            if (w->visible) w->draw(*this);
        }
    }

    bool isHovered(const GuiWidget* w) const { return m_hovered == w; }
    bool isPressed(const GuiWidget* w) const { return m_pressed == w; }

    int refLuaFunc(lua_State* L, int idx) {
        lua_pushvalue(L, idx);
        return luaL_ref(L, LUA_REGISTRYINDEX);
    }

    void unrefLuaFunc(int ref) {
        if (ref >= 0 && m_lua)
            luaL_unref(m_lua, LUA_REGISTRYINDEX, ref);
    }
};

} // namespace Gui
