// engine/gui.h
#pragma once

#include <graphics/mesh.h>
#include <graphics/shader.h>
#include <graphics/window.h>
#include <subject/text.h>
#include <utils/input/input.h>

#include <functional>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>

namespace EditorGUI {

struct Color {
    float r, g, b, a;
    Color() : r(1), g(1), b(1), a(1) {}
    Color(float _r, float _g, float _b, float _a = 1) : r(_r), g(_g), b(_b), a(_a) {}
    
    static Color White() { return Color(1,1,1,1); }
    static Color Black() { return Color(0,0,0,1); }
    static Color Gray() { return Color(0.5f,0.5f,0.5f,1); }
    static Color DarkGray() { return Color(0.2f,0.2f,0.25f,1); }
    static Color LightGray() { return Color(0.3f,0.3f,0.35f,1); }
    static Color Blue() { return Color(0.2f,0.4f,0.8f,1); }
    static Color Red() { return Color(0.8f,0.2f,0.2f,1); }
    static Color Green() { return Color(0.2f,0.8f,0.2f,1); }
    static Color Orange() { return Color(1,0.6f,0,1); }
    static Color TitleBar() { return Color(0.12f,0.12f,0.15f,1); }
    static Color Background() { return Color(0.08f,0.08f,0.1f,0.95f); }
    static Color PanelBg() { return Color(0.1f,0.1f,0.13f,0.92f); }
    static Color Hover() { return Color(0.25f,0.35f,0.55f,1); }
    static Color Active() { return Color(0.3f,0.5f,0.8f,1); }
};

struct Rect {
    float x, y, w, h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class UIRenderer {
private:
    Shader* shader;
    GLuint whiteTexture;
    int screenWidth, screenHeight;
    
    void generateWhiteTexture() {
        unsigned char data[] = {255, 255, 255, 255};
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
public:
    UIRenderer() : shader(nullptr), whiteTexture(0), screenWidth(1200), screenHeight(600) {}
    
    void init(int width, int height) {
        screenWidth = width;
        screenHeight = height;
        
        const char* vertexSource = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            uniform vec2 uScreenSize;
            out vec2 TexCoord;
            void main() {
                vec2 ndc = vec2((aPos.x / uScreenSize.x) * 2.0 - 1.0, 1.0 - (aPos.y / uScreenSize.y) * 2.0);
                gl_Position = vec4(ndc, 0.0, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        
        const char* fragmentSource = R"(
            #version 330 core
            uniform sampler2D uTexture;
            uniform vec4 uColor;
            uniform bool uUseColor;
            in vec2 TexCoord;
            out vec4 FragColor;
            void main() {
                if (uUseColor) {
                    FragColor = uColor;
                } else {
                    FragColor = texture(uTexture, TexCoord);
                }
            }
        )";
        
        shader = new Shader(vertexSource, fragmentSource);
        
        generateWhiteTexture();
    }
    
    ~UIRenderer() {
        delete shader;
        if (whiteTexture) glDeleteTextures(1, &whiteTexture);
    }
    
    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }
    
    void drawRect(float x, float y, float w, float h, const Color& color) {
        shader->use();
        shader->setVec2("uScreenSize", (float)screenWidth, (float)screenHeight);
        shader->setBool("uUseColor", true);
        shader->setVec4("uColor", color.r, color.g, color.b, color.a);
        
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        std::vector<Mesh2D::Vertex> vertices = {
            {x, y, 0, 0},
            {x + w, y, 1, 0},
            {x + w, y + h, 1, 1},
            {x, y + h, 0, 1}
        };
        std::vector<GLuint> indices = {0, 1, 2, 2, 3, 0};
        
        Mesh2D tempMesh;
        tempMesh.setData(vertices, indices);
        tempMesh.draw();
    }
    
    void drawRectOutline(float x, float y, float w, float h, const Color& color, float thickness = 1.0f) {
        drawRect(x, y, w, thickness, color);
        drawRect(x, y + h - thickness, w, thickness, color);
        drawRect(x, y, thickness, h, color);
        drawRect(x + w - thickness, y, thickness, h, color);
    }
    
    void drawText(Text* textObj, float x, float y, const std::string& str, const Color& color) {
        if (!textObj) return;
        textObj->setText(str);
        textObj->setColor(color.r, color.g, color.b, color.a);
        textObj->setPosition(x, y);
        textObj->draw();
    }
};

class UIWidget {
public:
    Rect rect;
    bool visible = true;
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    bool focused = false;
    std::string id;
    std::function<void()> onClick;
    
    UIWidget(const std::string& _id = "") : id(_id) {}
    virtual ~UIWidget() {}
    
    virtual void draw(UIRenderer& renderer, float mx, float my) = 0;
    virtual void update(float dt) {}
    virtual bool onMouseMove(float x, float y) { 
        hovered = rect.contains(x, y); 
        return hovered;
    }
    virtual bool onMouseDown(float x, float y, int button) {
        if (rect.contains(x, y) && enabled) {
            pressed = true;
            focused = true;
            if (button == GLFW_MOUSE_BUTTON_LEFT && onClick) onClick();
            return true;
        }
        return false;
    }
    virtual bool onMouseUp(float x, float y, int button) {
        if (pressed) {
            pressed = false;
            return true;
        }
        return false;
    }
    virtual void onKeyPress(int key, int mods) {}
};

class Label : public UIWidget {
public:
    std::string text;
    Color textColor;
    Text* renderText;
    
    Label(const std::string& _text, const std::string& _id = "", Text* _textObj = nullptr)
        : UIWidget(_id), text(_text), textColor(Color::White()), renderText(_textObj) {}
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible) return;
        onMouseMove(mx, my);
        if (renderText) {
            renderer.drawText(renderText, rect.x + 5, rect.y + 2, text, textColor);
        }
    }
};

class Button : public UIWidget {
public:
    std::string text;
    Color bgColor;
    Color hoverColor;
    Color activeColor;
    Color textColor;
    Text* renderText;
    
