#include "editor.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdio>

#ifdef PLATFORM_WINDOWS
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

Editor::Editor(fs::path engine_root)
    : m_engine_root(engine_root)
    , m_font(m_freetype, (engine_root / "arial.ttf").string())
{
    LoadTheme();
    ApplyTheme(m_current_theme);
    initTextRenderer();
}

void Editor::initTextRenderer() {
    if (!m_freetype.isValid()) {
        fprintf(stderr, "[Editor] Failed to initialize FreeType for text rendering\n");
        return;
    }
    if (!m_font.isValid()) {
        fprintf(stderr, "[Editor] Failed to load default font for text rendering\n");
        return;
    }
    m_textInitialized = true;
}

void Editor::renderText2DInViewport(BlazeBolt::Text2D& text, const Matrix3x3& projView, ImVec2 canvas_min, ImVec2 canvas_max) {
    if (!m_textInitialized) return;

    GLint prevVAO, prevProg, prevActiveTex, prevTex, prevScissor[4];
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTex);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glGetIntegerv(GL_SCISSOR_BOX, prevScissor);

    if (!blendEnabled) glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(
        static_cast<GLint>(canvas_min.x),
        static_cast<GLint>(ImGui::GetIO().DisplaySize.y - canvas_max.y),
        static_cast<GLsizei>(canvas_max.x - canvas_min.x),
        static_cast<GLsizei>(canvas_max.y - canvas_min.y)
    );

    text.draw(m_fontShader, projView);

    glViewport(0, 0,
        static_cast<GLsizei>(ImGui::GetIO().DisplaySize.x),
        static_cast<GLsizei>(ImGui::GetIO().DisplaySize.y));

    if (!blendEnabled) glDisable(GL_BLEND);
    glBindVertexArray(prevVAO);
    glUseProgram(prevProg);
    glActiveTexture(prevActiveTex);
    glBindTexture(GL_TEXTURE_2D, prevTex);
    glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
}

Editor::~Editor() {
    for (auto& tab : m_tabs) {
        if (tab.image_data) stbi_image_free(tab.image_data);
        if (tab.image_texture) {
            GLuint tex = (GLuint)(intptr_t)tab.image_texture;
            glDeleteTextures(1, &tex);
        }
        for (auto& [path, et] : tab.scene_texture_cache) {
            if (et.id) {
                GLuint tex = (GLuint)(intptr_t)et.id;
                glDeleteTextures(1, &tex);
            }
        }
        tab.scene_texture_cache.clear();
    }
}

void Editor::OpenProject(fs::path project_path) {
    m_project_path = project_path;
    fs::path engine_dir = project_path / "engine";
    if (!fs::exists(engine_dir)) {
        fs::create_directories(engine_dir);
    }
    m_fm_current_dir = engine_dir;
    m_fm_root = engine_dir;
    m_tabs.clear();
    m_active_tab = -1;
    m_active_edit_tab = -1;
    RefreshFM();
}

void Editor::RefreshFM() {
    m_fm_entries.clear();
    if (!fs::exists(m_fm_current_dir) || !fs::is_directory(m_fm_current_dir)) return;

    try {
        for (auto& entry : fs::directory_iterator(m_fm_current_dir, fs::directory_options::skip_permission_denied)) {
            if (!m_fm_show_hidden) {
                std::string name = entry.path().filename().string();
                if (!name.empty() && name[0] == '.' && name != "..") continue;
            }
            m_fm_entries.push_back(entry);
        }
    } catch (...) {}

    std::sort(m_fm_entries.begin(), m_fm_entries.end(), [](const auto& a, const auto& b) {
        bool a_dir = a.is_directory();
        bool b_dir = b.is_directory();
        if (a_dir != b_dir) return a_dir > b_dir;
        return a.path().filename().string() < b.path().filename().string();
    });
}

void Editor::FMNavigateUp() {
    if (m_fm_current_dir.has_parent_path() && m_fm_current_dir != m_fm_root) {
        fs::path next = m_fm_current_dir.parent_path();
        if (next == m_fm_root || next.lexically_normal().string().find(m_fm_root.lexically_normal().string()) == 0) {
            m_fm_current_dir = next;
            RefreshFM();
        }
    }
}

void Editor::FMCreateFile(const std::string& name) {
    std::ofstream f(m_fm_current_dir / name);
    if (f.is_open()) {
        f.close();
        RefreshFM();
        for (int i = 0; i < (int)m_fm_entries.size(); i++) {
            if (m_fm_entries[i].path().filename() == name) {
                m_fm_renaming_idx = i;
                std::string ns(name);
                size_t dot = ns.find_last_of('.');
                if (dot != std::string::npos && dot > 0) {
                    m_fm_rename_ext = ns.substr(dot);
                    strncpy(m_fm_rename_buf, ns.substr(0, dot).c_str(), sizeof(m_fm_rename_buf) - 1);
                } else {
                    m_fm_rename_ext.clear();
                    strncpy(m_fm_rename_buf, name.c_str(), sizeof(m_fm_rename_buf) - 1);
                }
                break;
            }
        }
    }
}

void Editor::FMCreateFolder(const std::string& name) {
    if (fs::create_directories(m_fm_current_dir / name)) {
        RefreshFM();
        for (int i = 0; i < (int)m_fm_entries.size(); i++) {
            if (m_fm_entries[i].path().filename() == name) {
                m_fm_renaming_idx = i;
                m_fm_rename_ext.clear();
                strncpy(m_fm_rename_buf, name.c_str(), sizeof(m_fm_rename_buf) - 1);
                break;
            }
        }
    }
}

void Editor::FMRename(int idx, const std::string& new_name) {
    if (idx < 0 || idx >= (int)m_fm_entries.size()) return;
    fs::path old_path = m_fm_entries[idx].path();
    fs::path new_path = old_path.parent_path() / new_name;
    if (old_path != new_path) {
        fs::rename(old_path, new_path);
        RefreshFM();
    }
}

void Editor::FMCopy(const std::string& src) {
    m_clipboard.path = src;
    m_clipboard.cut = false;
}

void Editor::FMCut(const std::string& src) {
    m_clipboard.path = src;
    m_clipboard.cut = true;
}

void Editor::FMPaste() {
    if (m_clipboard.path.empty() || !fs::exists(m_clipboard.path)) return;
    fs::path dest = m_fm_current_dir / fs::path(m_clipboard.path).filename();
    if (m_clipboard.cut) {
        fs::rename(m_clipboard.path, dest);
        m_clipboard.path.clear();
    } else {
        if (fs::is_directory(m_clipboard.path)) {
            fs::copy(m_clipboard.path, dest, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        } else {
            fs::copy_file(m_clipboard.path, dest, fs::copy_options::overwrite_existing);
        }
    }
    RefreshFM();
}

void Editor::FMOpenFile(const std::string& path) {
    if (fs::is_directory(path)) {
        m_fm_current_dir = path;
        RefreshFM();
    } else {
        OpenFileInTab(path);
    }
}

bool Editor::IsImageFile(const std::string& ext) {
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif" || ext == ".tga";
}

bool Editor::IsSceneFile(const std::string& ext) {
    return ext == ".scene";
}

bool Editor::IsCodeFile(const std::string& ext) {
    return ext == ".lua" || ext == ".luau" || ext == ".txt" || ext == ".json" ||
           ext == ".xml" || ext == ".yaml" || ext == ".yml" || ext == ".cfg" ||
           ext == ".ini" || ext == ".cpp" || ext == ".h" || ext == ".hpp" ||
           ext == ".c" || ext == ".py" || ext == ".js" || ext == ".ts" ||
           ext == ".md" || ext == ".cmake" || ext == ".toml";
}

void Editor::OpenFileInTab(const std::string& path) {
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].file_path == path) {
            m_active_tab = i;
            if (m_tabs[i].type == EditorTabType::Code || m_tabs[i].type == EditorTabType::Image) {
                m_active_edit_tab = i;
            }
            return;
        }
    }

    fs::path p(path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    EditorTab tab;
    tab.file_path = path;
    tab.title = p.filename().string();

    if (ext == ".scene") {
        tab.type = EditorTabType::Scene;
        std::ifstream f(path);
        if (f.is_open()) {
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            tab.scene_json = crude_json::value::parse(content);
            f.close();
        }
        if (tab.scene_json.is_discarded()) {
            tab.scene_json = crude_json::value(crude_json::object{
                {"name", p.stem().string()},
                {"version", 1.0},
                {"objects", crude_json::array{}}
            });
        }
    } else if (IsImageFile(ext)) {
        tab.type = EditorTabType::Image;
        int w, h, ch;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 4);
        if (data) {
            tab.image_data = data;
            tab.img_w = w;
            tab.img_h = h;
            tab.img_channels = 4;
            tab.zoom = 1.0f;

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            tab.image_texture = (ImTextureID)(intptr_t)tex;
        }
    } else {
        tab.type = EditorTabType::Code;
        tab.code_editor = std::make_unique<TextEditor>();
        tab.code_editor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        tab.code_editor->SetPalette(TextEditor::GetDarkPalette());
        tab.code_editor->SetTabSize(4);

        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (f.is_open()) {
            std::streamsize size = f.tellg();
            f.seekg(0, std::ios::beg);
            std::string content(size, '\0');
            f.read(&content[0], size);
            tab.code_editor->SetText(content);
        }
    }

    m_tabs.push_back(std::move(tab));
    int new_idx = (int)m_tabs.size() - 1;
    m_active_tab = new_idx;
    if (m_tabs[new_idx].type == EditorTabType::Code || m_tabs[new_idx].type == EditorTabType::Image) {
        m_active_edit_tab = new_idx;
    }
}

void Editor::CloseTab(int idx) {
    if (idx < 0 || idx >= (int)m_tabs.size()) return;
    auto& tab = m_tabs[idx];
    if (tab.image_data) stbi_image_free(tab.image_data);
    if (tab.image_texture) {
        GLuint tex = (GLuint)(intptr_t)tab.image_texture;
        glDeleteTextures(1, &tex);
    }
    for (auto& [path, et] : tab.scene_texture_cache) {
        if (et.id) {
            GLuint tex = (GLuint)(intptr_t)et.id;
            glDeleteTextures(1, &tex);
        }
    }
    tab.scene_texture_cache.clear();
    m_tabs.erase(m_tabs.begin() + idx);

    auto fix_idx = [&](int& tab_idx) {
        if (tab_idx > idx) {
            tab_idx--;
        } else if (tab_idx == idx) {
            if (tab_idx >= (int)m_tabs.size()) {
                tab_idx = (int)m_tabs.size() - 1;
            } else if (tab_idx > 0) {
                tab_idx--;
            }
        }
    };

    fix_idx(m_active_tab);
    fix_idx(m_active_edit_tab);
}

void Editor::SaveTab(EditorTab& tab) {
    if (tab.type == EditorTabType::Code && tab.code_editor) {
        std::string text = tab.code_editor->GetText();
        std::ofstream f(tab.file_path, std::ios::binary);
        if (f.is_open()) {
            f.write(text.data(), text.size());
            tab.modified = false;
        }
    } else if (tab.type == EditorTabType::Scene) {
        std::string json = tab.scene_json.dump(2, ' ');
        std::ofstream f(tab.file_path, std::ios::binary);
        if (f.is_open()) {
            f.write(json.data(), json.size());
            tab.modified = false;
        }
        for (auto& [path, et] : tab.scene_texture_cache) {
            if (et.id) {
                GLuint tex = (GLuint)(intptr_t)et.id;
                glDeleteTextures(1, &tex);
            }
        }
        tab.scene_texture_cache.clear();
    } else if (tab.type == EditorTabType::Image) {
        if (tab.image_data && tab.img_w > 0 && tab.img_h > 0) {
            std::string ext = fs::path(tab.file_path).extension().string();
            if (ext == ".png") {
                stbi_write_png(tab.file_path.c_str(), tab.img_w, tab.img_h, 4, tab.image_data, tab.img_w * 4);
            } else if (ext == ".jpg" || ext == ".jpeg") {
                stbi_write_jpg(tab.file_path.c_str(), tab.img_w, tab.img_h, 4, tab.image_data, 90);
            } else if (ext == ".bmp") {
                stbi_write_bmp(tab.file_path.c_str(), tab.img_w, tab.img_h, 4, tab.image_data);
            } else if (ext == ".tga") {
                stbi_write_tga(tab.file_path.c_str(), tab.img_w, tab.img_h, 4, tab.image_data);
            }
            tab.modified = false;
        }
    }
}

