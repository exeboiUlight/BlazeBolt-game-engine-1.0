#pragma once

#include <graphics/window.h>
#include <graphics/mesh.h>
#include <graphics/shader.h>
#include <subject/text.h>
#include <utils/input/input.h>
#include <utils/math/vector.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <array>
#include <set>
#include <cctype>
#include <functional>
#include <iostream>  // добавлено для std::cout/cerr (на случай, если window.h не включает)

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace fs = std::filesystem;

namespace EditorGUI {   // <-- оборачиваем всё в пространство имён

// -------------------------------------------------------------------
// Цветовые константы (Vector4)
// -------------------------------------------------------------------
const Vector4 COLOR_BG_DARK(0.18f, 0.18f, 0.22f, 1.0f);
const Vector4 COLOR_PANEL(0.22f, 0.22f, 0.26f, 1.0f);
const Vector4 COLOR_ACCENT(0.85f, 0.35f, 0.35f, 1.0f);
const Vector4 COLOR_TEXT(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4 COLOR_TEXT_DIM(0.6f, 0.6f, 0.7f, 1.0f);
const Vector4 COLOR_BORDER(0.3f, 0.3f, 0.35f, 1.0f);
const Vector4 COLOR_KEYWORD(0.85f, 0.50f, 0.85f, 1.0f);
const Vector4 COLOR_STRING(0.60f, 0.90f, 0.60f, 1.0f);
const Vector4 COLOR_COMMENT(0.55f, 0.55f, 0.60f, 1.0f);
const Vector4 COLOR_NUMBER(0.80f, 0.80f, 0.40f, 1.0f);
const Vector4 COLOR_FUNCTION(0.50f, 0.70f, 0.90f, 1.0f);
const Vector4 COLOR_PREPROC(0.70f, 0.55f, 0.40f, 1.0f);

// -------------------------------------------------------------------
// Простой цветной прямоугольник (рисуется через Mesh2D)
// -------------------------------------------------------------------
class Rectangle {
public:
    Rectangle() : mesh(nullptr), color(1,1,1,1) {}
    ~Rectangle() { delete mesh; }

    void setColor(const Vector4& col) { color = col; }
    void setRect(float x, float y, float w, float h) {
        if (mesh) delete mesh;
        mesh = new Mesh2D();
        float x1 = x, x2 = x + w;
        float y1 = y, y2 = y + h;
        std::vector<Mesh2D::Vertex> vertices = {
            {x1, y1, 0,0}, {x2, y1, 1,0}, {x2, y2, 1,1}, {x1, y2, 0,1}
        };
        std::vector<GLuint> indices = {0,1,2, 2,3,0};
        mesh->setData(vertices, indices);
    }

    void draw() const {
        if (!mesh) return;
        static Shader* colorShader = nullptr;
        if (!colorShader) {
            const char* vs = "#version 330 core\nlayout(location=0) in vec2 aPos;\nvoid main(){gl_Position=vec4(aPos,0,1);}\n";
            const char* fs = "#version 330 core\nuniform vec4 uColor;\nout vec4 FragColor;\nvoid main(){FragColor=uColor;}\n";
            colorShader = new Shader(vs, fs);
        }
        colorShader->use();
        colorShader->setVec4("uColor", color.x, color.y, color.z, color.w);
        mesh->draw();
    }

private:
    Mesh2D* mesh;
    Vector4 color;
};

// -------------------------------------------------------------------
// Сообщение консоли (с конструктором для удобной инициализации)
// -------------------------------------------------------------------
struct ConsoleMsg {
    enum Type { Info, Warning, Error, Success };
    Type type;
    std::string text;
    std::string timestamp;

    ConsoleMsg() : type(Info), text(""), timestamp("") {}
    ConsoleMsg(const std::string& t, Type tp, const std::string& ts)
        : type(tp), text(t), timestamp(ts) {}
};

// -------------------------------------------------------------------
// Редактор кода с подсветкой синтаксиса
// -------------------------------------------------------------------
class CodeEditor {
public:
    CodeEditor() : font(nullptr), visible(true), cursorX(0), cursorY(0), 
                   selectStart{0,0}, selectEnd{0,0}, scrollOffset(0), 
                   lineHeight(24), charWidth(10), modified(false), 
                   baseX(10), baseY(10), undoPos(0), mouseInside(false) {
        loadFont();
        lines.push_back("");
        updateCursorRect();
    }

    ~CodeEditor() { delete font; }

    void loadFile(const std::string& path) {
        currentPath = path;
        std::ifstream file(path);
        if (!file.is_open()) return;
        lines.clear();
        std::string line;
        while (std::getline(file, line)) lines.push_back(line);
        if (lines.empty()) lines.push_back("");
        modified = false;
        cursorX = cursorY = 0;
        selectStart = selectEnd = {0,0};
        scrollOffset = 0;
        language = detectLanguage(path);
        updateCursorRect();
    }

    bool saveFile() {
        if (currentPath.empty()) return false;
        std::ofstream file(currentPath);
        if (!file.is_open()) return false;
        for (size_t i=0; i<lines.size(); ++i) {
            file << lines[i];
            if (i+1 < lines.size()) file << '\n';
        }
        modified = false;
        return true;
    }

    void newFile() {
        lines.clear();
        lines.push_back("");
        currentPath.clear();
        modified = false;
        cursorX = cursorY = 0;
        selectStart = selectEnd = {0,0};
        scrollOffset = 0;
        language = "lua";
    }

    void setBounds(float _x, float _y, float _w, float _h) {
        x = _x; y = _y; w = _w; h = _h;
    }

    void update(float dt, const Input& input) {
        if (!visible) return;
        mouseInside = (input.getMouseX() >= x && input.getMouseX() <= x+w &&
                       input.getMouseY() >= y && input.getMouseY() <= y+h);
        if (mouseInside && input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            float mx = (float)input.getMouseX() - x - baseX;
            float my = (float)input.getMouseY() - y - baseY;
            int line = (int)(my / lineHeight) + scrollOffset;
            if (line < 0) line = 0;
            if (line >= (int)lines.size()) line = (int)lines.size()-1;
            int col = (int)(mx / charWidth);
            if (col < 0) col = 0;
            if (col > (int)lines[line].size()) col = (int)lines[line].size();
            setCursor(line, col);
            selectStart = {cursorY, cursorX};
            selectEnd = selectStart;
        }
        if (mouseInside && input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            float mx = (float)input.getMouseX() - x - baseX;
            float my = (float)input.getMouseY() - y - baseY;
            int line = (int)(my / lineHeight) + scrollOffset;
            if (line < 0) line = 0;
            if (line >= (int)lines.size()) line = (int)lines.size()-1;
            int col = (int)(mx / charWidth);
            if (col < 0) col = 0;
            if (col > (int)lines[line].size()) col = (int)lines[line].size();
            selectEnd = {line, col};
        }

        bool ctrl = input.isKeyPressed(GLFW_KEY_LEFT_CONTROL) || input.isKeyPressed(GLFW_KEY_RIGHT_CONTROL);
        bool shift = input.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || input.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_S)) { saveFile(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_N)) { newFile(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_Z) && !shift) { undo(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_Y)) { redo(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_X)) { cut(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_C)) { copy(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_V)) { paste(); return; }
        if (ctrl && input.isKeyJustPressed(GLFW_KEY_A)) { selectAll(); return; }

        if (input.isKeyJustPressed(GLFW_KEY_DELETE)) { deleteForward(); return; }
        if (input.isKeyJustPressed(GLFW_KEY_BACKSPACE)) { deleteBackward(); return; }
        if (input.isKeyJustPressed(GLFW_KEY_ENTER) || input.isKeyJustPressed(GLFW_KEY_KP_ENTER)) { insertNewline(); return; }
        if (input.isKeyJustPressed(GLFW_KEY_TAB)) { insertTab(); return; }

        if (input.isKeyJustPressed(GLFW_KEY_LEFT)) moveCursor(-1, 0, shift);
        if (input.isKeyJustPressed(GLFW_KEY_RIGHT)) moveCursor(1, 0, shift);
        if (input.isKeyJustPressed(GLFW_KEY_UP)) moveCursor(0, -1, shift);
        if (input.isKeyJustPressed(GLFW_KEY_DOWN)) moveCursor(0, 1, shift);
        if (input.isKeyJustPressed(GLFW_KEY_HOME)) moveCursorToLineStart(shift);
        if (input.isKeyJustPressed(GLFW_KEY_END)) moveCursorToLineEnd(shift);
        if (input.isKeyJustPressed(GLFW_KEY_PAGE_UP)) moveCursorPage(-(int)(h/lineHeight), shift);
        if (input.isKeyJustPressed(GLFW_KEY_PAGE_DOWN)) moveCursorPage((int)(h/lineHeight), shift);

        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_Z; ++key) {
            if (input.isKeyJustPressed(key)) {
                char ch = keyToChar(key, shift);
                if (ch) insertChar(ch);
            }
        }
        for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key) {
            if (input.isKeyJustPressed(key)) {
                char ch = keyToChar(key, shift);
                if (ch) insertChar(ch);
            }
        }
        static const std::pair<int, char> punct[] = {
            {GLFW_KEY_PERIOD, '.'}, {GLFW_KEY_COMMA, ','}, {GLFW_KEY_SLASH, '/'},
            {GLFW_KEY_SEMICOLON, ';'}, {GLFW_KEY_APOSTROPHE, '\''}, {GLFW_KEY_GRAVE_ACCENT, '`'},
            {GLFW_KEY_MINUS, '-'}, {GLFW_KEY_EQUAL, '='}, {GLFW_KEY_LEFT_BRACKET, '['},
            {GLFW_KEY_RIGHT_BRACKET, ']'}, {GLFW_KEY_BACKSLASH, '\\'}
        };
        for (auto& p : punct) {
            if (input.isKeyJustPressed(p.first)) {
                char ch = p.second;
                if (shift) {
                    switch (ch) {
                        case '.': ch='>'; break; case ',': ch='<'; break; case '/': ch='?'; break;
                        case ';': ch=':'; break; case '\'': ch='"'; break; case '`': ch='~'; break;
                        case '-': ch='_'; break; case '=': ch='+'; break; case '[': ch='{'; break;
                        case ']': ch='}'; break; case '\\': ch='|'; break;
                    }
                }
                insertChar(ch);
            }
        }
    }

    void draw() const {
        if (!visible) return;
        // Фон редактора
        Rectangle bg;
        bg.setColor(Vector4(0.15f,0.15f,0.18f,1.0f));
        bg.setRect(x, y, w, h);
        bg.draw();

        // Номера строк
        float lineNumW = 40;
        Rectangle lineNumBg;
        lineNumBg.setColor(Vector4(0.12f,0.12f,0.15f,1.0f));
        lineNumBg.setRect(x, y, lineNumW, h);
        lineNumBg.draw();

        // Временный объект Text для отрисовки (используем один и тот же)
        // Путь к шрифту – зададим явно
        static const std::string fontPath = "engine/fonts/arial.ttf";
        Text textDrawer(fontPath, 20);
        textDrawer.setScreenSize(1600, 900); // будет обновлён в update размеров окна

        float startY = y + baseY;
        int startLine = scrollOffset;
        int endLine = std::min((int)lines.size(), startLine + (int)(h/lineHeight) + 1);
        for (int i = startLine; i < endLine; ++i) {
            float lineY = startY + (i - startLine) * lineHeight;
            // Номер строки
            textDrawer.setPosition(x+5, lineY+2);
            textDrawer.setText(std::to_string(i+1));
            textDrawer.setColor(COLOR_TEXT_DIM.x, COLOR_TEXT_DIM.y, COLOR_TEXT_DIM.z, COLOR_TEXT_DIM.w);
            textDrawer.draw();
            // Текст строки с подсветкой
            drawHighlightedLine(lines[i], x + lineNumW + 2, lineY, textDrawer);
        }

        // Курсор
        if (visible && mouseInside) {
            float cursorScreenX = x + baseX + lineNumW + 2 + cursorX * charWidth;
            float cursorScreenY = y + baseY + (cursorY - scrollOffset) * lineHeight;
            Rectangle cursorRect;
            cursorRect.setColor(Vector4(1,1,1,0.7f));
            cursorRect.setRect(cursorScreenX, cursorScreenY, 2, lineHeight-4);
            cursorRect.draw();
        }
    }

    bool isModified() const { return modified; }
    const std::string& getCurrentPath() const { return currentPath; }
    bool wantsKeyboard() const { return mouseInside; }