    Button(const std::string& _text, const std::string& _id = "", std::function<void()> _onClick = nullptr, Text* _textObj = nullptr)
        : UIWidget(_id), text(_text), bgColor(Color::DarkGray()), hoverColor(Color::Hover()), 
          activeColor(Color::Active()), textColor(Color::White()), renderText(_textObj) {
        onClick = _onClick;
    }
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible) return;
        
        onMouseMove(mx, my);
        
        Color current = bgColor;
        if (pressed) current = activeColor;
        else if (hovered) current = hoverColor;
        
        renderer.drawRect(rect.x, rect.y, rect.w, rect.h, current);
        renderer.drawRectOutline(rect.x, rect.y, rect.w, rect.h, Color(0.4f, 0.4f, 0.5f, 1), 1);
        
        if (renderText) {
            float tw = text.length() * 7;
            renderer.drawText(renderText, rect.x + rect.w/2 - tw/2, rect.y + rect.h/2 - 6, text, textColor);
        }
    }
};

class Checkbox : public UIWidget {
public:
    bool* value;
    std::string label;
    Color boxColor;
    Color checkColor;
    Text* renderText;
    
    Checkbox(bool* _value, const std::string& _label, const std::string& _id = "", Text* _textObj = nullptr)
        : UIWidget(_id), value(_value), label(_label), boxColor(Color::DarkGray()), 
          checkColor(Color::Green()), renderText(_textObj) {}
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible) return;
        onMouseMove(mx, my);
        
        float boxSize = rect.h - 4;
        
        Color current = boxColor;
        if (hovered) current = Color::Hover();
        renderer.drawRect(rect.x, rect.y + 2, boxSize, boxSize, current);
        renderer.drawRectOutline(rect.x, rect.y + 2, boxSize, boxSize, Color(0.5f, 0.5f, 0.6f, 1), 1);
        
        if (*value) {
            renderer.drawRect(rect.x + 3, rect.y + 2 + boxSize/2 - 2, boxSize - 6, 2, checkColor);
            renderer.drawRect(rect.x + boxSize/2 - 2, rect.y + 2 + 3, 2, boxSize - 6, checkColor);
        }
        
        if (renderText) {
            renderer.drawText(renderText, rect.x + boxSize + 5, rect.y + 2, label, Color::White());
        }
    }
    
    bool onMouseDown(float x, float y, int button) {
        if (rect.contains(x, y) && enabled) {
            if (value) *value = !*value;
            pressed = true;
            focused = true;
            if (onClick) onClick();
            return true;
        }
        return false;
    }
};

class Slider : public UIWidget {
public:
    float* value;
    float minVal, maxVal;
    bool dragging;
    Color bgColor;
    Color fillColor;
    Color handleColor;
    