void Editor::RunGame() {
#ifdef PLATFORM_WINDOWS
    fs::path exe = m_project_path / "BlazeBolt.exe";
    if (!fs::exists(exe)) {
        system(("explorer \"" + m_project_path.string() + "\"").c_str());
        return;
    }
    std::string bat = "@echo off\ncd /d \"" + m_project_path.string() + "\"\nstart \"\" BlazeBolt.exe\n";
    fs::path bat_path = m_project_path / "_run_temp.bat";
    {
        std::ofstream f(bat_path);
        f << bat;
    }
    system(("cmd /c \"" + bat_path.string() + "\"").c_str());
    fs::remove(bat_path);
#else
    fs::path exe = m_project_path / "BlazeBolt";
    if (!fs::exists(exe)) {
        system(("xdg-open \"" + m_project_path.string() + "\"").c_str());
        return;
    }
    system(("cd \"" + m_project_path.string() + "\" && ./BlazeBolt &").c_str());
#endif
}

// ======================== THEMES ========================

void Editor::ApplyTheme(int idx) {
    m_current_theme = idx;
    switch (idx) {
        case 0: ApplyThemeDark(); break;
        case 1: ApplyThemeLight(); break;
        case 2: ApplyThemeClassic(); break;
        case 3: ApplyThemeCyberpunk(); break;
        case 4: ApplyThemeDracula(); break;
        default: ApplyThemeDark(); break;
    }
}

void Editor::ApplyThemeDark() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6; s.FrameRounding = 4; s.GrabRounding = 4; s.ScrollbarRounding = 6;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
    c[ImGuiCol_Border] = ImVec4(0.22f, 0.22f, 0.26f, 0.50f);
    c[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.19f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.31f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.68f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.50f, 0.80f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.36f, 0.62f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.68f, 0.40f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.40f, 0.68f, 0.60f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.40f, 0.68f, 0.80f);
    c[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.17f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.42f, 0.70f, 0.80f);
    c[ImGuiCol_TabActive] = ImVec4(0.20f, 0.40f, 0.68f, 1.0f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.34f, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.44f, 1.0f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.54f, 1.0f);
    c[ImGuiCol_Separator] = ImVec4(0.22f, 0.22f, 0.26f, 0.50f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.50f, 0.80f, 0.80f);
    c[ImGuiCol_SeparatorActive] = ImVec4(0.30f, 0.50f, 0.80f, 1.0f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.40f, 0.68f, 0.50f);
}

void Editor::ApplyThemeLight() {
    ImGui::StyleColorsLight();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6; s.FrameRounding = 4; s.GrabRounding = 4; s.ScrollbarRounding = 6;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.96f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.90f, 0.90f, 0.92f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 1.0f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.88f, 0.92f, 0.98f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.88f, 0.98f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.88f, 0.88f, 0.92f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.78f, 0.84f, 0.96f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.24f, 0.48f, 0.78f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.40f, 0.72f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.34f, 0.66f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.24f, 0.48f, 0.78f, 0.30f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.48f, 0.78f, 0.50f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.48f, 0.78f, 0.70f);
    c[ImGuiCol_Tab] = ImVec4(0.85f, 0.85f, 0.90f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.48f, 0.78f, 0.70f);
    c[ImGuiCol_TabActive] = ImVec4(0.24f, 0.48f, 0.78f, 1.0f);
}

void Editor::ApplyThemeClassic() {
    ImGui::StyleColorsClassic();
}

void Editor::ApplyThemeCyberpunk() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.FrameRounding = 0; s.GrabRounding = 0;
    s.ScrollbarRounding = 0; s.WindowBorderSize = 1; s.FrameBorderSize = 1;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.08f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.06f, 0.10f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.08f, 0.12f, 1.0f);
    c[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.40f);
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.10f, 0.16f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.14f, 0.24f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.16f, 0.30f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.03f, 0.06f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.06f, 0.12f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.00f, 0.80f, 1.00f, 0.60f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.80f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.70f, 0.90f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(1.00f, 0.00f, 0.60f, 0.30f);
    c[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.00f, 0.60f, 0.50f);
    c[ImGuiCol_HeaderActive] = ImVec4(1.00f, 0.00f, 0.60f, 0.70f);
    c[ImGuiCol_Tab] = ImVec4(0.10f, 0.08f, 0.14f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.00f, 0.80f, 1.00f, 0.70f);
    c[ImGuiCol_TabActive] = ImVec4(0.00f, 0.80f, 1.00f, 1.0f);
    c[ImGuiCol_Separator] = ImVec4(0.00f, 1.00f, 1.00f, 0.30f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.05f, 0.08f, 0.60f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.80f, 1.00f, 0.50f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.00f, 0.00f, 0.60f, 0.80f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.00f, 0.60f, 1.0f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.80f, 1.00f, 0.40f);
}

void Editor::ApplyThemeDracula() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 8; s.FrameRounding = 6; s.GrabRounding = 6; s.ScrollbarRounding = 6;
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.14f, 0.20f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.17f, 0.24f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.17f, 0.24f, 1.0f);
    c[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.42f, 0.50f);
    c[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.21f, 0.30f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.27f, 0.38f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.33f, 0.46f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.11f, 0.16f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.17f, 0.24f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.56f, 0.39f, 0.76f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.68f, 0.50f, 0.88f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.48f, 0.32f, 0.68f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.56f, 0.39f, 0.76f, 0.40f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.39f, 0.76f, 0.60f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.56f, 0.39f, 0.76f, 0.80f);
    c[ImGuiCol_Tab] = ImVec4(0.20f, 0.19f, 0.27f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.56f, 0.39f, 0.76f, 0.70f);
    c[ImGuiCol_TabActive] = ImVec4(0.56f, 0.39f, 0.76f, 1.0f);
    c[ImGuiCol_Separator] = ImVec4(0.30f, 0.30f, 0.42f, 0.50f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.14f, 0.20f, 0.60f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.35f, 0.50f, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.46f, 0.45f, 0.62f, 1.0f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.55f, 0.72f, 1.0f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.56f, 0.39f, 0.76f, 0.40f);
}

void Editor::LoadTheme() {
    std::ifstream f(m_engine_root / "theme.txt");
    if (f.is_open()) {
        int idx = 2;
        f >> idx;
        if (idx >= 0 && idx <= 4) m_current_theme = idx;
    }
}

void Editor::SaveTheme() {
    std::ofstream f(m_engine_root / "theme.txt", std::ios::trunc);
    f << m_current_theme;
}

// ======================== MAIN RENDER ========================

void Editor::Render() {
    ApplyTheme(m_current_theme);

    // Menu bar
    ImGui::BeginMainMenuBar();
    RenderMenuBar();
    ImGui::EndMainMenuBar();

    if (ImGui::IsKeyPressed(ImGuiKey_F5)) RunGame();

    // DockSpace (always fills below menu bar)
    RenderDockSpace();

    // Always-visible dockable windows
    RenderFileBrowserWindow();
    RenderSceneHierarchyWindow();
    RenderSceneInspectorWindow();
    RenderSceneEditorWindow();
    RenderCodeEditorWindow();
}

void Editor::RenderDockSpace() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    float menu_bar_height = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + menu_bar_height));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y - menu_bar_height));
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##DockSpace", nullptr, flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}

// ======================== MENU BAR ========================

void Editor::RenderMenuBar() {
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New File", "Ctrl+N")) {
            int idx = 1;
            std::string name;
            do { name = "untitled_" + std::to_string(idx++) + ".lua"; }
            while (fs::exists(m_fm_current_dir / name));
            FMCreateFile(name);
            OpenFileInTab((m_fm_current_dir / name).string());
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save", "Ctrl+S", false, m_active_edit_tab >= 0)) SaveTab(m_tabs[m_active_edit_tab]);
        if (ImGui::MenuItem("Save All")) { for (auto& t : m_tabs) SaveTab(t); }
        ImGui::Separator();
        if (ImGui::MenuItem("Close Tab", "Ctrl+W", false, m_active_tab >= 0)) CloseTab(m_active_tab);
        if (ImGui::MenuItem("Back to Hub")) m_return_to_hub = true;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        if (ImGui::MenuItem("Refresh Files")) RefreshFM();
        ImGui::Separator();
        ImGui::MenuItem("File Browser", nullptr, &m_show_file_browser);
        ImGui::MenuItem("Scene Hierarchy", nullptr, &m_show_scene_hierarchy);
        ImGui::MenuItem("Scene Inspector", nullptr, &m_show_scene_inspector);
        ImGui::MenuItem("Scene Editor", nullptr, &m_show_scene_editor);
        ImGui::MenuItem("Code Editor", nullptr, &m_show_code_editor);
        ImGui::Separator();
        if (ImGui::BeginMenu("Theme")) {
            const char* themes[] = { "Dark", "Light", "Classic", "Cyberpunk", "Dracula" };
            for (int i = 0; i < 5; i++)
                if (ImGui::MenuItem(themes[i], nullptr, m_current_theme == i)) { ApplyTheme(i); SaveTheme(); }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Run")) {
        if (ImGui::MenuItem("Run Game", "F5")) RunGame();
        ImGui::EndMenu();
    }
}

// ======================== FILE BROWSER WINDOW ========================