private:
    struct CursorPos { int line, col; };
    struct UndoState {
        std::vector<std::string> lines;
        CursorPos cursor;
        CursorPos selStart, selEnd;
    };
    std::vector<UndoState> undoStack;
    int undoPos;

    Text* font;
    std::vector<std::string> lines;
    std::string currentPath;
    std::string language;
    bool visible, modified;
    int cursorX, cursorY;
    CursorPos selectStart, selectEnd;
    int scrollOffset;
    float x,y,w,h;
    float baseX, baseY, lineHeight, charWidth;
    bool mouseInside;

    void loadFont() {
        font = new Text("engine/fonts/arial.ttf", 20);
        font->setScreenSize(1600,900);
    }

    std::string detectLanguage(const std::string& path) {
        auto endsWith = [](const std::string& str, const std::string& suf) {
            if (str.length() < suf.length()) return false;
            return str.compare(str.length()-suf.length(), suf.length(), suf) == 0;
        };
        if (endsWith(path, ".lua")) return "lua";
        if (endsWith(path, ".cpp") || endsWith(path, ".h") || endsWith(path, ".hpp")) return "cpp";
        if (endsWith(path, ".glsl") || endsWith(path, ".vert") || endsWith(path, ".frag")) return "glsl";
        return "lua";
    }

    void drawHighlightedLine(const std::string& line, float startX, float startY, Text& textDrawer) const {
        struct Token { std::string text; Vector4 color; };
        std::vector<Token> tokens;
        if (language == "lua") {
            std::vector<std::pair<std::string, Vector4>> tmp;
            tokenizeLua(line, tmp);
            for (auto& p : tmp) tokens.push_back({p.first, p.second});
        } else {
            tokens.push_back({line, COLOR_TEXT});
        }

        float curX = startX;
        for (const auto& tok : tokens) {
            textDrawer.setPosition(curX, startY+2);
            textDrawer.setText(tok.text);
            textDrawer.setColor(tok.color.x, tok.color.y, tok.color.z, tok.color.w);
            textDrawer.draw();
            curX += tok.text.size() * charWidth;
        }
    }

    void tokenizeLua(const std::string& line, std::vector<std::pair<std::string, Vector4>>& out) const {
        static const std::set<std::string> keywords = {
            "and","break","do","else","elseif","end","false","for","function","goto","if","in","local",
            "nil","not","or","repeat","return","then","true","until","while"
        };
        size_t i=0;
        while (i<line.size()) {
            if (isspace(line[i])) {
                size_t j=i; while(j<line.size() && isspace(line[j])) j++;
                out.emplace_back(line.substr(i,j-i), COLOR_TEXT);
                i=j; continue;
            }
            if (line[i]=='-' && i+1<line.size() && line[i+1]=='-') {
                out.emplace_back(line.substr(i), COLOR_COMMENT);
                break;
            }
            if (line[i]=='"' || line[i]=='\'') {
                char q=line[i]; size_t j=i+1;
                while(j<line.size() && line[j]!=q) { if(line[j]=='\\') j++; j++; }
                if(j<line.size()) j++;
                out.emplace_back(line.substr(i,j-i), COLOR_STRING);
                i=j; continue;
            }
            if (isdigit(line[i]) || (line[i]=='.' && i+1<line.size() && isdigit(line[i+1]))) {
                size_t j=i; while(j<line.size() && (isdigit(line[j]) || line[j]=='.')) j++;
                out.emplace_back(line.substr(i,j-i), COLOR_NUMBER);
                i=j; continue;
            }
            if (isalpha(line[i]) || line[i]=='_') {
                size_t j=i; while(j<line.size() && (isalnum(line[j]) || line[j]=='_')) j++;
                std::string word = line.substr(i,j-i);
                if (keywords.count(word)) out.emplace_back(word, COLOR_KEYWORD);
                else if (word.find("function")!=std::string::npos) out.emplace_back(word, COLOR_FUNCTION);
                else out.emplace_back(word, COLOR_TEXT);
                i=j; continue;
            }
            out.emplace_back(line.substr(i,1), COLOR_TEXT);
            i++;
        }
    }

    void setCursor(int line, int col) {
        if (line<0) line=0; if(line>=(int)lines.size()) line=lines.size()-1;
        if(col<0) col=0; if(col>(int)lines[line].size()) col=lines[line].size();
        cursorY=line; cursorX=col;
        if (cursorY < scrollOffset) scrollOffset = cursorY;
        if (cursorY >= scrollOffset + (int)(h/lineHeight)) scrollOffset = cursorY - (int)(h/lineHeight) + 1;
    }

    void moveCursor(int dx, int dy, bool extend) {
        int newLine = cursorY + dy;
        int newCol = cursorX + dx;
        if (newLine<0) newLine=0; if(newLine>=(int)lines.size()) newLine=lines.size()-1;
        if (newCol<0) newCol=0; if(newCol>(int)lines[newLine].size()) newCol=lines[newLine].size();
        if (!extend) selectStart = selectEnd = {newLine, newCol};
        else selectEnd = {newLine, newCol};
        setCursor(newLine, newCol);
    }
    void moveCursorToLineStart(bool extend) { moveCursor(-cursorX, 0, extend); }
    void moveCursorToLineEnd(bool extend)   { moveCursor(lines[cursorY].size()-cursorX, 0, extend); }
    void moveCursorPage(int delta, bool extend) { moveCursor(0, delta, extend); }

    void insertChar(char ch) {
        recordUndo();
        if (hasSelection()) deleteSelection();
        lines[cursorY].insert(cursorX, 1, ch);
        cursorX++;
        modified = true;
    }
    void insertNewline() {
        recordUndo();
        if (hasSelection()) deleteSelection();
        std::string rest = lines[cursorY].substr(cursorX);
        lines[cursorY] = lines[cursorY].substr(0, cursorX);
        lines.insert(lines.begin()+cursorY+1, rest);
        cursorX = 0;
        cursorY++;
        modified = true;
    }
    void insertTab() { insertChar('\t'); }
    void deleteBackward() {
        if (hasSelection()) { deleteSelection(); return; }
        if (cursorX==0 && cursorY>0) {
            recordUndo();
            int prevLen = lines[cursorY-1].size();
            lines[cursorY-1] += lines[cursorY];
            lines.erase(lines.begin()+cursorY);
            cursorY--;
            cursorX = prevLen;
            modified = true;
        } else if (cursorX>0) {
            recordUndo();
            lines[cursorY].erase(cursorX-1, 1);
            cursorX--;
            modified = true;
        }
    }
    void deleteForward() {
        if (hasSelection()) { deleteSelection(); return; }
        if (cursorX < (int)lines[cursorY].size()) {
            recordUndo();
            lines[cursorY].erase(cursorX, 1);
            modified = true;
        } else if (cursorY+1 < (int)lines.size()) {
            recordUndo();
            lines[cursorY] += lines[cursorY+1];
            lines.erase(lines.begin()+cursorY+1);
            modified = true;
        }
    }
    bool hasSelection() const { return !(selectStart.line==selectEnd.line && selectStart.col==selectEnd.col); }
    void deleteSelection() {
        if (!hasSelection()) return;
        recordUndo();
        auto [start, end] = normalizeSelection();
        if (start.line == end.line) {
            lines[start.line].erase(start.col, end.col-start.col);
            cursorY=start.line; cursorX=start.col;
        } else {
            std::string newLine = lines[start.line].substr(0, start.col) + lines[end.line].substr(end.col);
            lines.erase(lines.begin()+start.line+1, lines.begin()+end.line+1);
            lines[start.line] = newLine;
            cursorY=start.line; cursorX=start.col;
        }
        selectStart = selectEnd = {cursorY, cursorX};
        modified = true;
    }
    std::pair<CursorPos,CursorPos> normalizeSelection() const {
        if (selectStart.line < selectEnd.line) return {selectStart, selectEnd};
        if (selectStart.line > selectEnd.line) return {selectEnd, selectStart};
        if (selectStart.col <= selectEnd.col) return {selectStart, selectEnd};
        return {selectEnd, selectStart};
    }
    void copy() {
        if (!hasSelection()) return;
        auto [start, end] = normalizeSelection();
        std::string clip;
        if (start.line == end.line) clip = lines[start.line].substr(start.col, end.col-start.col);
        else {
            clip = lines[start.line].substr(start.col) + "\n";
            for (int l=start.line+1; l<end.line; ++l) clip += lines[l] + "\n";
            clip += lines[end.line].substr(0, end.col);
        }
        glfwSetClipboardString(nullptr, clip.c_str());
    }
    void cut() { if (hasSelection()) { copy(); deleteSelection(); } }
    void paste() {
        const char* clip = glfwGetClipboardString(nullptr);
        if (!clip) return;
        std::string str = clip;
        recordUndo();
        if (hasSelection()) deleteSelection();
        std::vector<std::string> pastedLines;
        std::stringstream ss(str);
        std::string line;
        while (std::getline(ss, line)) pastedLines.push_back(line);
        if (pastedLines.empty()) return;
        if (pastedLines.size() == 1) {
            lines[cursorY].insert(cursorX, pastedLines[0]);
            cursorX += pastedLines[0].size();
        } else {
            std::string firstPart = lines[cursorY].substr(0, cursorX);
            std::string restPart = lines[cursorY].substr(cursorX);
            lines[cursorY] = firstPart + pastedLines[0];
            for (size_t i=1; i<pastedLines.size()-1; ++i)
                lines.insert(lines.begin()+cursorY+i, pastedLines[i]);
            lines.insert(lines.begin()+cursorY+pastedLines.size()-1, pastedLines.back() + restPart);
            cursorY += pastedLines.size()-1;
            cursorX = pastedLines.back().size();
        }
        modified = true;
    }
    void selectAll() {
        selectStart = {0,0};
        selectEnd = {(int)lines.size()-1, (int)lines.back().size()};
        setCursor(selectEnd.line, selectEnd.col);
    }
    void recordUndo() {
        while ((int)undoStack.size() > 50) undoStack.erase(undoStack.begin());
        undoStack.resize(undoPos);
        UndoState st;
        st.lines = lines;
        st.cursor = {cursorY, cursorX};
        st.selStart = selectStart;
        st.selEnd = selectEnd;
        undoStack.push_back(st);
        undoPos = undoStack.size();
    }
    void undo() {
        if (undoPos>0 && undoPos-1 < (int)undoStack.size()) {
            undoPos--;
            const auto& st = undoStack[undoPos];
            lines = st.lines;
            cursorY = st.cursor.line; cursorX = st.cursor.col;
            selectStart = st.selStart; selectEnd = st.selEnd;
            modified = true;
        }
    }
    void redo() {
        if (undoPos+1 < (int)undoStack.size()) {
            undoPos++;
            const auto& st = undoStack[undoPos];
            lines = st.lines;
            cursorY = st.cursor.line; cursorX = st.cursor.col;
            selectStart = st.selStart; selectEnd = st.selEnd;
            modified = true;
        }
    }
    void updateCursorRect() {}
    char keyToChar(int key, bool shift) {
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            char c = 'a' + (key - GLFW_KEY_A);
            if (shift) c = toupper(c);
            return c;
        }
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            if (shift) {
                const char* shifted = ")!@#$%^&*(";
                return shifted[key - GLFW_KEY_0];
            }
            return '0' + (key - GLFW_KEY_0);
        }
        if (key == GLFW_KEY_SPACE) return ' ';
        return 0;
    }
};