    Slider(float* _value, float _min, float _max, const std::string& _id = "")
        : UIWidget(_id), value(_value), minVal(_min), maxVal(_max), dragging(false),
          bgColor(Color::DarkGray()), fillColor(Color::Blue()), handleColor(Color::LightGray()) {}
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible) return;
        
        onMouseMove(mx, my);
        
        float t = (*value - minVal) / (maxVal - minVal);
        float fillWidth = rect.w * t;
        
        renderer.drawRect(rect.x, rect.y + rect.h/2 - 3, rect.w, 6, bgColor);
        renderer.drawRect(rect.x, rect.y + rect.h/2 - 3, fillWidth, 6, fillColor);
        
        float handleX = rect.x + fillWidth;
        renderer.drawRect(handleX - 5, rect.y, 10, rect.h, handleColor);
        renderer.drawRectOutline(handleX - 5, rect.y, 10, rect.h, Color(0.6f, 0.6f, 0.7f, 1), 1);
    }
    
    bool onMouseDown(float x, float y, int button) {
        if (rect.contains(x, y) && enabled) {
            dragging = true;
            float t = (x - rect.x) / rect.w;
            *value = minVal + t * (maxVal - minVal);
            if (*value < minVal) *value = minVal;
            if (*value > maxVal) *value = maxVal;
            return true;
        }
        return false;
    }
    
    bool onMouseUp(float x, float y, int button) {
        dragging = false;
        return false;
    }
    
    bool onMouseMove(float x, float y) {
        hovered = rect.contains(x, y);
        if (dragging) {
            float t = (x - rect.x) / rect.w;
            *value = minVal + t * (maxVal - minVal);
            if (*value < minVal) *value = minVal;
            if (*value > maxVal) *value = maxVal;
        }
        return hovered;
    }
};

class TextInput : public UIWidget {
public:
    std::string* text;
    std::string placeholder;
    bool focused;
    Color bgColor;
    Color focusColor;
    Text* renderText;
    
    TextInput(std::string* _text, const std::string& _id = "", const std::string& _placeholder = "", Text* _textObj = nullptr)
        : UIWidget(_id), text(_text), placeholder(_placeholder), focused(false),
          bgColor(Color::DarkGray()), focusColor(Color::Blue()), renderText(_textObj) {}
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible) return;
        onMouseMove(mx, my);
        
        Color current = focused ? focusColor : bgColor;
        renderer.drawRect(rect.x, rect.y, rect.w, rect.h, current);
        renderer.drawRectOutline(rect.x, rect.y, rect.w, rect.h, Color(0.5f, 0.5f, 0.6f, 1), 1);
        
        if (renderText) {
            std::string display = text->empty() ? placeholder : *text;
            renderer.drawText(renderText, rect.x + 5, rect.y + rect.h/2 - 6, display, Color::White());
        }
    }
    
    bool onMouseDown(float x, float y, int button) {
        focused = rect.contains(x, y);
        return focused;
    }
    
    void onKeyPress(int key, int mods) {
        if (!focused) return;
        
        if (key == GLFW_KEY_BACKSPACE && !text->empty()) {
            text->pop_back();
        } else if (key == GLFW_KEY_ENTER) {
            focused = false;
        } else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            char c = (mods & GLFW_MOD_SHIFT) ? key : (key - GLFW_KEY_A + 'a');
            *text += c;
        } else if (key == GLFW_KEY_SPACE) {
            *text += ' ';
        } else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            *text += '0' + (key - GLFW_KEY_0);
        }
    }
};

class Panel : public UIWidget {
public:
    std::vector<UIWidget*> children;
    Color bgColor;
    Color titleColor;
    std::string title;
    bool movable;
    bool dragging;
    float dragOffsetX, dragOffsetY;
    bool showTitleBar;
    Text* renderText;
    
    Panel(const std::string& _title, const std::string& _id = "", 
          Color _bgColor = Color::PanelBg(), bool _movable = true)
        : UIWidget(_id), title(_title), bgColor(_bgColor), titleColor(Color::TitleBar()),
          movable(_movable), dragging(false), dragOffsetX(0), dragOffsetY(0),
          showTitleBar(true), renderText(nullptr) {}
    
    ~Panel() {
        for (auto child : children) delete child;
    }
    
    void addWidget(UIWidget* widget) {
        children.push_back(widget);
    }
    
    void setRenderText(Text* text) { renderText = text; }
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible) return;
        
        renderer.drawRect(rect.x, rect.y, rect.w, rect.h, bgColor);
        
        if (showTitleBar) {
            renderer.drawRect(rect.x, rect.y, rect.w, 22, titleColor);
        }
        
        renderer.drawRectOutline(rect.x, rect.y, rect.w, rect.h, Color(0.3f, 0.3f, 0.4f, 1), 1);
        
        if (renderText && showTitleBar) {
            renderer.drawText(renderText, rect.x + 8, rect.y + 3, title, Color::White());
        }
        
        for (auto child : children) {
            child->draw(renderer, mx, my);
        }
    }
    
    void update(float dt) {
        for (auto child : children) child->update(dt);
    }
    
    bool onMouseMove(float x, float y) {
        hovered = rect.contains(x, y);
        for (auto child : children) child->onMouseMove(x, y);
        
        if (dragging && movable && showTitleBar) {
            rect.x = x - dragOffsetX;
            rect.y = y - dragOffsetY;
            
            if (rect.x < 0) rect.x = 0;
            if (rect.y < 22) rect.y = 22;
        }
        
        return hovered;
    }
    
    bool onMouseDown(float x, float y, int button) {
        for (auto child : children) {
            if (child->onMouseDown(x, y, button))
                return true;
        }
        
        if (showTitleBar && Rect(rect.x, rect.y, rect.w, 22).contains(x, y) && movable && button == GLFW_MOUSE_BUTTON_LEFT) {
            dragging = true;
            dragOffsetX = x - rect.x;
            dragOffsetY = y - rect.y;
            return true;
        }
        
        return false;
    }
    
    bool onMouseUp(float x, float y, int button) {
        for (auto child : children) child->onMouseUp(x, y, button);
        dragging = false;
        return false;
    }
    
    void onKeyPress(int key, int mods) {
        for (auto child : children) child->onKeyPress(key, mods);
    }
};