void Editor::RenderFMTree(const fs::path& dir) {
    std::string dirname = dir.filename().string();
    if (dirname.empty()) dirname = dir.string();
    ImGui::PushID(dir.string().c_str());
    bool is_open = ImGui::TreeNodeEx(dirname.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick);
    bool clicked = ImGui::IsItemClicked();
    if (clicked && m_fm_current_dir != dir) {
        m_fm_current_dir = dir;
        RefreshFM();
    }
    if (is_open) {
        try {
            for (auto& entry : fs::directory_iterator(dir)) {
                if (!entry.is_directory()) continue;
                std::string name = entry.path().filename().string();
                if (!m_fm_show_hidden && name.size() > 0 && name[0] == '.') continue;
                RenderFMTree(entry.path());
            }
        } catch (...) {}
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Editor::RenderFileBrowserWindow() {
    if (!m_show_file_browser) return;

    ImGui::Begin("File Browser", &m_show_file_browser);

    // ── Toolbar ──
    if (m_fm_current_dir != m_fm_root) {
        if (ImGui::Button("..", ImVec2(30, 0))) FMNavigateUp();
    } else {
        ImGui::Dummy(ImVec2(30, 0));
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh")) RefreshFM();
    ImGui::SameLine();
    if (ImGui::SmallButton("+ New")) ImGui::OpenPopup("FM_NewItem");
    ImGui::SameLine();
    ImGui::Checkbox("Hidden", &m_fm_show_hidden);

    if (ImGui::BeginPopup("FM_NewItem")) {
        ImGui::TextDisabled("Create New");
        ImGui::Separator();
        if (ImGui::MenuItem("Folder")) { ImGui::CloseCurrentPopup(); FMCreateFolder("new_folder"); }
        if (ImGui::MenuItem("Lua Script")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_script.lua"); }
        if (ImGui::MenuItem("Scene")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_scene.scene"); }
        if (ImGui::MenuItem("Text File")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_text.txt"); }
        if (ImGui::MenuItem("JSON")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_data.json"); }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // ── Layout: 30% tree | 70% icon grid ──
    float avail_w = ImGui::GetContentRegionAvail().x;
    float tree_w = avail_w * 0.3f;

    // Left: folder tree
    ImGui::BeginChild("##FMTree", ImVec2(tree_w, 0));
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "Project");
    ImGui::Separator();
    RenderFMTree(m_fm_root);
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: large icon grid
    ImGui::BeginChild("##FMIcons", ImVec2(0, 0));
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "%s", m_fm_current_dir.filename().string().c_str());
    ImGui::Separator();

    float icon_size = 64.0f;
    float cell_w = icon_size + 24.0f;
    float panel_w = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, (int)(panel_w / cell_w));
    int col = 0;
    bool fm_any_hovered = false;

    for (int i = 0; i < (int)m_fm_entries.size(); i++) {
        auto& entry = m_fm_entries[i];
        std::string name = entry.path().filename().string();
        if (!m_fm_show_hidden && name.size() > 0 && name[0] == '.') continue;
        bool is_dir = entry.is_directory();
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        ImGui::PushID(i);

        if (col > 0) ImGui::SameLine();

        // Размеры ячейки (иконка + отступы под текст)
        float icon_size = 64.0f;
        float cell_w = icon_size + 24.0f;
        float cell_h = icon_size + 24.0f; // + место для имени

        // Невидимая кнопка, занимающая всю область ячейки
        ImGui::InvisibleButton("##cell", ImVec2(cell_w, cell_h));
        bool hovered = ImGui::IsItemHovered();
        if (hovered) fm_any_hovered = true;
        bool double_clicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        // Координаты кнопки для отрисовки
        ImVec2 rect_min = ImGui::GetItemRectMin();
        ImVec2 rect_max = ImGui::GetItemRectMax();
        ImVec2 cpos = rect_min;

        // Рисование иконки и текста
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 icon_col, border_col;

        if (is_dir) {
            icon_col = IM_COL32(200, 180, 80, 160);
            border_col = IM_COL32(200, 180, 80, 220);
        } else if (IsImageFile(ext)) {
            icon_col = IM_COL32(80, 160, 80, 160);
            border_col = IM_COL32(80, 160, 80, 220);
        } else if (IsSceneFile(ext)) {
            icon_col = IM_COL32(200, 100, 60, 160);
            border_col = IM_COL32(200, 100, 60, 220);
        } else if (IsCodeFile(ext)) {
            icon_col = IM_COL32(80, 100, 180, 160);
            border_col = IM_COL32(80, 100, 180, 220);
        } else {
            icon_col = IM_COL32(100, 100, 120, 120);
            border_col = IM_COL32(120, 120, 140, 180);
        }

        // Иконка
        ImVec2 icon_min = ImVec2(cpos.x + (cell_w - icon_size) * 0.5f, cpos.y + 4.0f);
        ImVec2 icon_max = ImVec2(icon_min.x + icon_size, icon_min.y + icon_size);
        dl->AddRectFilled(icon_min, icon_max, icon_col, 4.0f);
        dl->AddRect(icon_min, icon_max, border_col, 4.0f, 0, 2.0f);

        // Буква-символ внутри иконки
        const char* letter = is_dir ? ">>" : IsImageFile(ext) ? "[]" : IsSceneFile(ext) ? "Sd" : IsCodeFile(ext) ? "{}" : "--";
        ImVec2 text_size = ImGui::CalcTextSize(letter);
        ImVec2 text_pos = ImVec2(icon_min.x + (icon_size - text_size.x) * 0.5f,
                                 icon_min.y + (icon_size - text_size.y) * 0.5f);
        dl->AddText(text_pos, IM_COL32(255, 255, 255, 180), letter);

        // Имя файла под иконкой (или поле для переименования)
        float name_w = ImGui::CalcTextSize(name.c_str()).x;
        float max_name_w = icon_size + 8.0f;
        if (name_w > max_name_w) name_w = max_name_w;
        ImVec2 name_pos = ImVec2(cpos.x + (cell_w - max_name_w) * 0.5f,
                                 icon_max.y + 4.0f);

        if (m_fm_renaming_idx == i) {
            ImGui::SetKeyboardFocusHere(0);
            ImGui::SetCursorScreenPos(name_pos);
            ImGui::PushItemWidth(max_name_w + 8.0f);
            bool commit = ImGui::InputText("##rename", m_fm_rename_buf, sizeof(m_fm_rename_buf),
                ImGuiInputTextFlags_EnterReturnsTrue);
            bool cancelled = ImGui::IsItemDeactivatedAfterEdit() && !commit;
            if (commit) {
                std::string new_name = m_fm_rename_buf;
                if (!m_fm_rename_ext.empty() && new_name.find('.') == std::string::npos)
                    new_name += m_fm_rename_ext;
                FMRename(i, new_name);
                m_fm_renaming_idx = -1;
                m_fm_rename_ext.clear();
            } else if (cancelled) {
                m_fm_renaming_idx = -1;
                m_fm_rename_ext.clear();
            }
            ImGui::PopItemWidth();
        } else {
            dl->AddText(name_pos, IM_COL32(220, 220, 230, 255), name.c_str());
        }

        // Контекстное меню (привязывается к нашей кнопке)
        if (ImGui::BeginPopupContextItem()) {
            if (is_dir) {
                if (ImGui::MenuItem("Open")) FMOpenFile(entry.path().string());
            } else {
                if (ImGui::MenuItem("Open in Editor")) OpenFileInTab(entry.path().string());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Rename")) {
                m_fm_renaming_idx = i;
                size_t dot = name.find_last_of('.');
                if (dot != std::string::npos && dot > 0) {
                    m_fm_rename_ext = name.substr(dot);
                    strncpy(m_fm_rename_buf, name.substr(0, dot).c_str(), sizeof(m_fm_rename_buf) - 1);
                } else {
                    m_fm_rename_ext.clear();
                    strncpy(m_fm_rename_buf, name.c_str(), sizeof(m_fm_rename_buf) - 1);
                }
            }
            if (ImGui::MenuItem("Copy")) FMCopy(entry.path().string());
            if (ImGui::MenuItem("Cut")) FMCut(entry.path().string());
            if (!m_clipboard.path.empty() && ImGui::MenuItem("Paste Here")) FMPaste();
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                fs::remove_all(entry.path());
                RefreshFM();
                ImGui::EndPopup();
                ImGui::PopID();
                break; // выход из цикла после удаления, т.к. список обновился
            }
            if (ImGui::MenuItem("Show in Explorer")) {
            #ifdef PLATFORM_WINDOWS
                        system(("explorer /select,\"" + entry.path().string() + "\"").c_str());
            #else
                        system(("xdg-open \"" + entry.path().parent_path().string() + "\"").c_str());
            #endif
        }
        ImGui::EndPopup();
    }

    // Двойной клик
    if (double_clicked) {
        if (is_dir) {
            m_fm_current_dir = entry.path();
            RefreshFM();
        } else {
            FMOpenFile(entry.path().string());
        }
    }

    ImGui::PopID();

    col++;
    if (col >= cols) col = 0;
}
    // Right-click on empty area -> context menu (only if no item was hovered)
    if (!fm_any_hovered && ImGui::BeginPopupContextWindow("##FMEmpyArea", ImGuiPopupFlags_MouseButtonRight)) {
        ImGui::TextDisabled("Create New");
        ImGui::Separator();
        if (ImGui::MenuItem("Folder")) { ImGui::CloseCurrentPopup(); FMCreateFolder("new_folder"); }
        if (ImGui::MenuItem("Lua Script")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_script.lua"); }
        if (ImGui::MenuItem("Scene")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_scene.scene"); }
        if (ImGui::MenuItem("Text File")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_text.txt"); }
        if (ImGui::MenuItem("JSON")) { ImGui::CloseCurrentPopup(); FMCreateFile("new_data.json"); }
        ImGui::EndPopup();
    }

    // Cancel rename on Escape
    if (m_fm_renaming_idx >= 0 && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_fm_renaming_idx = -1;
    }

    ImGui::EndChild();

    ImGui::End();
}

// ======================== SCENE PROPERTY HELPERS ========================

static void SceneEditString(const char* label, crude_json::value& obj, const std::string& key, crude_json::value* fallback = nullptr) {
    std::string val;
    if (obj.is_object() && obj.contains(key) && obj[key].is_string())
        val = obj[key].get<crude_json::string>();
    else if (fallback && fallback->is_string())
        val = fallback->get<crude_json::string>();
    char buf[512];
    strncpy(buf, val.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText(label, buf, sizeof(buf))) {
        if (obj.is_object()) {
            obj[key] = crude_json::string(buf);
        }
    }
}

static void SceneEditNumber(const char* label, crude_json::value& obj, const std::string& key, double fallback = 0) {
    double val = fallback;
    if (obj.is_object() && obj.contains(key) && obj[key].is_number())
        val = obj[key].get<crude_json::number>();
    float fval = (float)val;
    if (ImGui::InputFloat(label, &fval, 0.1f, 1.0f, "%.3f")) {
        if (obj.is_object()) {
            obj[key] = (crude_json::number)fval;
        }
    }
}

static void SceneEditBool(const char* label, crude_json::value& obj, const std::string& key, bool fallback = false) {
    bool val = fallback;
    if (obj.is_object() && obj.contains(key) && obj[key].is_boolean())
        val = obj[key].get<crude_json::boolean>();
    if (ImGui::Checkbox(label, &val)) {
        if (obj.is_object()) {
            obj[key] = (crude_json::boolean)val;
        }
    }
}

static void SceneEditVec2(const char* label, crude_json::value& obj, const std::string& key, double fx = 0, double fy = 0) {
    float v[2] = {(float)fx, (float)fy};
    if (obj.is_object() && obj.contains(key) && obj[key].is_array() && obj[key].get<crude_json::array>().size() >= 2) {
        v[0] = (float)obj[key].get<crude_json::array>()[0].get<crude_json::number>();
        v[1] = (float)obj[key].get<crude_json::array>()[1].get<crude_json::number>();
    }
    if (ImGui::InputFloat2(label, v)) {
        if (obj.is_object()) {
            obj[key] = crude_json::array{ (crude_json::number)v[0], (crude_json::number)v[1] };
        }
    }
}

static void SceneEditVec4(const char* label, crude_json::value& obj, const std::string& key, double f0 = 1, double f1 = 1, double f2 = 1, double f3 = 1) {
    float v[4] = {(float)f0, (float)f1, (float)f2, (float)f3};
    if (obj.is_object() && obj.contains(key) && obj[key].is_array() && obj[key].get<crude_json::array>().size() >= 4) {
        v[0] = (float)obj[key].get<crude_json::array>()[0].get<crude_json::number>();
        v[1] = (float)obj[key].get<crude_json::array>()[1].get<crude_json::number>();
        v[2] = (float)obj[key].get<crude_json::array>()[2].get<crude_json::number>();
        v[3] = (float)obj[key].get<crude_json::array>()[3].get<crude_json::number>();
    }
    if (ImGui::InputFloat4(label, v)) {
        if (obj.is_object()) {
            obj[key] = crude_json::array{ (crude_json::number)v[0], (crude_json::number)v[1], (crude_json::number)v[2], (crude_json::number)v[3] };
        }
    }
}

static void SceneEditTypeProps(const char* type, crude_json::value& obj) {
    if (!obj.is_object()) return;
    std::string t = type ? type : "";
    if (t == "sprite") {
        SceneEditString("Texture", obj, "texture");
        SceneEditVec4("tex_rect", obj, "texture_rect", 0, 0, 1, 1);
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("size_x", obj, "size_x"); SceneEditNumber("size_y", obj, "size_y");
        SceneEditNumber("rot", obj, "rot");
        SceneEditNumber("color_r", obj, "color_r", 1); SceneEditNumber("color_g", obj, "color_g", 1);
        SceneEditNumber("color_b", obj, "color_b", 1); SceneEditNumber("color_a", obj, "color_a", 1);
        SceneEditBool("visible", obj, "visible");
        SceneEditNumber("origin_x", obj, "origin_x", 0.5); SceneEditNumber("origin_y", obj, "origin_y", 0.5);
    } else if (t == "animated_sprite") {
        SceneEditString("Texture", obj, "texture");
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("size_x", obj, "size_x"); SceneEditNumber("size_y", obj, "size_y");
        SceneEditNumber("rot", obj, "rot");
        SceneEditNumber("color_r", obj, "color_r", 1); SceneEditNumber("color_g", obj, "color_g", 1);
        SceneEditNumber("color_b", obj, "color_b", 1); SceneEditNumber("color_a", obj, "color_a", 1);
        SceneEditBool("visible", obj, "visible");
        SceneEditBool("looping", obj, "looping");
        SceneEditNumber("playback speed", obj, "playback_speed");
    } else if (t == "text") {
        SceneEditString("Font", obj, "font");
        SceneEditString("Text", obj, "text");
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("scale_x", obj, "scale_x"); SceneEditNumber("scale_y", obj, "scale_y");
        SceneEditNumber("rot", obj, "rot");
        SceneEditNumber("color_r", obj, "color_r", 1); SceneEditNumber("color_g", obj, "color_g", 1);
        SceneEditNumber("color_b", obj, "color_b", 1); SceneEditNumber("color_a", obj, "color_a", 1);
        SceneEditBool("visible", obj, "visible");
        SceneEditNumber("alignment", obj, "alignment");
    } else if (t == "camera") {
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("zoom", obj, "zoom");
        SceneEditNumber("rot", obj, "rot");
    } else if (t == "point_light") {
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("color_r", obj, "color_r", 1); SceneEditNumber("color_g", obj, "color_g", 1);
        SceneEditNumber("color_b", obj, "color_b", 1);
        SceneEditNumber("intensity", obj, "intensity");
        SceneEditNumber("radius", obj, "radius");
        SceneEditBool("enabled", obj, "enabled");
    } else if (t == "ambient_light") {
        SceneEditNumber("color_r", obj, "color_r", 1); SceneEditNumber("color_g", obj, "color_g", 1);
        SceneEditNumber("color_b", obj, "color_b", 1);
        SceneEditNumber("intensity", obj, "intensity");
    } else if (t == "particle_system") {
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditString("Texture", obj, "texture");
        SceneEditNumber("emission rate", obj, "emission_rate");
        SceneEditBool("active", obj, "active");
        SceneEditBool("visible", obj, "visible");
    } else if (t == "tileset") {
        SceneEditString("Texture", obj, "texture");
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("tile_width", obj, "tile_width"); SceneEditNumber("tile_height", obj, "tile_height");
        SceneEditNumber("atlas_cols", obj, "atlas_cols"); SceneEditNumber("atlas_rows", obj, "atlas_rows");
    } else if (t == "physics_body") {
        SceneEditNumber("pos_x", obj, "pos_x"); SceneEditNumber("pos_y", obj, "pos_y");
        SceneEditNumber("rot", obj, "rot");
    }
    // ── Physics body properties (common to all types) ──
    {
        double rawBT = (obj.is_object() && obj.contains("body_type") && obj["body_type"].is_number()) ? obj["body_type"].get<crude_json::number>() : -1;
        int bodyType = (int)rawBT;
        const char* typeNames[] = { "None", "Static", "Dynamic", "Kinematic" };
        int typeIdx = bodyType < 0 ? 0 : (bodyType + 1);
        if (ImGui::Combo("Body Type", &typeIdx, typeNames, 4)) {
            if (typeIdx <= 0) {
                obj.erase("body_type");
            } else {
                obj["body_type"] = (crude_json::number)(typeIdx - 1);
            }
        }
        if (bodyType >= 0) {
            ImGui::Indent(16.0f);
            SceneEditNumber("mass", obj, "mass", 1.0);
            SceneEditNumber("friction", obj, "friction", 0.3);
            SceneEditNumber("restitution", obj, "restitution", 0.5);
            const char* shapeNames[] = { "Circle", "Rectangle" };
            std::string curShape = (obj.is_object() && obj.contains("collider_shape") && obj["collider_shape"].is_string()) ? obj["collider_shape"].get<crude_json::string>() : "circle";
            int shapeIdx = (curShape == "rectangle") ? 1 : 0;
            if (ImGui::Combo("Collider Shape", &shapeIdx, shapeNames, 2)) {
                obj["collider_shape"] = std::string(shapeIdx == 0 ? "circle" : "rectangle");
            }
            if (shapeIdx == 0) {
                SceneEditNumber("circle_radius", obj, "circle_radius", 32.0);
                SceneEditNumber("circle_offset_x", obj, "circle_offset_x", 0.0);
                SceneEditNumber("circle_offset_y", obj, "circle_offset_y", 0.0);
            } else {
                SceneEditNumber("rect_half_width", obj, "rect_half_width", 32.0);
                SceneEditNumber("rect_half_height", obj, "rect_half_height", 32.0);
            }
            SceneEditNumber("gravity_scale", obj, "gravity_scale", 1.0);
            SceneEditBool("fixed_rotation", obj, "fixed_rotation");
            SceneEditBool("bullet", obj, "bullet");
            ImGui::Unindent(16.0f);
        }
    }
}

static const char* s_scene_types[] = {
    "sprite", "animated_sprite", "text", "camera",
    "point_light", "ambient_light", "particle_system", "tileset",
    "physics_body"
};

// ======================== SCENE INSPECTOR WINDOW ========================

void Editor::RenderSceneHierarchyWindow() {
    if (!m_show_scene_hierarchy) return;

    ImGui::Begin("Scene Hierarchy", &m_show_scene_hierarchy);

    int scene_tab_idx = -1;
    if (m_active_tab >= 0 && m_tabs[m_active_tab].type == EditorTabType::Scene) {
        scene_tab_idx = m_active_tab;
    } else {
        for (int i = 0; i < (int)m_tabs.size(); i++) {
            if (m_tabs[i].type == EditorTabType::Scene) { scene_tab_idx = i; break; }
        }
    }

    if (scene_tab_idx < 0) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a .scene file to edit objects");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    auto& tab = m_tabs[scene_tab_idx];
    m_active_tab = scene_tab_idx;
    auto& doc = tab.scene_json;

    auto getNum = [](crude_json::value& v, const std::string& key, double def) -> double {
        return (v.is_object() && v.contains(key) && v[key].is_number()) ? v[key].get<crude_json::number>() : def;
    };
    auto getStr = [](crude_json::value& v, const std::string& key, const std::string& def) -> std::string {
        return (v.is_object() && v.contains(key) && v[key].is_string()) ? v[key].get<crude_json::string>() : def;
    };
    auto& objects = [&]() -> crude_json::array& {
        static crude_json::array empty_arr;
        if (doc.contains("objects") && doc["objects"].is_array())
            return doc["objects"].get<crude_json::array>();
        return empty_arr;
    }();

    if (ImGui::Button("Save", ImVec2(80, 0))) SaveTab(tab);
    ImGui::Separator();

    ImGui::BeginChild("##HierarchyContent", ImVec2(0, 0));
    {
        ImGui::Text("Objects (%zu)", objects.size());
        ImGui::Separator();

        int to_remove = -1;
        for (size_t i = 0; i < objects.size(); i++) {
            auto& obj = objects[i];
            if (!obj.is_object()) continue;
            std::string display_name = getStr(obj, "name", "Unnamed");
            std::string type_str = getStr(obj, "type", "?");
            ImGui::PushID((int)i);
            bool sel = (tab.scene_selected_object == (int)i);
            float row_h = ImGui::GetTextLineHeightWithSpacing();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 av = ImGui::GetContentRegionAvail();
            if (ImGui::Selectable("##sel", &sel, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, row_h))) {
                tab.scene_selected_object = (int)i;
            }
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove")) { to_remove = (int)i; }
                if (ImGui::MenuItem("Duplicate")) {
                    objects.push_back(crude_json::value(objects[i]));
                    tab.modified = true;
                }
                ImGui::EndPopup();
            }
            ImGui::SetCursorScreenPos(pos);
            ImGui::Text("  %s", display_name.c_str());
            std::string type_tag = "[" + type_str + "]";
            float tag_w = ImGui::CalcTextSize(type_tag.c_str()).x;
            ImGui::SetCursorScreenPos(ImVec2(pos.x + av.x - tag_w - 4, pos.y));
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1), "%s", type_tag.c_str());
            ImGui::PopID();
        }
        if (to_remove >= 0) {
            objects.erase(objects.begin() + to_remove);
            tab.modified = true;
            if (tab.scene_selected_object == to_remove) tab.scene_selected_object = -1;
            else if (tab.scene_selected_object > to_remove) tab.scene_selected_object--;
        }

        ImGui::Separator();
        ImGui::Text("Add");
        ImGui::InputText("Name##NewObj", tab.scene_new_name, sizeof(tab.scene_new_name));
        int type_idx = 0;
        for (int i = 0; i < 9; i++) { if (strcmp(tab.scene_new_type, s_scene_types[i]) == 0) { type_idx = i; break; } }
        if (ImGui::Combo("Type", &type_idx, s_scene_types, 9)) {
            strncpy(tab.scene_new_type, s_scene_types[type_idx], sizeof(tab.scene_new_type) - 1);
        }
        if (ImGui::Button("Add Object")) {
            if (tab.scene_new_name[0]) {
                crude_json::value n{
                    crude_json::object{{"name", std::string(tab.scene_new_name)}, {"type", std::string(tab.scene_new_type)}, {"visible", true}}
                };
                objects.push_back(std::move(n));
                tab.scene_new_name[0] = '\0';
                tab.modified = true;
            }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void Editor::RenderSceneInspectorWindow() {
    if (!m_show_scene_inspector) return;

    ImGui::Begin("Scene Inspector", &m_show_scene_inspector);

    int scene_tab_idx = -1;
    if (m_active_tab >= 0 && m_tabs[m_active_tab].type == EditorTabType::Scene) {
        scene_tab_idx = m_active_tab;
    } else {
        for (int i = 0; i < (int)m_tabs.size(); i++) {
            if (m_tabs[i].type == EditorTabType::Scene) { scene_tab_idx = i; break; }
        }
    }

    if (scene_tab_idx < 0) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a .scene file to edit objects");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    auto& tab = m_tabs[scene_tab_idx];
    m_active_tab = scene_tab_idx;
    auto& doc = tab.scene_json;

    auto getNum = [](crude_json::value& v, const std::string& key, double def) -> double {
        return (v.is_object() && v.contains(key) && v[key].is_number()) ? v[key].get<crude_json::number>() : def;
    };
    auto getStr = [](crude_json::value& v, const std::string& key, const std::string& def) -> std::string {
        return (v.is_object() && v.contains(key) && v[key].is_string()) ? v[key].get<crude_json::string>() : def;
    };
    auto& objects = [&]() -> crude_json::array& {
        static crude_json::array empty_arr;
        if (doc.contains("objects") && doc["objects"].is_array())
            return doc["objects"].get<crude_json::array>();
        return empty_arr;
    }();

    // Properties for selected object
    if (tab.scene_selected_object >= 0 && tab.scene_selected_object < (int)objects.size()) {
        auto& obj = objects[tab.scene_selected_object];
        if (obj.is_object()) {
            std::string type = getStr(obj, "type", "");
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.9f, 1), "Properties");
            ImGui::Separator();

            std::string name = getStr(obj, "name", "");
            char name_buf[256];
            strncpy(name_buf, name.c_str(), sizeof(name_buf) - 1);
            name_buf[sizeof(name_buf) - 1] = '\0';
            if (ImGui::InputText("Name##Prop", name_buf, sizeof(name_buf))) {
                obj["name"] = std::string(name_buf);
                tab.modified = true;
            }

            char type_buf[64];
            strncpy(type_buf, type.c_str(), sizeof(type_buf) - 1);
            type_buf[sizeof(type_buf) - 1] = '\0';
            if (ImGui::InputText("Type##PropType", type_buf, sizeof(type_buf))) {
                obj["type"] = std::string(type_buf);
                tab.modified = true;
            }

            ImGui::Separator();
            SceneEditTypeProps(type.c_str(), obj);
            if (ImGui::IsItemDeactivatedAfterEdit()) tab.modified = true;
        }
    } else {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.4f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1));
        ImGui::Text("Select an object\nin the viewport\nor tree");
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

// ======================== CODE EDITOR WINDOW ========================

void Editor::RenderCodeEditorWindow() {
    if (!m_show_code_editor) return;

    ImGui::Begin("Code Editor", &m_show_code_editor);

    // Collect all editable tabs (code + image)
    std::vector<int> edit_tab_indices;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].type == EditorTabType::Code || m_tabs[i].type == EditorTabType::Image) {
            edit_tab_indices.push_back(i);
        }
    }

    if (edit_tab_indices.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a file to start editing");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // Use separate active tab tracker for editor
    bool active_is_editable = false;
    for (int idx : edit_tab_indices) {
        if (idx == m_active_edit_tab) { active_is_editable = true; break; }
    }
    if (!active_is_editable) {
        m_active_edit_tab = edit_tab_indices[0];
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) { SaveTab(m_tabs[m_active_edit_tab]); }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W)) { CloseTab(m_active_edit_tab); ImGui::End(); return; }

    // Tab bar for editable files (stable label = title only, no " *" to prevent ImGui ID from changing)
    if (ImGui::BeginTabBar("##CodeTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll)) {
        for (int idx : edit_tab_indices) {
            auto& t = m_tabs[idx];

            const char* label = t.title.c_str();
            ImGui::PushID(t.file_path.c_str());
            if (t.modified) {
                ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.30f, 0.25f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.40f, 0.33f, 0.12f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.35f, 0.28f, 0.11f, 1.0f));
            }
            bool open = true;
            bool active = ImGui::BeginTabItem(label, &open);
            if (t.modified) ImGui::PopStyleColor(3);
            if (active) {
                m_active_edit_tab = idx;
                if (t.type == EditorTabType::Code && t.code_editor) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    t.code_editor->Render(t.title.c_str(), avail);
                    if (t.code_editor->IsTextChanged()) t.modified = true;
                } else if (t.type == EditorTabType::Image && t.image_texture) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float img_ar = (float)t.img_w / (float)t.img_h;
                    float area_ar = avail.x / avail.y;

                    float draw_w, draw_h;
                    if (img_ar > area_ar) {
                        draw_w = avail.x;
                        draw_h = avail.x / img_ar;
                    } else {
                        draw_h = avail.y;
                        draw_w = avail.y * img_ar;
                    }
                    float offset_x = (avail.x - draw_w) * 0.5f;
                    float offset_y = (avail.y - draw_h) * 0.5f;
                    ImVec2 image_min = ImVec2(ImGui::GetCursorScreenPos().x + offset_x, ImGui::GetCursorScreenPos().y + offset_y);
                    ImVec2 image_max = ImVec2(image_min.x + draw_w, image_min.y + draw_h);
                    ImGui::GetWindowDrawList()->AddImage(t.image_texture, image_min, image_max);
                    ImGui::Text("  %d x %d  |  Zoom: %.0f%%", t.img_w, t.img_h, t.zoom * 100);
                }
                ImGui::EndTabItem();
            }
            ImGui::PopID();
            if (!open) {
                CloseTab(idx);
                ImGui::EndTabBar();
                ImGui::End();
                return;
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ======================== SCENE EDITOR WINDOW ========================

void Editor::RenderSceneEditorWindow() {
    if (!m_show_scene_editor) return;

    ImGui::Begin("Scene Editor", &m_show_scene_editor);

    std::vector<int> scene_tab_indices;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].type == EditorTabType::Scene) scene_tab_indices.push_back(i);
    }

    if (scene_tab_indices.empty()) {
        float avail = ImGui::GetContentRegionAvail().y;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a .scene file to edit");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    int active_scene_idx = -1;
    {
        if (scene_tab_indices.size() > 1) {
            if (ImGui::BeginTabBar("##SceneEditorTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll)) {
                for (int idx : scene_tab_indices) {
                    auto& t = m_tabs[idx];
                    ImGui::PushID(t.file_path.c_str());
                    if (t.modified) {
                        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.30f, 0.25f, 0.10f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.40f, 0.33f, 0.12f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.35f, 0.28f, 0.11f, 1.0f));
                    }
                    bool open = true;
                    if (ImGui::BeginTabItem(t.title.c_str(), &open)) {
                        active_scene_idx = idx;
                        ImGui::EndTabItem();
                    }
                    if (t.modified) ImGui::PopStyleColor(3);
                    ImGui::PopID();
                    if (!open) {
                        CloseTab(idx);
                        ImGui::End();
                        return;
                    }
                }
                ImGui::EndTabBar();
            }
        } else {
            active_scene_idx = scene_tab_indices[0];
        }
    }

    if (active_scene_idx >= 0) {
        m_active_tab = active_scene_idx;
        RenderSceneEditor(m_tabs[active_scene_idx]);
    }

    ImGui::End();
}