// -------------------------------------------------------------------
// Файловый менеджер
// -------------------------------------------------------------------
class FileManager {
public:
    FileManager() : selectedIndex(-1), scrollOffset(0), itemHeight(24), mouseInside(false) {
        setRoot(".");
    }

    void setRoot(const std::string& root) {
        currentPath = fs::absolute(root);
        refresh();
    }

    void setBounds(float _x, float _y, float _w, float _h) {
        x = _x; y = _y; w = _w; h = _h;
    }

    void update(const Input& input) {
        mouseInside = (input.getMouseX() >= x && input.getMouseX() <= x+w &&
                       input.getMouseY() >= y && input.getMouseY() <= y+h);
        if (mouseInside && input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            float my = (float)input.getMouseY() - y - 30;
            int idx = (int)(my / itemHeight) + scrollOffset;
            if (idx >=0 && idx < (int)items.size()) {
                selectedIndex = idx;
                if (std::chrono::steady_clock::now() - lastClickTime < std::chrono::milliseconds(500) &&
                    selectedIndex == lastClickedIndex) {
                    if (items[selectedIndex].isDirectory)
                        navigateInto(items[selectedIndex].path);
                    else if (onFileActivate)
                        onFileActivate(items[selectedIndex].path.string());
                    lastClickTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(1000);
                } else {
                    lastClickTime = std::chrono::steady_clock::now();
                    lastClickedIndex = selectedIndex;
                }
            }
        }
        if (mouseInside) {
            double scroll = input.getScrollY();
            if (scroll != 0) {
                scrollOffset -= (int)scroll;
                if (scrollOffset < 0) scrollOffset = 0;
                if (scrollOffset > (int)items.size() - (int)(h/itemHeight))
                    scrollOffset = std::max(0, (int)items.size() - (int)(h/itemHeight));
            }
        }
    }