class MenuBar : public UIWidget {
public:
    struct MenuItem {
        std::string name;
        std::function<void()> action;
        std::vector<MenuItem> submenu;
    };
    
    std::vector<MenuItem> items;
    int openMenuIndex;
    Color bgColor;
    Color hoverColor;
    Text* renderText;
    UIRenderer* uiRenderer;
    
    MenuBar(const std::string& _id = "", Text* _textObj = nullptr) 
        : UIWidget(_id), openMenuIndex(-1), bgColor(Color::TitleBar()), 
          hoverColor(Color::Hover()), renderText(_textObj), uiRenderer(nullptr) {}
    
    void setRenderer(UIRenderer* r) { uiRenderer = r; }
    
    void addItem(const std::string& name, std::function<void()> action = nullptr) {
        items.push_back({name, action, {}});
    }
    
    void addSubmenu(const std::string& parent, const std::string& name, std::function<void()> action) {
        for (auto& item : items) {
            if (item.name == parent) {
                item.submenu.push_back({name, action, {}});
                break;
            }
        }
    }
    
    void draw(UIRenderer& renderer, float mx, float my) {
        if (!visible || !uiRenderer) return;
        uiRenderer = &renderer;
        
        renderer.drawRect(rect.x, rect.y, rect.w, rect.h, bgColor);
        
        float xOffset = 10;
        for (size_t i = 0; i < items.size(); i++) {
            float itemW = items[i].name.length() * 7;
            Rect itemRect(rect.x + xOffset, rect.y, itemW + 20, rect.h);
            
            if (itemRect.contains(mx, my) || (int)i == openMenuIndex) {
                renderer.drawRect(itemRect.x, itemRect.y, itemRect.w, itemRect.h, hoverColor);
            }
            
            if (renderText) {
                renderer.drawText(renderText, itemRect.x + 10, itemRect.y + 5, items[i].name, Color::White());
            }
            
            xOffset += itemW + 20;
        }
        
        if (openMenuIndex >= 0 && openMenuIndex < (int)items.size() && !items[openMenuIndex].submenu.empty()) {
            float subX = rect.x + 10;
            for (int i = 0; i < openMenuIndex; i++) {
                subX += items[i].name.length() * 7 + 20;
            }
            
            float maxSubW = 0;
            for (auto& sub : items[openMenuIndex].submenu) {
                float w = sub.name.length() * 7 + 40;
                if (w > maxSubW) maxSubW = w;
            }
            
            renderer.drawRect(subX, rect.y + rect.h, maxSubW, items[openMenuIndex].submenu.size() * 24, Color::PanelBg());
            renderer.drawRectOutline(subX, rect.y + rect.h, maxSubW, items[openMenuIndex].submenu.size() * 24, Color(0.3f, 0.3f, 0.4f, 1), 1);
            
            float yOffset = 0;
            for (auto& sub : items[openMenuIndex].submenu) {
                Rect subRect(subX, rect.y + rect.h + yOffset, maxSubW, 24);
                if (subRect.contains(mx, my)) {
                    renderer.drawRect(subX, rect.y + rect.h + yOffset, maxSubW, 24, hoverColor);
                    if (Input::getInstance().isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && sub.action) {
                        sub.action();
                        openMenuIndex = -1;
                    }
                }
                if (renderText) {
                    renderer.drawText(renderText, subX + 10, rect.y + rect.h + yOffset + 4, sub.name, Color::White());
                }
                yOffset += 24;
            }
        }
    }
    
    bool onMouseMove(float x, float y) {
        hovered = rect.contains(x, y);
        return hovered;
    }
    