// ======================== SCENE EDITOR ========================

void Editor::OpenSceneEditor(const std::string& path) {
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].file_path == path) {
            m_active_tab = i;
            return;
        }
    }

    EditorTab tab;
    tab.file_path = path;
    tab.title = fs::path(path).filename().string();
    tab.type = EditorTabType::Scene;
    tab.scene_selected_object = -1;

    std::ifstream f(path);
    if (f.is_open()) {
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        tab.scene_json = crude_json::value::parse(content);
        f.close();
    }
    if (tab.scene_json.is_discarded()) {
        tab.scene_json = crude_json::value(crude_json::object{
            {"name", fs::path(path).stem().string()},
            {"version", 1.0},
            {"objects", crude_json::array{}}
        });
    }

    m_tabs.push_back(std::move(tab));
    m_active_tab = (int)m_tabs.size() - 1;
}

void Editor::RenderSceneEditor(EditorTab& tab) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveTab(tab);

    auto& doc = tab.scene_json;
    if (doc.is_discarded()) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Invalid scene file");
        return;
    }

    auto hasObjects = [&]() -> crude_json::array& {
        static crude_json::array empty_arr;
        if (doc.contains("objects") && doc["objects"].is_array())
            return doc["objects"].get<crude_json::array>();
        return empty_arr;
    };
    auto& objects = hasObjects();

    auto getNum = [](crude_json::value& v, const std::string& key, double def) -> double {
        return (v.is_object() && v.contains(key) && v[key].is_number()) ? v[key].get<crude_json::number>() : def;
    };
    auto setNum = [](crude_json::value& v, const std::string& key, double val) {
        if (v.is_object()) v[key] = (crude_json::number)val;
    };
    auto getStr = [](crude_json::value& v, const std::string& key, const std::string& def) -> std::string {
        return (v.is_object() && v.contains(key) && v[key].is_string()) ? v[key].get<crude_json::string>() : def;
    };
    auto getDefaultSize = [](const std::string& type) -> ImVec2 {
        if (type == "sprite") return ImVec2(0.4f, 0.4f);
        if (type == "animated_sprite") return ImVec2(0.4f, 0.4f);
        if (type == "text") return ImVec2(0.8f, 0.15f);
        if (type == "camera") return ImVec2(0.2f, 0.2f);
        if (type == "point_light") return ImVec2(0.3f, 0.3f);
        if (type == "ambient_light") return ImVec2(1.0f, 1.0f);
        if (type == "particle_system") return ImVec2(0.3f, 0.3f);
        if (type == "tileset") return ImVec2(0.8f, 0.8f);
        if (type == "physics_body") return ImVec2(0.4f, 0.4f);
        return ImVec2(0.4f, 0.4f);
    };
    auto getTypeColor = [](const std::string& type) -> ImU32 {
        if (type == "sprite") return IM_COL32(70, 140, 255, 200);
        if (type == "animated_sprite") return IM_COL32(0, 200, 255, 200);
        if (type == "text") return IM_COL32(80, 220, 80, 200);
        if (type == "camera") return IM_COL32(255, 220, 50, 200);
        if (type == "point_light") return IM_COL32(255, 160, 40, 200);
        if (type == "ambient_light") return IM_COL32(255, 240, 140, 100);
        if (type == "particle_system") return IM_COL32(200, 80, 255, 200);
        if (type == "tileset") return IM_COL32(180, 140, 100, 200);
        if (type == "physics_body") return IM_COL32(0, 200, 255, 120);
        return IM_COL32(150, 150, 150, 200);
    };

    // Toolbar
    {
        if (ImGui::Button("Save", ImVec2(80, 0))) SaveTab(tab);
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        if (ImGui::Button("Fit View")) { tab.pan_x = tab.pan_y = 0; tab.zoom = 1.0f; }
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        const char* gizmo_names[3] = { "Move", "Rotate", "Scale" };
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4);
        for (int gm = 0; gm < 3; gm++) {
            if (gm > 0) ImGui::SameLine(0, 2);
            bool active = (tab.scene_gizmo_mode == gm);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.50f, 0.85f, 1));
            if (ImGui::Button(gizmo_names[gm])) { tab.scene_gizmo_mode = gm; tab.scene_drag_handle = -1; }
            if (active) ImGui::PopStyleColor();
        }
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        char mode_shortcuts[3] = { 'W', 'E', 'R' };
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1), "  [%c] %s  |  Zoom: %.0f%%",
            mode_shortcuts[tab.scene_gizmo_mode], gizmo_names[tab.scene_gizmo_mode], tab.zoom * 100);
    }

    ImGui::Separator();

    // Viewport
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##SceneViewport", avail, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 cv_p0 = ImGui::GetCursorScreenPos();
        ImVec2 cv_sz = ImGui::GetContentRegionAvail();
        if (cv_sz.x < 50) cv_sz.x = 50;
        if (cv_sz.y < 50) cv_sz.y = 50;
        ImVec2 cv_p1 = ImVec2(cv_p0.x + cv_sz.x, cv_p0.y + cv_sz.y);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(cv_p0, cv_p1, IM_COL32(30, 30, 35, 255));

        // Camera-aware viewport centering (first render)
        constexpr float PPM = 160.0f;
        if (!tab.scene_view_initialized) {
            for (auto& obj : objects) {
                if (obj.is_object() && getStr(obj, "type", "") == "camera") {
                    double cx = getNum(obj, "pos_x", 0);
                    double cy = getNum(obj, "pos_y", 0);
                    double cz = getNum(obj, "zoom", 1);
                    tab.pan_x = (float)(-cx * cz * PPM);
                    tab.pan_y = (float)(cy * cz * PPM);
                    tab.zoom = std::max(0.05f, std::min((float)cz, 20.0f));
                    break;
                }
            }
            tab.scene_view_initialized = true;
        }

        float ox = cv_p0.x + cv_sz.x * 0.5f + tab.pan_x;
        float oy = cv_p0.y + cv_sz.y * 0.5f + tab.pan_y;
        float z = tab.zoom * PPM;
        float gz = tab.zoom;

        auto w2s = [&](float wx, float wy) { return ImVec2(wx * z + ox, -wy * z + oy); };
        auto s2w = [&](float sx, float sy) { return ImVec2((sx - ox) / z, -(sy - oy) / z); };
        auto bbox = [&](double l, double b, double w, double h) -> std::pair<ImVec2, ImVec2> {
            ImVec2 bl = w2s((float)l, (float)b);
            ImVec2 br = w2s((float)(l + w), (float)b);
            ImVec2 tl = w2s((float)l, (float)(b + h));
            ImVec2 tr = w2s((float)(l + w), (float)(b + h));
            return {
                ImVec2(std::min({bl.x, br.x, tl.x, tr.x}), std::min({bl.y, br.y, tl.y, tr.y})),
                ImVec2(std::max({bl.x, br.x, tl.x, tr.x}), std::max({bl.y, br.y, tl.y, tr.y}))
            };
        };

        // Grid
        static const float gsteps[] = {0.1f, 0.2f, 0.5f, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
        float gs = gsteps[sizeof(gsteps)/sizeof(gsteps[0]) - 1];
        for (float s : gsteps) { if (s * z >= 50.0f) { gs = s; break; } }
        ImVec2 wtl = s2w(cv_p0.x, cv_p0.y);
        ImVec2 wbr = s2w(cv_p1.x, cv_p1.y);
        float gx0 = std::floor(wtl.x / gs) * gs;
        float gy0 = std::floor(wtl.y / gs) * gs;
        float fs = ImGui::GetFontSize();
        char lbl[64];

        // Minor grid
        if (gs * z * 0.25f > 8.0f) {
            float ms = gs * 0.25f;
            for (float wx = gx0; wx <= wbr.x + gs; wx += ms) {
                ImVec2 a = w2s(wx, wtl.y), b = w2s(wx, wbr.y);
                dl->AddLine(a, b, IM_COL32(40, 40, 48, 255));
            }
            for (float wy = gy0; wy >= wbr.y - gs; wy -= ms) {
                ImVec2 a = w2s(wtl.x, wy), b = w2s(wbr.x, wy);
                dl->AddLine(a, b, IM_COL32(40, 40, 48, 255));
            }
        }
        // Major grid + labels
        for (float wx = gx0; wx <= wbr.x + gs; wx += gs) {
            ImVec2 a = w2s(wx, wtl.y), b = w2s(wx, wbr.y);
            dl->AddLine(a, b, IM_COL32(60, 60, 75, 255));
            if (a.x >= cv_p0.x + 4 && a.x <= cv_p1.x - 4) {
                snprintf(lbl, sizeof(lbl), gs < 1.0f ? "%.1f" : "%.0f", wx);
                ImVec2 ts = ImGui::CalcTextSize(lbl);
                dl->AddText(ImVec2(a.x - ts.x * 0.5f, cv_p1.y - fs - 2), IM_COL32(140, 140, 160, 220), lbl);
            }
        }
        for (float wy = gy0; wy >= wbr.y - gs; wy -= gs) {
            ImVec2 a = w2s(wtl.x, wy), b = w2s(wbr.x, wy);
            dl->AddLine(a, b, IM_COL32(60, 60, 75, 255));
            if (a.y >= cv_p0.y + 4 && a.y <= cv_p1.y - 4) {
                snprintf(lbl, sizeof(lbl), gs < 1.0f ? "%.1f" : "%.0f", wy);
                dl->AddText(ImVec2(cv_p0.x + 4, a.y - fs * 0.5f), IM_COL32(140, 140, 160, 220), lbl);
            }
        }
        // Origin axes
        {
            ImVec2 o = w2s(0, 0);
            if (o.x >= cv_p0.x && o.x <= cv_p1.x)
                dl->AddLine(ImVec2(o.x, cv_p0.y), ImVec2(o.x, cv_p1.y), IM_COL32(180, 60, 60, 200), 1.5f);
            if (o.y >= cv_p0.y && o.y <= cv_p1.y)
                dl->AddLine(ImVec2(cv_p0.x, o.y), ImVec2(cv_p1.x, o.y), IM_COL32(60, 180, 60, 200), 1.5f);
            ImVec2 ol = w2s(0, -0.5f);
            if (ol.x >= cv_p0.x && ol.x <= cv_p1.x - 20 && ol.y >= cv_p0.y && ol.y <= cv_p1.y - fs)
                dl->AddText(ImVec2(ol.x + 4, ol.y), IM_COL32(200, 200, 200, 180), "0");
        }

        // Object rendering
        int hovered_obj = -1;
        for (size_t i = 0; i < objects.size(); i++) {
            auto& obj = objects[i];
            if (!obj.is_object()) continue;
            double px = getNum(obj, "pos_x", 0);
            double py = getNum(obj, "pos_y", 0);
            std::string tp = getStr(obj, "type", "");
            std::string nm = getStr(obj, "name", "");
            ImVec2 ds = getDefaultSize(tp);
            double sx = getNum(obj, "size_x", ds.x);
            double sy = getNum(obj, "size_y", ds.y);
            if (tp == "point_light") { sx = getNum(obj, "radius", 0.5); sy = sx; }
            if (tp == "tileset") { sx = getNum(obj, "tile_width", 0.2) * getNum(obj, "atlas_cols", 4); sy = getNum(obj, "tile_height", 0.2) * getNum(obj, "atlas_rows", 4); }
            double wl = px, wb = py, wr = px + sx, wt = py + sy;
            if (tp == "sprite" || tp == "animated_sprite") {
                double oxx = getNum(obj, "origin_x", 0.5), oyy = getNum(obj, "origin_y", 0.5);
                wl = px - sx * oxx; wb = py - sy * oyy;
                wr = wl + sx; wt = wb + sy;
            }
            auto [sc_min, sc_max] = bbox(wl, wb, wr - wl, wt - wb);
            ImU32 col = getTypeColor(tp);
            bool sel = ((int)i == tab.scene_selected_object);
            ImVec2 cen = w2s((float)px, (float)py);

            if (tp == "point_light") {
                float r = (float)sx * z;
                dl->AddCircleFilled(cen, r, IM_COL32(255, 200, 80, 12));
                dl->AddCircle(cen, r, IM_COL32(255, 200, 80, 60), 0, 2.0f * gz);
                dl->AddCircleFilled(cen, 5 * gz, col);
            } else if (tp == "ambient_light") {
                float r = (float)sx * z;
                dl->AddCircleFilled(cen, r, IM_COL32(255, 240, 140, 8));
                dl->AddCircle(cen, r, IM_COL32(255, 240, 140, 25), 0, 1.0f * gz);
            } else if (tp == "camera") {
                float szz = 20 * gz;
                dl->AddTriangleFilled(ImVec2(cen.x - szz, cen.y - szz), ImVec2(cen.x + szz, cen.y - szz), ImVec2(cen.x, cen.y + szz), col);
                dl->AddTriangle(ImVec2(cen.x - szz, cen.y - szz), ImVec2(cen.x + szz, cen.y - szz), ImVec2(cen.x, cen.y + szz), IM_COL32(255, 255, 255, 100), 1.5f * gz);
            } else if (tp == "particle_system") {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddCircleFilled(w2s((float)(px + sx * 0.3), (float)(py + sy * 0.3)), 3 * gz, IM_COL32(255, 255, 255, 180));
                dl->AddCircleFilled(w2s((float)(px + sx * 0.7), (float)(py + sy * 0.7)), 3 * gz, IM_COL32(255, 255, 255, 180));
                dl->AddCircleFilled(w2s((float)(px + sx * 0.5), (float)(py + sy * 0.3)), 2 * gz, IM_COL32(255, 255, 255, 180));
            } else if (tp == "text") {
                std::string ttxt = getStr(obj, "text", "");
                ImU32 tcol = IM_COL32(80, 220, 80, 60);
                dl->AddRectFilled(sc_min, sc_max, tcol);
                if (!ttxt.empty()) {
                    if (m_textInitialized) {
                        BlazeBolt::Text2D* textObj = new BlazeBolt::Text2D(m_quadVBO, m_font);
                        textObj->setPosition((float)px, (float)py);
                        textObj->setScale(
                            (float)getNum(obj, "scale_x", 1.0),
                            (float)getNum(obj, "scale_y", 1.0)
                        );
                        textObj->setOrigin(
                            (float)getNum(obj, "origin_x", 0.0),
                            (float)getNum(obj, "origin_y", 0.0)
                        );
                        textObj->setColor(
                            (float)getNum(obj, "color_r", 1.0),
                            (float)getNum(obj, "color_g", 1.0),
                            (float)getNum(obj, "color_b", 1.0),
                            (float)getNum(obj, "color_a", 1.0)
                        );
                        int align = (int)getNum(obj, "alignment", 0);
                        if (align == 1) textObj->setAlignment(BlazeBolt::Text2D::Alignment::Center);
                        else if (align == 2) textObj->setAlignment(BlazeBolt::Text2D::Alignment::Right);
                        else textObj->setAlignment(BlazeBolt::Text2D::Alignment::Left);
                        textObj->setText(ttxt);
                        textObj->setVisible(true);

                        float ar = cv_sz.x / cv_sz.y;
                        float wx_min = (cv_p0.x - ox) / z;
                        float wx_max = (cv_p1.x - ox) / z;
                        float wy_min = -(cv_p1.y - oy) / z;
                        float wy_max = -(cv_p0.y - oy) / z;

                        Matrix3x3 projView;
                        projView.m[0][0] = 2.0f * ar / (wx_max - wx_min);
                        projView.m[1][1] = 2.0f / (wy_max - wy_min);
                        projView.m[2][0] = -ar * (wx_max + wx_min) / (wx_max - wx_min);
                        projView.m[2][1] = -(wy_max + wy_min) / (wy_max - wy_min);

                        struct TextCB {
                            BlazeBolt::Text2D* text;
                            BlazeBolt::FontShader2D* shader;
                            Matrix3x3 pv;
                            float aspect;
                            ImVec2 cm;
                            ImVec2 cM;
                        };
                        TextCB* cb = new TextCB{textObj, &m_fontShader, projView, ar, cv_p0, cv_p1};
                        dl->AddCallback([](const ImDrawList*, const ImDrawCmd* cmd) {
                            TextCB* d = (TextCB*)cmd->UserCallbackData;
                            GLint prevVp[4], prevVAO, prevProg;
                            glGetIntegerv(GL_VIEWPORT, prevVp);
                            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
                            glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);
                            glViewport((GLint)d->cm.x, (GLint)(ImGui::GetIO().DisplaySize.y - d->cM.y),
                                       (GLsizei)(d->cM.x - d->cm.x), (GLsizei)(d->cM.y - d->cm.y));
                            d->shader->bind();
                            d->shader->setAspectRatio(d->aspect);
                            d->text->draw(*d->shader, d->pv);
                            glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
                            glBindVertexArray(prevVAO);
                            glUseProgram(prevProg);
                            delete d->text;
                            delete d;
                        }, cb);
                        dl->AddCallback([](const ImDrawList*, const ImDrawCmd*) {}, nullptr);
                    } else {
                        ImVec2 ts = ImGui::CalcTextSize(ttxt.c_str());
                        float s = std::min((sc_max.x - sc_min.x) / std::max(ts.x, 1.0f),
                                           (sc_max.y - sc_min.y) / std::max(ts.y, 1.0f));
                        s = std::min(s, 1.0f) * gz;
                        ImVec2 tp = ImVec2(
                            cen.x - ts.x * s * 0.5f,
                            cen.y - ts.y * s * 0.5f
                        );
                        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * s, tp,
                                    IM_COL32(80, 220, 80, 220), ttxt.c_str());
                    }
                } else {
                    const char* placeholder = "[Text]";
                    ImVec2 ts = ImGui::CalcTextSize(placeholder);
                    float s = std::min((sc_max.x - sc_min.x) / std::max(ts.x, 1.0f),
                                       (sc_max.y - sc_min.y) / std::max(ts.y, 1.0f));
                    s = std::min(s, 1.0f) * gz;
                    ImVec2 tp = ImVec2(
                        cen.x - ts.x * s * 0.5f,
                        cen.y - ts.y * s * 0.5f
                    );
                    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * s, tp,
                                IM_COL32(80, 220, 80, 180), placeholder);
                }
            } else if (tp == "tileset") {
                dl->AddRectFilled(sc_min, sc_max, col);
                float tw = (float)getNum(obj, "tile_width", 0.2), th = (float)getNum(obj, "tile_height", 0.2);
                int ac = (int)getNum(obj, "atlas_cols", 4), ar = (int)getNum(obj, "atlas_rows", 4);
                for (int ri = 0; ri < ar; ri++) for (int ci = 0; ci < ac; ci++) {
                    ImVec2 t_tl = w2s((float)(wl + ci * tw), (float)(wb + ri * th));
                    ImVec2 t_br = w2s((float)(wl + (ci + 1) * tw), (float)(wb + (ri + 1) * th));
                    dl->AddRect(t_tl, t_br, IM_COL32(0, 0, 0, 80), 0, 0, 1.0f * gz);
                }
            } else if (tp == "sprite" || tp == "animated_sprite") {
                std::string tex_path = getStr(obj, "texture", "");
                ImTextureID tex_id = (ImTextureID)0;
                bool tex_ok = false;
                if (!tex_path.empty()) {
                    auto it = tab.scene_texture_cache.find(tex_path);
                    if (it != tab.scene_texture_cache.end()) {
                        tex_id = it->second.id;
                        tex_ok = true;
                    } else {
                        fs::path scene_dir = fs::path(tab.file_path).parent_path();
                        fs::path tex_full = scene_dir / tex_path;
                        if (!fs::exists(tex_full))
                            tex_full = m_project_path / "engine" / tex_path;
                        if (!fs::exists(tex_full))
                            tex_full = m_project_path / tex_path;
                        if (fs::exists(tex_full)) {
                            int w, h, ch;
                            unsigned char* data = stbi_load(tex_full.string().c_str(), &w, &h, &ch, 4);
                            if (data) {
                                GLuint gltex;
                                glGenTextures(1, &gltex);
                                glBindTexture(GL_TEXTURE_2D, gltex);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                                stbi_image_free(data);
                                tex_id = (ImTextureID)(intptr_t)gltex;
                                tab.scene_texture_cache[tex_path] = {tex_id, w, h};
                                tex_ok = true;
                            }
                        }
                    }
                }
                if (tex_ok) {
                    double tu = 0, tv = 0, tw = 1, th = 1;
                    if (obj.is_object() && obj.contains("texture_rect") && obj["texture_rect"].is_array()) {
                        auto& arr = obj["texture_rect"].get<crude_json::array>();
                        if (arr.size() >= 4) {
                            tu = arr[0].get<crude_json::number>();
                            tv = arr[1].get<crude_json::number>();
                            tw = arr[2].get<crude_json::number>();
                            th = arr[3].get<crude_json::number>();
                        }
                    } else {
                        tu = getNum(obj, "texture_u", 0);
                        tv = getNum(obj, "texture_v", 0);
                        tw = getNum(obj, "texture_w", 1);
                        th = getNum(obj, "texture_h", 1);
                    }
                    float cr = (float)getNum(obj, "color_r", 1);
                    float cg = (float)getNum(obj, "color_g", 1);
                    float cb = (float)getNum(obj, "color_b", 1);
                    float ca = (float)getNum(obj, "color_a", 1);
                    dl->AddImage(tex_id, sc_min, sc_max,
                        ImVec2((float)tu, (float)tv),
                        ImVec2((float)(tu + tw), (float)(tv + th)),
                        IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), (int)(ca*255)));
                    dl->AddRect(sc_min, sc_max,
                        sel ? IM_COL32(255, 200, 50, 255) : col, 0, 0,
                        sel ? (2.0f * gz) : (1.0f * gz));
                } else {
                    dl->AddRectFilled(sc_min, sc_max, col);
                    dl->AddLine(ImVec2(sc_min.x, sc_min.y), ImVec2(sc_max.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * gz);
                    dl->AddLine(ImVec2(sc_max.x, sc_min.y), ImVec2(sc_min.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * gz);
                }
                if (tp == "animated_sprite") dl->AddCircleFilled(cen, 4 * gz, IM_COL32(255, 255, 255, 120));
            } else {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddLine(ImVec2(sc_min.x, sc_min.y), ImVec2(sc_max.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * gz);
                dl->AddLine(ImVec2(sc_max.x, sc_min.y), ImVec2(sc_min.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * gz);
            }
            // Physics body wireframe
            if (obj.is_object() && obj.contains("body_type") && obj["body_type"].is_number()) {
                ImVec2 pc = w2s((float)px, (float)py);
                std::string collider = getStr(obj, "collider_shape", "circle");
                ImU32 phcol = IM_COL32(0, 200, 255, 200);
                float phw = std::max(1.5f, 2.0f * gz);
                if (collider == "circle") {
                    double cr = getNum(obj, "circle_radius", 32.0);
                    double cox = getNum(obj, "circle_offset_x", 0.0);
                    double coy = getNum(obj, "circle_offset_y", 0.0);
                    ImVec2 cc = w2s((float)(px + cox), (float)(py + coy));
                    float rr = (float)cr * z;
                    dl->AddCircle(cc, rr, phcol, 0, phw);
                    dl->AddLine(cc, ImVec2(cc.x + rr, cc.y), phcol, 1.0f * gz);
                } else {
                    double hw = getNum(obj, "rect_half_width", 32.0);
                    double hh = getNum(obj, "rect_half_height", 32.0);
                    double rox = getNum(obj, "rect_offset_x", 0.0);
                    double roy = getNum(obj, "rect_offset_y", 0.0);
                    double rl = px + rox - hw, rb = py + roy - hh;
                    auto [rmin, rmax] = bbox(rl, rb, hw * 2, hh * 2);
                    dl->AddRect(rmin, rmax, phcol, 0, 0, phw);
                }
            }
            // Name label
            if (z > 0.3f) {
                ImVec2 lp = ImVec2(sc_min.x, sc_max.y + 2 * gz);
                dl->AddText(ImVec2(lp.x + 1, lp.y + 1), IM_COL32(0, 0, 0, 180), nm.c_str());
                dl->AddText(lp, IM_COL32(220, 220, 230, 220), nm.c_str());
            }
            // Gizmo
            if (sel && gz > 0.05f) {
                float pd = 5 * gz;
                ImU32 oc_col = IM_COL32(255, 200, 50, 255);
                float cx = (float)(wl + wr) * 0.5f, cy = (float)(wb + wt) * 0.5f;
                ImVec2 oc = w2s(cx, cy);
                dl->AddRect(ImVec2(sc_min.x - pd, sc_min.y - pd), ImVec2(sc_max.x + pd, sc_max.y + pd), oc_col, 0, 0, 2.0f * gz);
                if (tab.scene_gizmo_mode == 0) {
                    float al = 36 * gz, hl = 10 * gz, hw = 6 * gz;
                    ImVec2 xe = ImVec2(oc.x + al, oc.y);
                    ImU32 xc = (tab.scene_drag_handle == 10) ? IM_COL32(255, 230, 50, 255) : IM_COL32(220, 70, 70, 255);
                    dl->AddLine(oc, xe, xc, 3);
                    dl->AddTriangleFilled(ImVec2(xe.x + hl, oc.y), ImVec2(xe.x, oc.y - hw), ImVec2(xe.x, oc.y + hw), xc);
                    ImVec2 ye = ImVec2(oc.x, oc.y - al);
                    ImU32 yc = (tab.scene_drag_handle == 11) ? IM_COL32(255, 230, 50, 255) : IM_COL32(70, 180, 70, 255);
                    dl->AddLine(oc, ye, yc, 3);
                    dl->AddTriangleFilled(ImVec2(oc.x, ye.y - hl), ImVec2(oc.x - hw, ye.y), ImVec2(oc.x + hw, ye.y), yc);
                    float cs = 5 * gz;
                    ImU32 cc = (tab.scene_drag_handle == 9) ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 230);
                    dl->AddRectFilled(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), cc);
                    dl->AddRect(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), IM_COL32(60, 60, 60, 200));
                } else if (tab.scene_gizmo_mode == 1) {
                    float rr = std::max((float)sx, (float)sy) * 0.5f * z + 16 * gz;
                    dl->AddCircle(oc, rr, IM_COL32(80, 180, 255, 180), 0, 2.0f * gz);
                    double ang = getNum(obj, "rot", 0);
                    float rad = (float)(ang * 3.141592653589793 / 180.0);
                    ImVec2 hp = ImVec2(oc.x + cosf(rad) * rr, oc.y - sinf(rad) * rr);
                    float hr = 6 * gz;
                    ImU32 hc = (tab.scene_drag_handle == 8) ? IM_COL32(255, 230, 50, 255) : IM_COL32(80, 200, 255, 230);
                    dl->AddLine(oc, hp, IM_COL32(80, 180, 255, 120), 1.5f * gz);
                    dl->AddCircleFilled(hp, hr, hc);
                    dl->AddCircle(hp, hr, oc_col, 0, 1.5f * gz);
                    float il = std::max((float)sx, (float)sy) * 0.5f * z + 12 * gz;
                    ImVec2 dir = ImVec2(cosf(rad) * il, -sinf(rad) * il);
                    dl->AddLine(oc, ImVec2(oc.x + dir.x, oc.y + dir.y), IM_COL32(255, 200, 50, 200), 2.0f * gz);
                    dl->AddCircleFilled(ImVec2(oc.x + dir.x, oc.y + dir.y), 3 * gz, IM_COL32(255, 200, 50, 220));
                } else if (tab.scene_gizmo_mode == 2) {
                    float hs = 8 * gz, hhs = hs * 0.5f, spd = pd;
                    ImVec2 cor[4] = {
                        ImVec2(sc_min.x - spd - hhs, sc_min.y - spd - hhs),
                        ImVec2(sc_max.x + spd - hhs, sc_min.y - spd - hhs),
                        ImVec2(sc_max.x + spd - hhs, sc_max.y + spd - hhs),
                        ImVec2(sc_min.x - spd - hhs, sc_max.y + spd - hhs)
                    };
                    ImVec2 edg[4] = {
                        ImVec2((sc_min.x + sc_max.x) * 0.5f - hhs, sc_min.y - spd - hhs),
                        ImVec2(sc_max.x + spd - hhs, (sc_min.y + sc_max.y) * 0.5f - hhs),
                        ImVec2((sc_min.x + sc_max.x) * 0.5f - hhs, sc_max.y + spd - hhs),
                        ImVec2(sc_min.x - spd - hhs, (sc_min.y + sc_max.y) * 0.5f - hhs)
                    };
                    for (int ci = 0; ci < 4; ci++) dl->AddLine(oc, ImVec2(cor[ci].x + hhs, cor[ci].y + hhs), IM_COL32(200, 200, 200, 80), 1.0f * gz);
                    for (int ci = 0; ci < 4; ci++) dl->AddLine(oc, ImVec2(edg[ci].x + hhs, edg[ci].y + hhs), IM_COL32(200, 200, 200, 60), 1.0f * gz);
                    for (int ci = 0; ci < 4; ci++) {
                        ImU32 c = (tab.scene_drag_handle == ci) ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 230);
                        dl->AddRectFilled(cor[ci], ImVec2(cor[ci].x + hs, cor[ci].y + hs), c);
                        dl->AddRect(cor[ci], ImVec2(cor[ci].x + hs, cor[ci].y + hs), IM_COL32(100, 100, 100, 200));
                    }
                    for (int ci = 0; ci < 4; ci++) {
                        ImU32 c = (tab.scene_drag_handle == ci + 4) ? IM_COL32(255, 230, 50, 255) : IM_COL32(200, 200, 220, 230);
                        dl->AddRectFilled(edg[ci], ImVec2(edg[ci].x + hs, edg[ci].y + hs), c);
                        dl->AddRect(edg[ci], ImVec2(edg[ci].x + hs, edg[ci].y + hs), IM_COL32(100, 100, 100, 200));
                    }
                    float cs = 5 * gz;
                    ImU32 cc = (tab.scene_drag_handle == 9) ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 200);
                    dl->AddRectFilled(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), cc);
                    dl->AddRect(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), IM_COL32(60, 60, 60, 200));
                }
            }
            // Hover test
            if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(cv_p0, cv_p1)) {
                ImVec2 mw = s2w(io.MousePos.x, io.MousePos.y);
                if (mw.x >= wl && mw.x <= wr && mw.y >= wb && mw.y <= wt) hovered_obj = (int)i;
            }
        }

        // Viewport interaction
        if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(cv_p0, cv_p1)) {
            if (io.MouseWheel != 0.0f) {
                tab.zoom *= (io.MouseWheel > 0) ? 1.1f : 0.9f;
                tab.zoom = std::max(0.05f, std::min(tab.zoom, 20.0f));
                z = tab.zoom * PPM;
                ImVec2 mw = s2w(io.MousePos.x, io.MousePos.y);
                ox = io.MousePos.x - mw.x * z;
                oy = io.MousePos.y - mw.y * z;
                tab.pan_x = ox - (cv_p0.x + cv_sz.x * 0.5f);
                tab.pan_y = oy - (cv_p0.y + cv_sz.y * 0.5f);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) { tab.pan_x += io.MouseDelta.x; tab.pan_y += io.MouseDelta.y; }
            if (ImGui::IsKeyPressed(ImGuiKey_W)) { tab.scene_gizmo_mode = 0; tab.scene_drag_handle = -1; }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) { tab.scene_gizmo_mode = 1; tab.scene_drag_handle = -1; }
            if (ImGui::IsKeyPressed(ImGuiKey_R)) { tab.scene_gizmo_mode = 2; tab.scene_drag_handle = -1; }

            int sel = tab.scene_selected_object;
            bool on_handle = false;

            if (sel >= 0 && sel < (int)objects.size()) {
                auto& sobj = objects[sel];
                if (sobj.is_object()) {
                    std::string st = getStr(sobj, "type", "");
                    ImVec2 ssz = getDefaultSize(st);
                    double spx = getNum(sobj, "pos_x", 0);
                    double spy = getNum(sobj, "pos_y", 0);
                    double ssx = getNum(sobj, "size_x", ssz.x);
                    double ssy = getNum(sobj, "size_y", ssz.y);
                if (st == "point_light") { ssx = getNum(sobj, "radius", 0.5); ssy = ssx; }
                    if (st == "tileset") { ssx = getNum(sobj, "tile_width", 0.2) * getNum(sobj, "atlas_cols", 4); ssy = getNum(sobj, "tile_height", 0.2) * getNum(sobj, "atlas_rows", 4); }
                    double swl = spx, swb = spy, swr = spx + ssx, swt = spy + ssy;
                    if (st == "sprite" || st == "animated_sprite") {
                        double sorx = getNum(sobj, "origin_x", 0.5), sory = getNum(sobj, "origin_y", 0.5);
                        swl = spx - ssx * sorx; swb = spy - ssy * sory;
                        swr = swl + ssx; swt = swb + ssy;
                    }
                    auto [ssmin, ssmax] = bbox(swl, swb, swr - swl, swt - swb);
                    float spd = 5 * gz;
                    float scx = (float)(swl + swr) * 0.5f, scy = (float)(swb + swt) * 0.5f;
                    ImVec2 soc = w2s(scx, scy);
                    ImVec2 ms = io.MousePos;

                    if (tab.scene_drag_handle >= 0) {
                        on_handle = true;
                        ImVec2 mw = s2w(ms.x, ms.y);
                        if (tab.scene_gizmo_mode == 0) {
                            if (tab.scene_drag_handle == 10) {
                                double dx = mw.x - tab.scene_drag_vx;
                                setNum(sobj, "pos_x", (tab.scene_drag_vx + dx));
                            } else if (tab.scene_drag_handle == 11) {
                                double dy = mw.y - tab.scene_drag_vy;
                                setNum(sobj, "pos_y", (tab.scene_drag_vy + dy));
                            } else {
                                setNum(sobj, "pos_x", (spx + io.MouseDelta.x / z));
                                setNum(sobj, "pos_y", (spy - io.MouseDelta.y / z));
                            }
                            tab.modified = true;
                        } else if (tab.scene_gizmo_mode == 1) {
                            float dx = mw.x - scx, dy = mw.y - scy;
                            setNum(sobj, "rot", atan2(dy, dx) * 180.0 / 3.141592653589793);
                            tab.modified = true;
                        } else if (tab.scene_gizmo_mode == 2) {
                            double nwl = tab.scene_drag_wl, nwb = tab.scene_drag_wb;
                            double nwr = tab.scene_drag_wr, nwt = tab.scene_drag_wt;
                            if (tab.scene_drag_handle == 9) {
                                double dx = io.MouseDelta.x / z, dy = -io.MouseDelta.y / z;
                                double avg = (dx + dy) * 0.5;
                                double nsx = tab.scene_drag_vsx + avg, nsy = tab.scene_drag_vsy + avg;
                                if (nsx < 0.1) nsx = 0.1; if (nsy < 0.1) nsy = 0.1;
                                double sorx = getNum(sobj, "origin_x", 0.5), sory = getNum(sobj, "origin_y", 0.5);
                                double wlo = tab.scene_drag_vx - tab.scene_drag_vsx * sorx;
                                double wbo = tab.scene_drag_vy - tab.scene_drag_vsy * sory;
                                setNum(sobj, "pos_x", (wlo + nsx * sorx));
                                setNum(sobj, "pos_y", (wbo + nsy * sory));
                                setNum(sobj, "size_x", nsx); setNum(sobj, "size_y", nsy);
                            } else {
                                switch (tab.scene_drag_handle) {
                                    case 0: nwl = mw.x; nwt = mw.y; break;
                                    case 1: nwr = mw.x; nwt = mw.y; break;
                                    case 2: nwr = mw.x; nwb = mw.y; break;
                                    case 3: nwl = mw.x; nwb = mw.y; break;
                                    case 4: nwt = mw.y; break;
                                    case 5: nwr = mw.x; break;
                                    case 6: nwb = mw.y; break;
                                    case 7: nwl = mw.x; break;
                                }
                                double mn = 0.1; int dh = tab.scene_drag_handle;
                                if ((dh == 1 || dh == 2 || dh == 5) && nwr - nwl < mn) nwr = nwl + mn;
                                else if ((dh == 0 || dh == 3 || dh == 7) && nwr - nwl < mn) nwl = nwr - mn;
                                if ((dh == 0 || dh == 1 || dh == 4) && nwt - nwb < mn) nwt = nwb + mn;
                                else if ((dh == 2 || dh == 3 || dh == 6) && nwt - nwb < mn) nwb = nwt - mn;
                                double nsx = nwr - nwl, nsy = nwt - nwb;
                                double sorx = getNum(sobj, "origin_x", 0.5), sory = getNum(sobj, "origin_y", 0.5);
                                double npx = nwl + nsx * sorx, npy = nwb + nsy * sory;
                                setNum(sobj, "pos_x", npx); setNum(sobj, "pos_y", npy);
                                setNum(sobj, "size_x", nsx); setNum(sobj, "size_y", nsy);
                            }
                            tab.modified = true;
                        }
                        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) tab.scene_drag_handle = -1;
                    } else {
                        float hsz = 8 * gz;
                        if (tab.scene_gizmo_mode == 0) {
                            float al = 36 * gz, aw = 8 * gz, cs = 7 * gz;
                            float ax1 = soc.x - 5 * gz, ax2 = soc.x + al + 10 * gz, ay1 = soc.y - aw, ay2 = soc.y + aw;
                            float yx1 = soc.x - aw, yx2 = soc.x + aw, yy1 = soc.y - al - 10 * gz, yy2 = soc.y + 5 * gz;
                            bool on_x = (ms.x >= ax1 && ms.x <= ax2 && ms.y >= ay1 && ms.y <= ay2);
                            bool on_y = (ms.x >= yx1 && ms.x <= yx2 && ms.y >= yy1 && ms.y <= yy2);
                            bool on_ct = (ms.x >= soc.x - cs && ms.x <= soc.x + cs && ms.y >= soc.y - cs && ms.y <= soc.y + cs);
                            if (on_x) { on_handle = true; if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 10; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; } }
                            else if (on_y) { on_handle = true; if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 11; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; } }
                            else if (on_ct) { on_handle = true; if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 9; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; } }
                        } else if (tab.scene_gizmo_mode == 1) {
                            float rr = std::max((float)ssx, (float)ssy) * 0.5f * z + 16 * gz;
                            double ang = getNum(sobj, "rot", 0);
                            float rad = (float)(ang * 3.141592653589793 / 180.0);
                            float hx = soc.x + cosf(rad) * rr, hy = soc.y - sinf(rad) * rr, hr = 8 * gz;
                            if (ms.x >= hx - hr && ms.x <= hx + hr && ms.y >= hy - hr && ms.y <= hy + hr) {
                                on_handle = true;
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 8; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; }
                            }
                        } else if (tab.scene_gizmo_mode == 2) {
                            float shs = hsz * 0.5f;
                            ImVec2 sc[9] = {
                                ImVec2(ssmin.x - spd - shs, ssmin.y - spd - shs),
                                ImVec2(ssmax.x + spd - shs, ssmin.y - spd - shs),
                                ImVec2(ssmax.x + spd - shs, ssmax.y + spd - shs),
                                ImVec2(ssmin.x - spd - shs, ssmax.y + spd - shs),
                                ImVec2((ssmin.x + ssmax.x) * 0.5f - shs, ssmin.y - spd - shs),
                                ImVec2(ssmax.x + spd - shs, (ssmin.y + ssmax.y) * 0.5f - shs),
                                ImVec2((ssmin.x + ssmax.x) * 0.5f - shs, ssmax.y + spd - shs),
                                ImVec2(ssmin.x - spd - shs, (ssmin.y + ssmax.y) * 0.5f - shs),
                                ImVec2(soc.x - 7 * gz, soc.y - 7 * gz)
                            };
                            for (int hi = 0; hi < 9; hi++) {
                                int hid = (hi == 8) ? 9 : hi;
                                ImVec2 hm = sc[hi];
                                ImVec2 hx = (hi == 8) ? ImVec2(hm.x + 14 * gz, hm.y + 14 * gz) : ImVec2(hm.x + hsz, hm.y + hsz);
                                if (ms.x >= hm.x && ms.x <= hx.x && ms.y >= hm.y && ms.y <= hx.y) {
                                    on_handle = true;
                                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                        tab.scene_drag_handle = hid;
                                        tab.scene_drag_wl = swl; tab.scene_drag_wb = swb;
                                        tab.scene_drag_wr = swr; tab.scene_drag_wt = swt;
                                        tab.scene_drag_vx = spx; tab.scene_drag_vy = spy;
                                        tab.scene_drag_vsx = ssx; tab.scene_drag_vsy = ssy;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (!on_handle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                tab.scene_selected_object = (hovered_obj >= 0) ? hovered_obj : -1;
            }
            if (!on_handle && tab.scene_drag_handle < 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)
                && sel >= 0 && sel < (int)objects.size()) {
                auto& mobj = objects[sel];
                if (mobj.is_object()) {
                    double oxx = getNum(mobj, "pos_x", 0);
                    double oyy = getNum(mobj, "pos_y", 0);
                    setNum(mobj, "pos_x", oxx + (io.MouseDelta.x / z));
                    setNum(mobj, "pos_y", oyy - (io.MouseDelta.y / z));
                    tab.modified = true;
                }
            }
            if (tab.scene_drag_handle >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) tab.scene_drag_handle = -1;
        }

        // Tooltip
        if (hovered_obj >= 0 && hovered_obj < (int)objects.size()) {
            auto& obj = objects[hovered_obj];
            std::string nm = getStr(obj, "name", "Unnamed");
            std::string tp = getStr(obj, "type", "?");
            ImVec2 ds = getDefaultSize(tp);
            double px = getNum(obj, "pos_x", 0), py = getNum(obj, "pos_y", 0);
            double sx = getNum(obj, "size_x", ds.x), sy = getNum(obj, "size_y", ds.y);
            double rot = getNum(obj, "rot", 0);
            ImGui::SetTooltip("[%s] %s\npos: %.2f, %.2f  size: %.2f x %.2f  rot: %.1f\xC2\xB0",
                tp.c_str(), nm.c_str(), px, py, sx, sy, rot);
        }
    }
    ImGui::EndChild();
}

bool Editor::ShouldReturnToHub() const { return m_return_to_hub; }
void Editor::ResetReturnFlag() { m_return_to_hub = false; }