    void draw() const {
        static const std::string fontPath = "engine/fonts/arial.ttf";

        Rectangle bg; bg.setColor(COLOR_PANEL); bg.setRect(x,y,w,h); bg.draw();
        Rectangle titleBg; titleBg.setColor(Vector4(0.12f,0.12f,0.15f,1)); titleBg.setRect(x,y,w,30); titleBg.draw();
        Text title(fontPath, 18); title.setScreenSize(1600,900);
        title.setPosition(x+5, y+5); title.setText("File Explorer"); title.setColor(COLOR_TEXT.x,COLOR_TEXT.y,COLOR_TEXT.z,1);
        title.draw();

        Text upBtn(fontPath, 16); upBtn.setScreenSize(1600,900);
        upBtn.setPosition(x+w-30, y+5); upBtn.setText("⬆"); upBtn.setColor(COLOR_ACCENT.x,COLOR_ACCENT.y,COLOR_ACCENT.z,1);
        upBtn.draw();

        int startIdx = scrollOffset;
        int endIdx = std::min((int)items.size(), startIdx + (int)(h/itemHeight));
        Text lineText(fontPath, 18); lineText.setScreenSize(1600,900);
        for (int i=startIdx; i<endIdx; ++i) {
            float itemY = y + 30 + (i-startIdx)*itemHeight;
            if (i == selectedIndex) {
                Rectangle sel; sel.setColor(Vector4(0.3f,0.3f,0.35f,1)); sel.setRect(x+2, itemY, w-4, itemHeight-2); sel.draw();
            }
            lineText.setPosition(x+10, itemY+3);
            std::string display = items[i].isDirectory ? "[DIR] " : "[FILE] ";
            display += items[i].path.filename().string();
            lineText.setText(display);
            lineText.setColor(COLOR_TEXT.x,COLOR_TEXT.y,COLOR_TEXT.z,1);
            lineText.draw();
        }
    }