    bool onMouseDown(float x, float y, int button) {
        if (!rect.contains(x, y)) {
            openMenuIndex = -1;
            return false;
        }
        
        float xOffset = 10;
        for (size_t i = 0; i < items.size(); i++) {
            float itemW = items[i].name.length() * 7;
            Rect itemRect(rect.x + xOffset, rect.y, itemW + 20, rect.h);
            if (itemRect.contains(x, y)) {
                openMenuIndex = (openMenuIndex == (int)i) ? -1 : (int)i;
                return true;
            }
            xOffset += itemW + 20;
        }
        return false;
    }
};

class EditorGUI {
public:
    Window& window;
    UIRenderer renderer;
    MenuBar* menuBar;
    Panel* hierarchyPanel;
    Panel* inspectorPanel;
    Panel* consolePanel;
    Panel* assetBrowserPanel;
    Panel* luaPanel;
    
    Text* uiText;
    
    bool showLuaPanel;
    bool luaEnabled;
    std::string currentScript;
    int spriteCount, modelCount, textCount;
    float cameraZoom;
    std::vector<std::string> consoleLines;
    
    EditorGUI(Window& win) : window(win), showLuaPanel(true), luaEnabled(true),
        currentScript("script.lua"), spriteCount(0), modelCount(1), textCount(0), cameraZoom(1.0f) {
        uiText = nullptr;
        menuBar = nullptr;
        hierarchyPanel = nullptr;
        inspectorPanel = nullptr;
        consolePanel = nullptr;
        assetBrowserPanel = nullptr;
        luaPanel = nullptr;
    }
    
    ~EditorGUI() {
        delete uiText;
        delete menuBar;
        delete hierarchyPanel;
        delete inspectorPanel;
        delete consolePanel;
        delete assetBrowserPanel;
        delete luaPanel;
    }
    