    void setOnFileActivate(std::function<void(const std::string&)> cb) { onFileActivate = cb; }
    const fs::path& getSelectedPath() const { if(selectedIndex>=0) return items[selectedIndex].path; static fs::path empty; return empty; }

private:
    struct Item { fs::path path; bool isDirectory; };
    std::vector<Item> items;
    fs::path currentPath;
    int selectedIndex, lastClickedIndex, scrollOffset;
    float x,y,w,h, itemHeight;
    bool mouseInside;
    std::chrono::steady_clock::time_point lastClickTime;
    std::function<void(const std::string&)> onFileActivate;

    void refresh() {
        items.clear();
        try {
            for (auto& entry : fs::directory_iterator(currentPath)) {
                items.push_back({entry.path(), entry.is_directory()});
            }
            std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
                if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
                return a.path.filename() < b.path.filename();
            });
        } catch(...) {}
        selectedIndex = -1;
        scrollOffset = 0;
    }
    void navigateInto(const fs::path& path) { currentPath = path; refresh(); }
};

// -------------------------------------------------------------------
// Главное окно редактора
// -------------------------------------------------------------------
class CodeEditorWindow {
public:
    CodeEditorWindow(Window& win) : window(win), fileManager(), editor() {
        fileManager.setOnFileActivate([this](const std::string& path) { editor.loadFile(path); });
        consoleMessages.push_back(ConsoleMsg("Editor started", ConsoleMsg::Info, getTimeStr()));
    }

    void init() {
        consoleFont = new Text("engine/fonts/arial.ttf", 16);
        consoleFont->setScreenSize(window.getWidth(), window.getHeight());
    }

    void update(float dt) {
        int winW = window.getWidth(), winH = window.getHeight();
        fileManager.setBounds(0, 0, 300, winH);
        editor.setBounds(300, 0, winW-300, winH-150);
        consoleRect = {0, (float)winH-150, (float)winW, 150};

        fileManager.update(Input::getInstance());
        editor.update(dt, Input::getInstance());

        if (Input::getInstance().isKeyJustPressed(GLFW_KEY_S) &&
            (Input::getInstance().isKeyPressed(GLFW_KEY_LEFT_CONTROL) || Input::getInstance().isKeyPressed(GLFW_KEY_RIGHT_CONTROL))) {
            if (Input::getInstance().isKeyPressed(GLFW_KEY_LEFT_SHIFT) || Input::getInstance().isKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
                editor.saveFile();
                addConsoleMessage("Saved " + editor.getCurrentPath(), ConsoleMsg::Success);
            } else {
                editor.saveFile();
                addConsoleMessage("Saved", ConsoleMsg::Success);
            }
        }

        updatePerformance(dt);
    }

    void draw() {
        fileManager.draw();
        editor.draw();
        drawConsole();
        drawPerformance();
    }