    void init() {
        renderer.init(window.getWidth(), window.getHeight());
        
        uiText = new Text("./engine/assets/arial.ttf", 13);
        uiText->setScreenSize(window.getWidth(), window.getHeight());
        
        menuBar = new MenuBar("main_menu", uiText);
        menuBar->rect = Rect(0, 0, (float)window.getWidth(), 22);
        menuBar->setRenderer(&renderer);
        
        menuBar->addItem("File");
        menuBar->addSubmenu("File", "New Scene", [this]() { addConsoleLine("> New Scene created"); });
        menuBar->addSubmenu("File", "Open Scene", [this]() { addConsoleLine("> Open Scene dialog"); });
        menuBar->addSubmenu("File", "Save Scene", [this]() { addConsoleLine("> Scene saved"); });
        menuBar->addSubmenu("File", "Exit", [this]() { glfwSetWindowShouldClose(window.getGLFWwindow(), true); });
        
        menuBar->addItem("View");
        menuBar->addSubmenu("View", "Hierarchy", [this]() { if(hierarchyPanel) hierarchyPanel->visible = !hierarchyPanel->visible; });
        menuBar->addSubmenu("View", "Inspector", [this]() { if(inspectorPanel) inspectorPanel->visible = !inspectorPanel->visible; });
        menuBar->addSubmenu("View", "Console", [this]() { if(consolePanel) consolePanel->visible = !consolePanel->visible; });
        menuBar->addSubmenu("View", "Lua Script", [this]() { if(luaPanel) luaPanel->visible = !luaPanel->visible; });
        
        menuBar->addItem("Game Object");
        menuBar->addSubmenu("Game Object", "Create Sprite", [this]() { spriteCount++; addConsoleLine("> Created Sprite"); });
        menuBar->addSubmenu("Game Object", "Create Text", [this]() { textCount++; addConsoleLine("> Created Text"); });
        menuBar->addSubmenu("Game Object", "Create 3D Model", [this]() { modelCount++; addConsoleLine("> Created 3D Model"); });
        
        menuBar->addItem("Tools");
        menuBar->addSubmenu("Tools", "Reload Script", [this]() { addConsoleLine("> Reloading script: " + currentScript); });
        menuBar->addSubmenu("Tools", "Edit Script", [this]() { addConsoleLine("> Opening script editor for: " + currentScript); });
        menuBar->addSubmenu("Tools", "New Script", [this]() { 
            currentScript = "new_script.lua"; 
            addConsoleLine("> Created new script: " + currentScript);
        });
        
        menuBar->addItem("Theme");
        menuBar->addSubmenu("Theme", "Dark", []() {});
        menuBar->addSubmenu("Theme", "Light", []() {});
        menuBar->addSubmenu("Theme", "Blue", []() {});
        
        int w = window.getWidth();
        int h = window.getHeight();
        
        hierarchyPanel = new Panel("Hierarchy", "hierarchy", Color::PanelBg(), true);
        hierarchyPanel->rect = Rect(10, 28, 220, h - 250);
        hierarchyPanel->showTitleBar = true;
        hierarchyPanel->setRenderText(uiText);
        
        inspectorPanel = new Panel("Inspector", "inspector", Color::PanelBg(), true);
        inspectorPanel->rect = Rect(w - 230, 28, 220, h - 250);
        inspectorPanel->showTitleBar = true;
        inspectorPanel->setRenderText(uiText);
        
        consolePanel = new Panel("Console", "console", Color::PanelBg(), true);
        consolePanel->rect = Rect(10, h - 190, w - 20, 180);
        consolePanel->showTitleBar = true;
        consolePanel->setRenderText(uiText);
        
        assetBrowserPanel = new Panel("Asset Browser", "assets", Color::PanelBg(), true);
        assetBrowserPanel->rect = Rect(240, 28, 220, h - 250);
        assetBrowserPanel->showTitleBar = true;
        assetBrowserPanel->setRenderText(uiText);
        
        luaPanel = new Panel("Lua Script", "lua", Color::PanelBg(), true);
        luaPanel->rect = Rect(w - 460, 28, 240, 380);
        luaPanel->showTitleBar = true;
        luaPanel->setRenderText(uiText);
        
        float y = 28;
        
        Checkbox* luaEnabledCheck = new Checkbox(&luaEnabled, "Lua Rendering: Enabled", "lua_enabled", uiText);
        luaEnabledCheck->rect = Rect(10, y, 180, 18);
        luaPanel->addWidget(luaEnabledCheck);
        y += 24;
        
        Label* scriptLabel = new Label("Script: " + currentScript, "script_label", uiText);
        scriptLabel->rect = Rect(10, y, 200, 16);
        luaPanel->addWidget(scriptLabel);
        y += 22;
        
        Label* statusLabel = new Label("Game State: Stopped", "status_label", uiText);
        statusLabel->rect = Rect(10, y, 160, 16);
        luaPanel->addWidget(statusLabel);
        y += 22;
        
        Button* reloadBtn = new Button("Reload", "reload_btn", [this]() { 
            addConsoleLine("> Reloading script " + currentScript);
        }, uiText);
        reloadBtn->rect = Rect(10, y, 70, 26);
        luaPanel->addWidget(reloadBtn);
        
        Button* editBtn = new Button("Edit", "edit_btn", [this]() {
            addConsoleLine("> Opening " + currentScript);
        }, uiText);
        editBtn->rect = Rect(85, y, 70, 26);
        luaPanel->addWidget(editBtn);
        
        Button* newScriptBtn = new Button("New", "new_btn", [this]() {
            currentScript = "new_script.lua";
            addConsoleLine("> Created new script: " + currentScript);
        }, uiText);
        newScriptBtn->rect = Rect(160, y, 70, 26);
        luaPanel->addWidget(newScriptBtn);
        y += 32;
        
        Label* statsLabel = new Label("Scene Statistics:", "stats_title", uiText);
        statsLabel->rect = Rect(10, y, 120, 16);
        luaPanel->addWidget(statsLabel);
        y += 20;
        
        Label* spriteStat = new Label("Sprites: 0", "sprite_stat", uiText);
        spriteStat->rect = Rect(15, y, 100, 16);
        luaPanel->addWidget(spriteStat);
        y += 18;
        
        Label* modelStat = new Label("3D Models: 1", "model_stat", uiText);
        modelStat->rect = Rect(15, y, 100, 16);
        luaPanel->addWidget(modelStat);
        y += 18;
        
        Label* textStat = new Label("Text Objects: 0", "text_stat", uiText);
        textStat->rect = Rect(15, y, 120, 16);
        luaPanel->addWidget(textStat);
        y += 24;
        
        Label* zoomLabel = new Label("Camera Zoom:", "zoom_title", uiText);
        zoomLabel->rect = Rect(10, y, 100, 16);
        luaPanel->addWidget(zoomLabel);
        y += 18;
        
        Slider* zoomSlider = new Slider(&cameraZoom, 0.1f, 3.0f, "zoom_slider");
        zoomSlider->rect = Rect(10, y, 220, 18);
        luaPanel->addWidget(zoomSlider);
    }
    
    void addConsoleLine(const std::string& line) {
        consoleLines.push_back(line);
        if (consoleLines.size() > 15) {
            consoleLines.erase(consoleLines.begin());
        }
        updateConsolePanel();
    }
    
    void updateConsolePanel() {
        if (!consolePanel) return;
        
        for (auto child : consolePanel->children) {
            delete child;
        }
        consolePanel->children.clear();
        
        float yOffset = 25;
        for (size_t i = 0; i < consoleLines.size(); i++) {
            Label* lineLabel = new Label(consoleLines[i], "console_line_" + std::to_string(i), uiText);
            lineLabel->rect = Rect(10, yOffset, consolePanel->rect.w - 20, 14);
            consolePanel->addWidget(lineLabel);
            yOffset += 16;
        }
    }
    