    void shutdown() {}

    void addConsoleMessage(const std::string& msg, ConsoleMsg::Type type = ConsoleMsg::Info) {
        consoleMessages.push_back(ConsoleMsg(msg, type, getTimeStr()));
        if (consoleMessages.size() > 100) consoleMessages.erase(consoleMessages.begin());
    }

private:
    Window& window;
    FileManager fileManager;
    CodeEditor editor;
    Text* consoleFont;
    struct { float x,y,w,h; } consoleRect;
    std::vector<ConsoleMsg> consoleMessages;
    float fps=60, smoothFps=60, frameTime=16;
    std::array<float,120> fpsHistory;
    int historyIdx=0;
    size_t ramUsage=0;

    void drawConsole() {
        static const std::string fontPath = "engine/fonts/arial.ttf";

        Rectangle bg; bg.setColor(Vector4(0.12f,0.12f,0.15f,1)); bg.setRect(consoleRect.x, consoleRect.y, consoleRect.w, consoleRect.h); bg.draw();
        Text title(fontPath, 16); title.setScreenSize(window.getWidth(), window.getHeight());
        title.setPosition(consoleRect.x+5, consoleRect.y+5); title.setText("Console"); title.setColor(COLOR_TEXT.x,COLOR_TEXT.y,COLOR_TEXT.z,1);
        title.draw();
        float lineY = consoleRect.y+30;
        Text lineText(fontPath, 14); lineText.setScreenSize(window.getWidth(), window.getHeight());
        for (const auto& msg : consoleMessages) {
            lineText.setPosition(consoleRect.x+10, lineY);
            lineText.setText("["+msg.timestamp+"] "+msg.text);
            Vector4 col = COLOR_TEXT;
            if (msg.type == ConsoleMsg::Warning) col = Vector4(0.9f,0.6f,0.2f,1);
            else if (msg.type == ConsoleMsg::Error) col = Vector4(1,0.3f,0.3f,1);
            else if (msg.type == ConsoleMsg::Success) col = Vector4(0.3f,0.8f,0.3f,1);
            lineText.setColor(col.x,col.y,col.z,col.w);
            lineText.draw();
            lineY += 20;
            if (lineY > consoleRect.y+consoleRect.h-20) break;
        }
    }

    void drawPerformance() {
        static const std::string fontPath = "engine/fonts/arial.ttf";
        char fpsText[64]; sprintf(fpsText, "FPS: %.1f (%.2f ms)", smoothFps, frameTime);
        Text perf(fontPath, 16); perf.setScreenSize(window.getWidth(), window.getHeight());
        perf.setPosition(window.getWidth()-150, 5);
        perf.setText(fpsText);
        perf.setColor(COLOR_ACCENT.x,COLOR_ACCENT.y,COLOR_ACCENT.z,1);
        perf.draw();
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
        ramUsage = pmc.WorkingSetSize;
        char memText[64]; sprintf(memText, "RAM: %.1f MB", ramUsage/1024.0/1024.0);
        perf.setPosition(window.getWidth()-280, 5);
        perf.setText(memText);
        perf.draw();
#endif
    }

    void updatePerformance(float dt) {
        frameTime = dt*1000;
        smoothFps = smoothFps*0.95f + (1.0f/dt)*0.05f;
        fpsHistory[historyIdx] = smoothFps;
        historyIdx = (historyIdx+1)%fpsHistory.size();
    }

    std::string getTimeStr() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        char buf[20]; strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
        return buf;
    }
};

} // namespace EditorGUI