    void updateHierarchyPanel() {
        if (!hierarchyPanel) return;
        
        for (auto child : hierarchyPanel->children) {
            delete child;
        }
        hierarchyPanel->children.clear();
        
        float yOffset = 25;
        
        for (int i = 0; i < spriteCount; i++) {
            Label* spriteLabel = new Label("Sprite_" + std::to_string(i), "sprite_" + std::to_string(i), uiText);
            spriteLabel->rect = Rect(10, yOffset, 180, 16);
            hierarchyPanel->addWidget(spriteLabel);
            yOffset += 18;
        }
        
        for (int i = 0; i < textCount; i++) {
            Label* textLabel = new Label("Text_" + std::to_string(i), "text_" + std::to_string(i), uiText);
            textLabel->rect = Rect(10, yOffset, 180, 16);
            hierarchyPanel->addWidget(textLabel);
            yOffset += 18;
        }
        
        for (int i = 0; i < modelCount; i++) {
            Label* modelLabel = new Label("Model3D_" + std::to_string(i), "model_" + std::to_string(i), uiText);
            modelLabel->rect = Rect(10, yOffset, 180, 16);
            hierarchyPanel->addWidget(modelLabel);
            yOffset += 18;
        }
    }
    
    void updateInspectorPanel() {
        if (!inspectorPanel) return;
        
        for (auto child : inspectorPanel->children) {
            delete child;
        }
        inspectorPanel->children.clear();
        
        float yOffset = 25;
        
        Label* posLabel = new Label("Position", "pos_title", uiText);
        posLabel->rect = Rect(10, yOffset, 60, 16);
        inspectorPanel->addWidget(posLabel);
        yOffset += 20;
        
        TextInput* posX = new TextInput(new std::string("0.0"), "pos_x", "X", uiText);
        posX->rect = Rect(10, yOffset, 70, 20);
        inspectorPanel->addWidget(posX);
        
        TextInput* posY = new TextInput(new std::string("0.0"), "pos_y", "Y", uiText);
        posY->rect = Rect(85, yOffset, 70, 20);
        inspectorPanel->addWidget(posY);
        yOffset += 26;
        
        Label* scaleLabel = new Label("Scale", "scale_title", uiText);
        scaleLabel->rect = Rect(10, yOffset, 60, 16);
        inspectorPanel->addWidget(scaleLabel);
        yOffset += 20;
        
        Slider* scaleSlider = new Slider(&cameraZoom, 0.1f, 3.0f, "scale_slider");
        scaleSlider->rect = Rect(10, yOffset, 180, 18);
        inspectorPanel->addWidget(scaleSlider);
        yOffset += 26;
        
        bool* visibleVal = new bool(true);
        Checkbox* visibleCheck = new Checkbox(visibleVal, "Visible", "visible_check", uiText);
        visibleCheck->rect = Rect(10, yOffset, 120, 18);
        inspectorPanel->addWidget(visibleCheck);
    }
    
    void update(float dt) {
        int w = window.getWidth();
        int h = window.getHeight();
        
        renderer.setScreenSize(w, h);
        if (uiText) uiText->setScreenSize(w, h);
        
        if (menuBar) menuBar->rect = Rect(0, 0, (float)w, 22);
        if (hierarchyPanel) {
            hierarchyPanel->rect.w = 220;
            hierarchyPanel->rect.h = h - 250;
            hierarchyPanel->rect.x = 10;
            hierarchyPanel->rect.y = 28;
        }
        if (inspectorPanel) {
            inspectorPanel->rect.w = 220;
            inspectorPanel->rect.h = h - 250;
            inspectorPanel->rect.x = w - 230;
            inspectorPanel->rect.y = 28;
        }
        if (consolePanel) {
            consolePanel->rect.w = w - 20;
            consolePanel->rect.h = 180;
            consolePanel->rect.x = 10;
            consolePanel->rect.y = h - 190;
        }
        if (assetBrowserPanel) {
            assetBrowserPanel->rect.w = 220;
            assetBrowserPanel->rect.h = h - 250;
            assetBrowserPanel->rect.x = 240;
            assetBrowserPanel->rect.y = 28;
        }
        if (luaPanel) {
            luaPanel->rect.w = 250;
            luaPanel->rect.h = 380;
            luaPanel->rect.x = w - 490;
            luaPanel->rect.y = 28;
        }
        
        updateHierarchyPanel();
        updateInspectorPanel();
        
        float mx = Input::getInstance().getMouseX();
        float my = Input::getInstance().getMouseY();
        
        if (menuBar) {
            menuBar->update(dt);
            menuBar->onMouseMove(mx, my);
        }
        if (hierarchyPanel && hierarchyPanel->visible) {
            hierarchyPanel->update(dt);
            hierarchyPanel->onMouseMove(mx, my);
        }
        if (inspectorPanel && inspectorPanel->visible) {
            inspectorPanel->update(dt);
            inspectorPanel->onMouseMove(mx, my);
        }
        if (consolePanel && consolePanel->visible) {
            consolePanel->update(dt);
            consolePanel->onMouseMove(mx, my);
        }
        if (assetBrowserPanel && assetBrowserPanel->visible) {
            assetBrowserPanel->update(dt);
            assetBrowserPanel->onMouseMove(mx, my);
        }
        if (luaPanel && luaPanel->visible) {
            luaPanel->update(dt);
            luaPanel->onMouseMove(mx, my);
        }
    }
    
    void draw() {
        float mx = Input::getInstance().getMouseX();
        float my = Input::getInstance().getMouseY();
        
        renderer.drawRect(0, 22, (float)window.getWidth(), (float)window.getHeight() - 22, Color::Background());
        
        if (menuBar) menuBar->draw(renderer, mx, my);
        if (hierarchyPanel && hierarchyPanel->visible) hierarchyPanel->draw(renderer, mx, my);
        if (inspectorPanel && inspectorPanel->visible) inspectorPanel->draw(renderer, mx, my);
        if (consolePanel && consolePanel->visible) consolePanel->draw(renderer, mx, my);
        if (assetBrowserPanel && assetBrowserPanel->visible) assetBrowserPanel->draw(renderer, mx, my);
        if (luaPanel && luaPanel->visible) luaPanel->draw(renderer, mx, my);
    }
    
    bool handleMouseMove(float mx, float my) {
        if (menuBar && menuBar->onMouseMove(mx, my)) return true;
        if (hierarchyPanel && hierarchyPanel->visible && hierarchyPanel->onMouseMove(mx, my)) return true;
        if (inspectorPanel && inspectorPanel->visible && inspectorPanel->onMouseMove(mx, my)) return true;
        if (consolePanel && consolePanel->visible && consolePanel->onMouseMove(mx, my)) return true;
        if (assetBrowserPanel && assetBrowserPanel->visible && assetBrowserPanel->onMouseMove(mx, my)) return true;
        if (luaPanel && luaPanel->visible && luaPanel->onMouseMove(mx, my)) return true;
        return false;
    }
    
    bool handleMouseDown(float mx, float my, int button) {
        if (menuBar && menuBar->onMouseDown(mx, my, button)) return true;
        if (hierarchyPanel && hierarchyPanel->visible && hierarchyPanel->onMouseDown(mx, my, button)) return true;
        if (inspectorPanel && inspectorPanel->visible && inspectorPanel->onMouseDown(mx, my, button)) return true;
        if (consolePanel && consolePanel->visible && consolePanel->onMouseDown(mx, my, button)) return true;
        if (assetBrowserPanel && assetBrowserPanel->visible && assetBrowserPanel->onMouseDown(mx, my, button)) return true;
        if (luaPanel && luaPanel->visible && luaPanel->onMouseDown(mx, my, button)) return true;
        return false;
    }
    
    bool handleMouseUp(float mx, float my, int button) {
        if (menuBar && menuBar->onMouseUp(mx, my, button)) return true;
        if (hierarchyPanel && hierarchyPanel->visible && hierarchyPanel->onMouseUp(mx, my, button)) return true;
        if (inspectorPanel && inspectorPanel->visible && inspectorPanel->onMouseUp(mx, my, button)) return true;
        if (consolePanel && consolePanel->visible && consolePanel->onMouseUp(mx, my, button)) return true;
        if (assetBrowserPanel && assetBrowserPanel->visible && assetBrowserPanel->onMouseUp(mx, my, button)) return true;
        if (luaPanel && luaPanel->visible && luaPanel->onMouseUp(mx, my, button)) return true;
        return false;
    }
    
    void handleKeyPress(int key, int mods) {
        if (menuBar) menuBar->onKeyPress(key, mods);
        if (hierarchyPanel && hierarchyPanel->visible) hierarchyPanel->onKeyPress(key, mods);
        if (inspectorPanel && inspectorPanel->visible) inspectorPanel->onKeyPress(key, mods);
        if (consolePanel && consolePanel->visible) consolePanel->onKeyPress(key, mods);
        if (assetBrowserPanel && assetBrowserPanel->visible) assetBrowserPanel->onKeyPress(key, mods);
        if (luaPanel && luaPanel->visible) luaPanel->onKeyPress(key, mods);
    }
};

}