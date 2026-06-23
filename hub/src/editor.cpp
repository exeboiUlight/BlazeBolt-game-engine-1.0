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
{
    LoadTheme();
    ApplyTheme(m_current_theme);
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
    }
}

void Editor::FMCreateFolder(const std::string& name) {
    fs::create_directories(m_fm_current_dir / name);
    RefreshFM();
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

bool Editor::IsCodeFile(const std::string& ext) {
    return ext == ".lua" || ext == ".luau" || ext == ".txt" || ext == ".json" ||
           ext == ".scene" || ext == ".xml" || ext == ".yaml" || ext == ".yml" || ext == ".cfg" ||
           ext == ".ini" || ext == ".cpp" || ext == ".h" || ext == ".hpp" ||
           ext == ".c" || ext == ".py" || ext == ".js" || ext == ".ts" ||
           ext == ".md" || ext == ".cmake" || ext == ".toml";
}

void Editor::OpenFileInTab(const std::string& path) {
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].file_path == path) {
            m_active_tab = i;
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
    m_active_tab = (int)m_tabs.size() - 1;
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
    if (m_active_tab >= (int)m_tabs.size()) m_active_tab = (int)m_tabs.size() - 1;
    if (m_active_tab == idx && m_active_tab > 0) m_active_tab--;
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

    // DockSpace (always fills below menu bar)
    RenderDockSpace();

    // Always-visible dockable windows
    RenderFileBrowserWindow();
    RenderSceneViewportWindow();
    RenderSceneHierarchyWindow();
    RenderSceneInspectorWindow();
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
        if (ImGui::MenuItem("Save", "Ctrl+S", false, m_active_tab >= 0)) SaveTab(m_tabs[m_active_tab]);
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
        ImGui::MenuItem("Scene Viewport", nullptr, &m_show_scene_viewport);
        ImGui::MenuItem("Scene Hierarchy", nullptr, &m_show_scene_hierarchy);
        ImGui::MenuItem("Scene Inspector", nullptr, &m_show_scene_inspector);
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

void Editor::RenderFileBrowserWindow() {
    if (!m_show_file_browser) return;

    ImGui::Begin("File Browser", &m_show_file_browser);

    if (m_fm_current_dir != m_fm_root) {
        if (ImGui::Button("..", ImVec2(30, 0))) FMNavigateUp();
    } else {
        ImGui::Dummy(ImVec2(30, 0));
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Refresh")) RefreshFM();
    ImGui::SameLine();
    if (ImGui::SmallButton("+ New")) ImGui::OpenPopup("FM_NewItem");

    if (ImGui::BeginPopup("FM_NewItem")) {
        static char new_name_buf[256] = "";
        ImGui::InputText("Name", new_name_buf, sizeof(new_name_buf));
        if (ImGui::MenuItem("Create File")) { if (new_name_buf[0]) { FMCreateFile(new_name_buf); new_name_buf[0] = '\0'; } }
        if (ImGui::MenuItem("Create Folder")) { if (new_name_buf[0]) { FMCreateFolder(new_name_buf); new_name_buf[0] = '\0'; } }
        ImGui::EndPopup();
    }

    ImGui::Checkbox("Hidden", &m_fm_show_hidden);
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "%s", m_fm_current_dir.filename().string().c_str());
    ImGui::Separator();

    ImGui::BeginChild("##FMList", ImVec2(0, 0));
    for (int i = 0; i < (int)m_fm_entries.size(); i++) {
        auto& entry = m_fm_entries[i];
        std::string name = entry.path().filename().string();
        bool is_dir = entry.is_directory();
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        ImGui::PushID(i);
        if (is_dir) ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.3f, 1.0f), "%s", ">>");
        else if (IsImageFile(ext)) ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", "[]");
        else if (IsCodeFile(ext)) ImGui::TextColored(ImVec4(0.4f, 0.6f, 1.0f, 1.0f), "%s", "{}");
        else ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", "--");
        ImGui::SameLine(30.0f);

        if (m_fm_renaming_idx == i) {
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##rn", m_fm_rename_buf, sizeof(m_fm_rename_buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                FMRename(i, m_fm_rename_buf);
                m_fm_renaming_idx = -1;
            }
            if (ImGui::IsItemDeactivated()) m_fm_renaming_idx = -1;
        } else {
            if (ImGui::Selectable(name.c_str(), false)) FMOpenFile(entry.path().string());
        }

        if (ImGui::BeginPopupContextItem()) {
            if (is_dir) { if (ImGui::MenuItem("Open")) FMOpenFile(entry.path().string()); }
            else { if (ImGui::MenuItem("Open in Editor")) OpenFileInTab(entry.path().string()); }
            ImGui::Separator();
            if (ImGui::MenuItem("Rename")) { m_fm_renaming_idx = i; strncpy(m_fm_rename_buf, name.c_str(), sizeof(m_fm_rename_buf) - 1); }
            if (ImGui::MenuItem("Copy")) FMCopy(entry.path().string());
            if (ImGui::MenuItem("Cut")) FMCut(entry.path().string());
            if (!m_clipboard.path.empty() && ImGui::MenuItem("Paste Here")) FMPaste();
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) { fs::remove_all(entry.path()); RefreshFM(); ImGui::EndPopup(); ImGui::PopID(); break; }
            if (ImGui::MenuItem("Show in Explorer")) {
                system(("explorer /select,\"" + entry.path().string() + "\"").c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::End();
}

// ======================== SCENE VIEWPORT WINDOW ========================

void Editor::RenderSceneViewportWindow() {
    if (!m_show_scene_viewport) return;

    ImGui::Begin("Scene Viewport", &m_show_scene_viewport);

    int scene_tab_idx = -1;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].type == EditorTabType::Scene) { scene_tab_idx = i; break; }
    }

    if (scene_tab_idx < 0) {
        float avail = ImGui::GetContentRegionAvail().y;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a .scene file to edit");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    auto& tab = m_tabs[scene_tab_idx];
    m_active_tab = scene_tab_idx;
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveTab(tab);
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W)) { CloseTab(m_active_tab); ImGui::End(); return; }

    const char* gizmo_names[3] = { "Move", "Rotate", "Scale" };
    if (ImGui::Button("Save")) SaveTab(tab);
    ImGui::SameLine();
    if (ImGui::Button("Fit##vp")) { tab.pan_x = 0; tab.pan_y = 0; tab.zoom = 1.0f; }
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    for (int gm = 0; gm < 3; gm++) {
        if (gm > 0) ImGui::SameLine(0, 2);
        bool active = (tab.scene_gizmo_mode == gm);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.50f, 0.85f, 1));
        if (ImGui::Button(gizmo_names[gm])) { tab.scene_gizmo_mode = gm; tab.scene_drag_handle = -1; }
        if (active) ImGui::PopStyleColor();
    }
    ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();
    ImGui::Text("  [%c] %s  |  Zoom: %.0f%%",
        "WER"[tab.scene_gizmo_mode], gizmo_names[tab.scene_gizmo_mode], tab.zoom * 100);
    ImGui::Separator();

    // --- Canvas setup ---
    auto& doc = tab.scene_json;
    auto& objects = [&]() -> crude_json::array& {
        static crude_json::array empty;
        if (doc.is_object() && doc.contains("objects") && doc["objects"].is_array())
            return doc["objects"].get<crude_json::array>();
        return empty;
    }();

    auto getNum = [](crude_json::value& v, const std::string& k, double d) {
        return (v.is_object() && v.contains(k) && v[k].is_number()) ? v[k].get<crude_json::number>() : d;
    };
    auto getStr = [](crude_json::value& v, const std::string& k, const std::string& d) {
        return (v.is_object() && v.contains(k) && v[k].is_string()) ? v[k].get<crude_json::string>() : d;
    };
    auto setNum = [](crude_json::value& v, const std::string& k, double val) {
        if (v.is_object()) v[k] = (crude_json::number)val;
    };

    auto sizeFor = [](const std::string& t) -> ImVec2 {
        if (t == "sprite" || t == "animated_sprite") return ImVec2(64, 64);
        if (t == "text")   return ImVec2(128, 24);
        if (t == "camera") return ImVec2(32, 32);
        if (t == "point_light")       return ImVec2(48, 48);
        if (t == "ambient_light")     return ImVec2(160, 160);
        if (t == "particle_system")   return ImVec2(48, 48);
        if (t == "tileset")           return ImVec2(128, 128);
        return ImVec2(64, 64);
    };
    auto colorFor = [](const std::string& t) -> ImU32 {
        if (t == "sprite")           return IM_COL32(70, 140, 255, 200);
        if (t == "animated_sprite")  return IM_COL32(0, 200, 255, 200);
        if (t == "text")             return IM_COL32(80, 220, 80, 200);
        if (t == "camera")           return IM_COL32(255, 220, 50, 200);
        if (t == "point_light")      return IM_COL32(255, 160, 40, 200);
        if (t == "ambient_light")    return IM_COL32(255, 240, 140, 100);
        if (t == "particle_system")  return IM_COL32(200, 80, 255, 200);
        if (t == "tileset")          return IM_COL32(180, 140, 100, 200);
        return IM_COL32(150, 150, 150, 200);
    };

    ImVec2 canvas_min = ImGui::GetCursorScreenPos();
    ImVec2 canvas_sz  = ImGui::GetContentRegionAvail();
    if (canvas_sz.x < 50) canvas_sz.x = 50;
    if (canvas_sz.y < 50) canvas_sz.y = 50;
    ImVec2 canvas_max = ImVec2(canvas_min.x + canvas_sz.x, canvas_min.y + canvas_sz.y);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvas_min, canvas_max, IM_COL32(30, 30, 35, 255));

    // Camera-aware first frame initialisation
    constexpr float PPM = 160.0f;
    if (!tab.scene_view_initialized) {
        for (auto& o : objects) {
            if (o.is_object() && getStr(o, "type", "") == "camera") {
                double cx = getNum(o, "pos_x", 0);
                double cy = getNum(o, "pos_y", 0);
                double cz = getNum(o, "zoom", 1);
                tab.pan_x = (float)(-cx * cz * PPM);
                tab.pan_y = (float)(cy * cz * PPM);
                tab.zoom = std::max(0.05f, std::min((float)cz, 20.0f));
                break;
            }
        }
        tab.scene_view_initialized = true;
    }

    // Camera: origin = centre of canvas, Y‑up, zoom scales both axes
    float ox = canvas_min.x + canvas_sz.x * 0.5f + tab.pan_x;
    float oy = canvas_min.y + canvas_sz.y * 0.5f + tab.pan_y;
    float z  = tab.zoom * PPM;
    float gz = tab.zoom;

    auto toScreen = [&](float wx, float wy) { return ImVec2(ox + wx * z, oy - wy * z); };
    auto toWorld  = [&](float sx, float sy) { return ImVec2((sx - ox) / z, -(sy - oy) / z); };

    // World‑space bounding box in screen pixels
    auto worldRect = [&](double l, double b, double w, double h) {
        ImVec2 bl = toScreen((float)l, (float)b);
        ImVec2 br = toScreen((float)(l + w), (float)b);
        ImVec2 tl = toScreen((float)l, (float)(b + h));
        ImVec2 tr = toScreen((float)(l + w), (float)(b + h));
        return std::pair{
            ImVec2(std::min({bl.x, br.x, tl.x, tr.x}), std::min({bl.y, br.y, tl.y, tr.y})),
            ImVec2(std::max({bl.x, br.x, tl.x, tr.x}), std::max({bl.y, br.y, tl.y, tr.y}))
        };
    };

    // Expand a rect by padding (world units converted to screen)
    auto growRect = [&](const ImVec2& mn, const ImVec2& mx, float pad_wu) {
        float p = pad_wu * z;
        return std::pair{ImVec2(mn.x - p, mn.y - p), ImVec2(mx.x + p, mx.y + p)};
    };

    // ── GRID ──────────────────────────────────────────────────────────────
    {
        static const float steps[] = {0.1f, 0.2f, 0.5f, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000};
        float gs = steps[0];
        for (float s : steps) { if (s * z >= 50.0f) { gs = s; break; } }
        float ms = gs * 0.25f;

        ImVec2 wtl = toWorld(canvas_min.x, canvas_min.y);
        ImVec2 wbr = toWorld(canvas_max.x, canvas_max.y);
        float x0 = std::floor(wtl.x / gs) * gs;
        float y0 = std::floor(wtl.y / gs) * gs;

        // Minor grid
        if (ms * z > 8.0f) {
            for (float wx = x0; wx <= wbr.x + gs; wx += ms) {
                ImVec2 a = toScreen(wx, wtl.y), b = toScreen(wx, wbr.y);
                dl->AddLine(a, b, IM_COL32(40, 40, 48, 255));
            }
            for (float wy = y0; wy >= wbr.y - gs; wy -= ms) {
                ImVec2 a = toScreen(wtl.x, wy), b = toScreen(wbr.x, wy);
                dl->AddLine(a, b, IM_COL32(40, 40, 48, 255));
            }
        }
        // Major grid + labels
        float fs = ImGui::GetFontSize();
        char buf[64];
        for (float wx = x0; wx <= wbr.x + gs; wx += gs) {
            ImVec2 a = toScreen(wx, wtl.y), b = toScreen(wx, wbr.y);
            dl->AddLine(a, b, IM_COL32(60, 60, 75, 255));
            if (a.x > canvas_min.x + 4 && a.x < canvas_max.x - 4) {
                snprintf(buf, sizeof(buf), gs < 1.0f ? "%.1f" : "%.0f", wx);
                ImVec2 ts = ImGui::CalcTextSize(buf);
                dl->AddText(ImVec2(a.x - ts.x * 0.5f, canvas_max.y - fs - 2), IM_COL32(140, 140, 160, 220), buf);
            }
        }
        for (float wy = y0; wy >= wbr.y - gs; wy -= gs) {
            ImVec2 a = toScreen(wtl.x, wy), b = toScreen(wbr.x, wy);
            dl->AddLine(a, b, IM_COL32(60, 60, 75, 255));
            if (a.y > canvas_min.y + 4 && a.y < canvas_max.y - 4) {
                snprintf(buf, sizeof(buf), gs < 1.0f ? "%.1f" : "%.0f", wy);
                dl->AddText(ImVec2(canvas_min.x + 4, a.y - fs * 0.5f), IM_COL32(140, 140, 160, 220), buf);
            }
        }
        // Origin axes
        ImVec2 o = toScreen(0, 0);
        if (o.x >= canvas_min.x && o.x <= canvas_max.x)
            dl->AddLine(ImVec2(o.x, canvas_min.y), ImVec2(o.x, canvas_max.y), IM_COL32(180, 60, 60, 200), 1.5f);
        if (o.y >= canvas_min.y && o.y <= canvas_max.y)
            dl->AddLine(ImVec2(canvas_min.x, o.y), ImVec2(canvas_max.x, o.y), IM_COL32(60, 180, 60, 200), 1.5f);
        ImVec2 ol = toScreen(0, -0.5f);
        if (ol.x > canvas_min.x && ol.x < canvas_max.x - 20 && ol.y > canvas_min.y && ol.y < canvas_max.y - fs)
            dl->AddText(ImVec2(ol.x + 4, ol.y), IM_COL32(200, 200, 200, 180), "0");
    }

    // ── OBJECT RENDERING ──────────────────────────────────────────────────
    int hovered_idx = -1;
    for (size_t i = 0; i < objects.size(); i++) {
        auto& obj = objects[i];
        if (!obj.is_object()) continue;

        double px  = getNum(obj, "pos_x", 0);
        double py  = getNum(obj, "pos_y", 0);
        std::string type  = getStr(obj, "type", "");
        std::string name  = getStr(obj, "name", "");
        ImVec2 def = sizeFor(type);
        double sx  = getNum(obj, "size_x", def.x);
        double sy  = getNum(obj, "size_y", def.y);

        if (type == "point_light") { sx = getNum(obj, "radius", 60); sy = sx; }
        if (type == "tileset") {
            sx = getNum(obj, "tile_width", 32) * getNum(obj, "atlas_cols", 4);
            sy = getNum(obj, "tile_height", 32) * getNum(obj, "atlas_rows", 4);
        }

        // World AABB
        double l = px, b = py, r = px + sx, t = py + sy;
        if (type == "sprite" || type == "animated_sprite") {
            double oxx = getNum(obj, "origin_x", 0.5);
            double oyy = getNum(obj, "origin_y", 0.5);
            l = px - sx * oxx; b = py - sy * oyy;
            r = l + sx; t = b + sy;
        }
        auto [scr_min, scr_max] = worldRect(l, b, r - l, t - b);
        ImU32 col = colorFor(type);
        bool selected = ((int)i == tab.scene_selected_object);
        ImVec2 centre = toScreen((float)px, (float)py);

        // ── Draw shape ──
        if (type == "point_light") {
            float rw = (float)sx * z;
            dl->AddCircleFilled(centre, rw, IM_COL32(255, 200, 80, 40));
            dl->AddCircle(centre, rw, IM_COL32(255, 200, 80, 160), 0, 2.0f * z);
            dl->AddCircleFilled(centre, 5.0f * z, col);
        }
        else if (type == "ambient_light") {
            float rw = (float)sx * z;
            dl->AddCircleFilled(centre, rw, IM_COL32(255, 240, 140, 25));
            dl->AddCircle(centre, rw, IM_COL32(255, 240, 140, 80), 0, 1.0f * z);
        }
        else if (type == "camera") {
            float szz = 20.0f * z;
            ImVec2 a(centre.x - szz, centre.y - szz);
            ImVec2 b(centre.x + szz, centre.y - szz);
            ImVec2 c(centre.x,        centre.y + szz);
            dl->AddTriangleFilled(a, b, c, col);
            dl->AddTriangle(a, b, c, IM_COL32(255, 255, 255, 100), 1.5f * z);
        }
        else if (type == "particle_system") {
            dl->AddRectFilled(scr_min, scr_max, col);
            dl->AddCircleFilled(toScreen((float)(px + sx * 0.3f), (float)(py + sy * 0.3f)), 3 * z, IM_COL32(255, 255, 255, 180));
            dl->AddCircleFilled(toScreen((float)(px + sx * 0.7f), (float)(py + sy * 0.7f)), 3 * z, IM_COL32(255, 255, 255, 180));
            dl->AddCircleFilled(toScreen((float)(px + sx * 0.5f), (float)(py + sy * 0.3f)), 2 * z, IM_COL32(255, 255, 255, 180));
        }
        else if (type == "text") {
            dl->AddRectFilled(scr_min, scr_max, col);
            dl->AddText(ImVec2(scr_min.x + 4.0f * z, scr_min.y + 4.0f * z), IM_COL32(255, 255, 255, 220), "T");
        }
        else if (type == "tileset") {
            dl->AddRectFilled(scr_min, scr_max, col);
            float tw = (float)getNum(obj, "tile_width", 32), th = (float)getNum(obj, "tile_height", 32);
            int ac = (int)getNum(obj, "atlas_cols", 4), ar = (int)getNum(obj, "atlas_rows", 4);
            for (int ri = 0; ri < ar; ri++) {
                for (int ci = 0; ci < ac; ci++) {
                    ImVec2 t0 = toScreen((float)(l + ci * tw), (float)(b + ri * th));
                    ImVec2 t1 = toScreen((float)(l + (ci + 1) * tw), (float)(b + (ri + 1) * th));
                    dl->AddRect(t0, t1, IM_COL32(0, 0, 0, 80), 0, 0, 1.0f * z);
                }
            }
        }
        else if (type == "sprite" || type == "animated_sprite") {
            std::string tex_path = getStr(obj, "texture", "");
            ImTextureID tex_id = (ImTextureID)0;
            bool tex_ok = false;
            if (!tex_path.empty()) {
                auto it = tab.scene_texture_cache.find(tex_path);
                if (it != tab.scene_texture_cache.end()) {
                    tex_id = it->second.id; tex_ok = true;
                } else {
                    fs::path scene_dir = fs::path(tab.file_path).parent_path();
                    fs::path full = scene_dir / tex_path;
                    if (!fs::exists(full)) full = m_project_path / "engine" / tex_path;
                    if (fs::exists(full)) {
                        int w, h, ch;
                        unsigned char* data = stbi_load(full.string().c_str(), &w, &h, &ch, 4);
                        if (data) {
                            GLuint gl;
                            glGenTextures(1, &gl);
                            glBindTexture(GL_TEXTURE_2D, gl);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                            stbi_image_free(data);
                            tex_id = (ImTextureID)(intptr_t)gl;
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
                        tu = arr[0].get<crude_json::number>(); tv = arr[1].get<crude_json::number>();
                        tw = arr[2].get<crude_json::number>(); th = arr[3].get<crude_json::number>();
                    }
                } else {
                    tu = getNum(obj, "texture_u", 0); tv = getNum(obj, "texture_v", 0);
                    tw = getNum(obj, "texture_w", 1); th = getNum(obj, "texture_h", 1);
                }
                float cr = (float)getNum(obj, "color_r", 1);
                float cg = (float)getNum(obj, "color_g", 1);
                float cb = (float)getNum(obj, "color_b", 1);
                float ca = (float)getNum(obj, "color_a", 1);
                dl->AddImage(tex_id, scr_min, scr_max,
                    ImVec2((float)tu, (float)tv), ImVec2((float)(tu + tw), (float)(tv + th)),
                    IM_COL32((int)(cr*255), (int)(cg*255), (int)(cb*255), (int)(ca*255)));
                dl->AddRect(scr_min, scr_max,
                    selected ? IM_COL32(255, 200, 50, 255) : col, 0, 0,
                    selected ? (2.0f * z) : (1.0f * z));
            } else {
                dl->AddRectFilled(scr_min, scr_max, col);
                dl->AddLine(ImVec2(scr_min.x, scr_min.y), ImVec2(scr_max.x, scr_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
                dl->AddLine(ImVec2(scr_max.x, scr_min.y), ImVec2(scr_min.x, scr_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
            }
            if (type == "animated_sprite") dl->AddCircleFilled(centre, 4.0f * z, IM_COL32(255, 255, 255, 120));
        }
        else {
            dl->AddRectFilled(scr_min, scr_max, col);
            dl->AddLine(ImVec2(scr_min.x, scr_min.y), ImVec2(scr_max.x, scr_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
            dl->AddLine(ImVec2(scr_max.x, scr_min.y), ImVec2(scr_min.x, scr_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
        }

        // Label
        if (z > 0.3f) {
            ImVec2 lp(scr_min.x, scr_max.y + 2.0f * z);
            dl->AddText(ImVec2(lp.x + 1, lp.y + 1), IM_COL32(0, 0, 0, 180), name.c_str());
            dl->AddText(lp, IM_COL32(220, 220, 230, 220), name.c_str());
        }

        // ── Selection gizmo ──
        if (selected && gz > 0.05f) {
            float pd = 5.0f * gz;
            float sel_pad = 5.0f * gz;
            auto [sm, sM] = std::pair{ImVec2(scr_min.x - sel_pad, scr_min.y - sel_pad), ImVec2(scr_max.x + sel_pad, scr_max.y + sel_pad)};
            float cx = (float)(l + r) * 0.5f, cy = (float)(b + t) * 0.5f;
            ImVec2 oc = toScreen(cx, cy);
            ImU32 oc_col = IM_COL32(255, 200, 50, 255);
            dl->AddRect(sm, sM, oc_col, 0, 0, 2.0f * gz);

            if (tab.scene_gizmo_mode == 0) {              // MOVE
                float al = 36.0f * gz, hl = 10.0f * gz, hw = 6.0f * gz, cs = 5.0f * gz;
                ImVec2 xe(oc.x + al, oc.y);
                ImVec2 ye(oc.x, oc.y - al);
                dl->AddLine(oc, xe, tab.scene_drag_handle == 10 ? IM_COL32(255, 230, 50, 255) : IM_COL32(220, 70, 70, 255), 3);
                dl->AddTriangleFilled(ImVec2(xe.x + hl, oc.y), ImVec2(xe.x, oc.y - hw), ImVec2(xe.x, oc.y + hw), IM_COL32(220, 70, 70, 255));
                dl->AddLine(oc, ye, tab.scene_drag_handle == 11 ? IM_COL32(255, 230, 50, 255) : IM_COL32(70, 180, 70, 255), 3);
                dl->AddTriangleFilled(ImVec2(oc.x, ye.y - hl), ImVec2(oc.x - hw, ye.y), ImVec2(oc.x + hw, ye.y), IM_COL32(70, 180, 70, 255));
                ImU32 cc = tab.scene_drag_handle == 9 ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 230);
                dl->AddRectFilled(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), cc);
                dl->AddRect(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), IM_COL32(60, 60, 60, 200));
            }
            else if (tab.scene_gizmo_mode == 1) {         // ROTATE
                float rr = std::max((float)sx, (float)sy) * 0.5f * z + 16.0f * gz;
                dl->AddCircle(oc, rr, IM_COL32(80, 180, 255, 180), 0, 2.0f * gz);
                double ang = getNum(obj, "rot", 0);
                float rad = (float)(ang * 3.14159265f / 180.0f);
                ImVec2 hp(oc.x + cosf(rad) * rr, oc.y - sinf(rad) * rr);
                float hr = 6.0f * gz;
                dl->AddLine(oc, hp, IM_COL32(80, 180, 255, 120), 1.5f * gz);
                dl->AddCircleFilled(hp, hr, tab.scene_drag_handle == 8 ? IM_COL32(255, 230, 50, 255) : IM_COL32(80, 200, 255, 230));
                dl->AddCircle(hp, hr, oc_col, 0, 1.5f * gz);
            }
            else if (tab.scene_gizmo_mode == 2) {         // SCALE
                float hs = 8.0f * gz, hhs = hs * 0.5f, sp = pd;
                ImVec2 corners[4] = {
                    {sm.x - sp - hhs, sm.y - sp - hhs},
                    {sM.x + sp - hhs, sm.y - sp - hhs},
                    {sM.x + sp - hhs, sM.y + sp - hhs},
                    {sm.x - sp - hhs, sM.y + sp - hhs}
                };
                ImVec2 edges[4] = {
                    {(sm.x + sM.x) * 0.5f - hhs, sm.y - sp - hhs},
                    {sM.x + sp - hhs, (sm.y + sM.y) * 0.5f - hhs},
                    {(sm.x + sM.x) * 0.5f - hhs, sM.y + sp - hhs},
                    {sm.x - sp - hhs, (sm.y + sM.y) * 0.5f - hhs}
                };
                for (int ci = 0; ci < 4; ci++) dl->AddLine(oc, ImVec2(corners[ci].x + hhs, corners[ci].y + hhs), IM_COL32(200, 200, 200, 80), 1.0f * gz);
                for (int ci = 0; ci < 4; ci++) dl->AddLine(oc, ImVec2(edges[ci].x + hhs, edges[ci].y + hhs), IM_COL32(200, 200, 200, 60), 1.0f * gz);
                auto hcol = [&](int hid, ImU32 normal) { return tab.scene_drag_handle == hid ? IM_COL32(255, 230, 50, 255) : normal; };
                for (int ci = 0; ci < 4; ci++) {
                    dl->AddRectFilled(corners[ci], ImVec2(corners[ci].x + hs, corners[ci].y + hs), hcol(ci, IM_COL32(255, 255, 255, 230)));
                    dl->AddRect(corners[ci], ImVec2(corners[ci].x + hs, corners[ci].y + hs), IM_COL32(100, 100, 100, 200));
                }
                for (int ci = 0; ci < 4; ci++) {
                    dl->AddRectFilled(edges[ci], ImVec2(edges[ci].x + hs, edges[ci].y + hs), hcol(ci + 4, IM_COL32(200, 200, 220, 230)));
                    dl->AddRect(edges[ci], ImVec2(edges[ci].x + hs, edges[ci].y + hs), IM_COL32(100, 100, 100, 200));
                }
                float cs = 5.0f * gz;
                ImU32 cc = tab.scene_drag_handle == 9 ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 200);
                dl->AddRectFilled(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), cc);
                dl->AddRect(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), IM_COL32(60, 60, 60, 200));
            }
        }

        // Hover test
        if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(canvas_min, canvas_max)) {
            ImVec2 mw = toWorld(io.MousePos.x, io.MousePos.y);
            if (mw.x >= l && mw.x <= r && mw.y >= b && mw.y <= t) hovered_idx = (int)i;
        }
    }

    // ── VIEWPORT INTERACTION ──────────────────────────────────────────────
    if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(canvas_min, canvas_max)) {
        // Zoom toward mouse cursor
        if (io.MouseWheel != 0.0f) {
            float old_z = z;
            tab.zoom *= (io.MouseWheel > 0) ? 1.1f : 0.9f;
            tab.zoom = std::max(0.05f, std::min(tab.zoom, 20.0f));
            z = tab.zoom * PPM;
            // Keep world point under cursor stationary
            ImVec2 mw = toWorld(io.MousePos.x, io.MousePos.y);
            float scale = z / old_z;
            tab.pan_x = (tab.pan_x + canvas_sz.x * 0.5f) * scale - canvas_sz.x * 0.5f - mw.x * z + mw.x * old_z * scale;
            tab.pan_y = (tab.pan_y + canvas_sz.y * 0.5f) * scale - canvas_sz.y * 0.5f + mw.y * z - mw.y * old_z * scale;

            ox = canvas_min.x + canvas_sz.x * 0.5f + tab.pan_x;
            oy = canvas_min.y + canvas_sz.y * 0.5f + tab.pan_y;
        }
        // Pan with middle mouse
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            tab.pan_x += io.MouseDelta.x;
            tab.pan_y += io.MouseDelta.y;
            ox = canvas_min.x + canvas_sz.x * 0.5f + tab.pan_x;
            oy = canvas_min.y + canvas_sz.y * 0.5f + tab.pan_y;
        }
        // Gizmo mode shortcuts
        if (ImGui::IsKeyPressed(ImGuiKey_W)) { tab.scene_gizmo_mode = 0; tab.scene_drag_handle = -1; }
        if (ImGui::IsKeyPressed(ImGuiKey_E)) { tab.scene_gizmo_mode = 1; tab.scene_drag_handle = -1; }
        if (ImGui::IsKeyPressed(ImGuiKey_R)) { tab.scene_gizmo_mode = 2; tab.scene_drag_handle = -1; }

        int sel = tab.scene_selected_object;
        bool on_handle = false;

        if (sel >= 0 && sel < (int)objects.size()) {
            auto& sobj = objects[sel];
            if (sobj.is_object()) {
                std::string st = getStr(sobj, "type", "");
                ImVec2 ssz = sizeFor(st);
                double spx = getNum(sobj, "pos_x", 0);
                double spy = getNum(sobj, "pos_y", 0);
                double ssx = getNum(sobj, "size_x", ssz.x);
                double ssy = getNum(sobj, "size_y", ssz.y);
                if (st == "point_light") { ssx = getNum(sobj, "radius", 48); ssy = ssx; }
                if (st == "tileset") {
                    ssx = getNum(sobj, "tile_width", 32) * getNum(sobj, "atlas_cols", 4);
                    ssy = getNum(sobj, "tile_height", 32) * getNum(sobj, "atlas_rows", 4);
                }
                double swl = spx, swb = spy, swr = spx + ssx, swt = spy + ssy;
                if (st == "sprite" || st == "animated_sprite") {
                    double sorx = getNum(sobj, "origin_x", 0.5);
                    double sory = getNum(sobj, "origin_y", 0.5);
                    swl = spx - ssx * sorx; swb = spy - ssy * sory;
                    swr = swl + ssx; swt = swb + ssy;
                }
                auto [ssmin, ssmax] = worldRect(swl, swb, swr - swl, swt - swb);
                float spd = 5.0f * gz;
                float scx = (float)(swl + swr) * 0.5f;
                float scy = (float)(swb + swt) * 0.5f;
                ImVec2 soc = toScreen(scx, scy);
                ImVec2 ms = io.MousePos;

                // ── Drag active ──
                if (tab.scene_drag_handle >= 0) {
                    on_handle = true;
                    ImVec2 mw = toWorld(ms.x, ms.y);
                    if (tab.scene_gizmo_mode == 0) {
                        if (tab.scene_drag_handle == 10)
                            setNum(sobj, "pos_x", tab.scene_drag_vx + (mw.x - tab.scene_drag_vx));
                        else if (tab.scene_drag_handle == 11)
                            setNum(sobj, "pos_y", tab.scene_drag_vy + (mw.y - tab.scene_drag_vy));
                        else {
                            setNum(sobj, "pos_x", spx + io.MouseDelta.x / z);
                            setNum(sobj, "pos_y", spy - io.MouseDelta.y / z);
                        }
                        tab.modified = true;
                    }
                    else if (tab.scene_gizmo_mode == 1) {
                        float dx = mw.x - scx, dy = mw.y - scy;
                        setNum(sobj, "rot", atan2((double)dy, (double)dx) * 180.0 / 3.141592653589793);
                        tab.modified = true;
                    }
                    else if (tab.scene_gizmo_mode == 2) {
                        double nwl = tab.scene_drag_wl, nwb = tab.scene_drag_wb;
                        double nwr = tab.scene_drag_wr, nwt = tab.scene_drag_wt;
                        if (tab.scene_drag_handle == 9) {
                            double dx = io.MouseDelta.x / z, dy = -io.MouseDelta.y / z;
                            double avg = (dx + dy) * 0.5;
                            double nsx = tab.scene_drag_vsx + avg;
                            double nsy = tab.scene_drag_vsy + avg;
                            if (nsx < 0.1) nsx = 0.1; if (nsy < 0.1) nsy = 0.1;
                            double sorx = getNum(sobj, "origin_x", 0.5);
                            double sory = getNum(sobj, "origin_y", 0.5);
                            double wlo = tab.scene_drag_vx - tab.scene_drag_vsx * sorx;
                            double wbo = tab.scene_drag_vy - tab.scene_drag_vsy * sory;
                            setNum(sobj, "pos_x", wlo + nsx * sorx);
                            setNum(sobj, "pos_y", wbo + nsy * sory);
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
                }
                // ── Hit test for handles ──
                else {
                    float hsz = 8.0f * gz;
                    if (tab.scene_gizmo_mode == 0) {
                        float al = 36.0f * gz, aw = 8.0f * gz, cs = 7.0f * gz;
                        bool on_x = ms.x >= soc.x - 5.0f * gz && ms.x <= soc.x + al + 10.0f * gz && ms.y >= soc.y - aw && ms.y <= soc.y + aw;
                        bool on_y = ms.x >= soc.x - aw && ms.x <= soc.x + aw && ms.y >= soc.y - al - 10.0f * gz && ms.y <= soc.y + 5.0f * gz;
                        bool on_ct = fabsf(ms.x - soc.x) < cs && fabsf(ms.y - soc.y) < cs;
                        if (on_x) { on_handle = true; if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 10; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; } }
                        else if (on_y) { on_handle = true; if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 11; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; } }
                        else if (on_ct) { on_handle = true; if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 9; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; } }
                    }
                    else if (tab.scene_gizmo_mode == 1) {
                        float rr = std::max((float)ssx, (float)ssy) * 0.5f * z + 16.0f * gz;
                        double ang = getNum(sobj, "rot", 0);
                        float rad = (float)(ang * 3.14159265f / 180.0f);
                        float hx = soc.x + cosf(rad) * rr, hy = soc.y - sinf(rad) * rr, hr = 8.0f * gz;
                        if (fabsf(ms.x - hx) < hr && fabsf(ms.y - hy) < hr) {
                            on_handle = true;
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { tab.scene_drag_handle = 8; tab.scene_drag_vx = spx; tab.scene_drag_vy = spy; }
                        }
                    }
                    else if (tab.scene_gizmo_mode == 2) {
                        float shs = hsz * 0.5f;
                        ImVec2 handles[9] = {
                            {ssmin.x - spd - shs, ssmin.y - spd - shs},
                            {ssmax.x + spd - shs, ssmin.y - spd - shs},
                            {ssmax.x + spd - shs, ssmax.y + spd - shs},
                            {ssmin.x - spd - shs, ssmax.y + spd - shs},
                            {(ssmin.x + ssmax.x) * 0.5f - shs, ssmin.y - spd - shs},
                            {ssmax.x + spd - shs, (ssmin.y + ssmax.y) * 0.5f - shs},
                            {(ssmin.x + ssmax.x) * 0.5f - shs, ssmax.y + spd - shs},
                            {ssmin.x - spd - shs, (ssmin.y + ssmax.y) * 0.5f - shs},
                            {soc.x - 7.0f * gz, soc.y - 7.0f * gz}
                        };
                        for (int hi = 0; hi < 9; hi++) {
                            int hid = (hi == 8) ? 9 : hi;
                            ImVec2 hm = handles[hi];
                            float hw = (hi == 8) ? 14.0f * gz : hsz;
                            if (fabsf(ms.x - hm.x - hw * 0.5f) < hw * 0.5f && fabsf(ms.y - hm.y - hw * 0.5f) < hw * 0.5f) {
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

        // Click to select / deselect
        if (!on_handle && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            tab.scene_selected_object = (hovered_idx >= 0) ? hovered_idx : -1;

        // Drag-move selected object (not on handle)
        if (!on_handle && tab.scene_drag_handle < 0 && ImGui::IsMouseDragging(ImGuiMouseButton_Left)
            && sel >= 0 && sel < (int)objects.size()) {
            auto& mobj = objects[sel];
            if (mobj.is_object()) {
                setNum(mobj, "pos_x", getNum(mobj, "pos_x", 0) + io.MouseDelta.x / z);
                setNum(mobj, "pos_y", getNum(mobj, "pos_y", 0) - io.MouseDelta.y / z);
                tab.modified = true;
            }
        }
        if (tab.scene_drag_handle >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) tab.scene_drag_handle = -1;
    }

    // ── Tooltip ──
    if (hovered_idx >= 0 && hovered_idx < (int)objects.size()) {
        auto& obj = objects[hovered_idx];
        std::string nm = getStr(obj, "name", "Unnamed");
        std::string tp = getStr(obj, "type", "?");
        ImVec2 ds = sizeFor(tp);
        double px = getNum(obj, "pos_x", 0), py = getNum(obj, "pos_y", 0);
        double sx = getNum(obj, "size_x", ds.x), sy = getNum(obj, "size_y", ds.y);
        double rot = getNum(obj, "rot", 0);
        ImGui::SetTooltip("[%s] %s\npos: %.2f, %.2f  size: %.2f x %.2f  rot: %.1f\xC2\xB0",
            tp.c_str(), nm.c_str(), px, py, sx, sy, rot);
    }

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
    }
}

static const char* s_scene_types[] = {
    "sprite", "animated_sprite", "text", "camera",
    "point_light", "ambient_light", "particle_system", "tileset"
};

// ======================== SCENE INSPECTOR WINDOW ========================

void Editor::RenderSceneHierarchyWindow() {
    if (!m_show_scene_hierarchy) return;

    ImGui::Begin("Scene Hierarchy", &m_show_scene_hierarchy);

    int scene_tab_idx = -1;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].type == EditorTabType::Scene) { scene_tab_idx = i; break; }
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
    ImGui::SameLine();
    if (ImGui::Button("Export Lua", ImVec2(100, 0))) {
        std::string name = doc.contains("name") && doc["name"].is_string()
            ? doc["name"].get<crude_json::string>() : "scene";
        std::string lua;
        lua += "-- Auto-generated scene loader: " + name + "\n";
        lua += "local " + name + " = BlazeBolt.LoadSceneFile(\"" + tab.file_path + "\")\n";
        lua += "-- Object IDs:\n";
        if (doc.contains("objects") && doc["objects"].is_array()) {
            for (auto& obj : doc["objects"].get<crude_json::array>()) {
                if (obj.is_object() && obj.contains("name") && obj["name"].is_string()) {
                    lua += "-- " + name + "[\"" + obj["name"].get<crude_json::string>() + "\"] = entity ID\n";
                }
            }
        }
        std::string lua_path = tab.file_path;
        auto dot_pos = lua_path.rfind('.');
        if (dot_pos != std::string::npos) lua_path = lua_path.substr(0, dot_pos);
        lua_path += ".lua";
        { std::ofstream f(lua_path, std::ios::binary); if (f.is_open()) { f.write(lua.data(), lua.size()); } }
        OpenFileInTab(lua_path);
    }
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
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1));
            ImGui::Text("[%s]", type_str.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine(60);
            bool sel = (tab.scene_selected_object == (int)i);
            if (ImGui::Selectable(display_name.c_str(), &sel)) {
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
        for (int i = 0; i < 8; i++) { if (strcmp(tab.scene_new_type, s_scene_types[i]) == 0) { type_idx = i; break; } }
        if (ImGui::Combo("Type", &type_idx, s_scene_types, 8)) {
            strncpy(tab.scene_new_type, s_scene_types[type_idx], sizeof(tab.scene_new_type) - 1);
        }
        if (ImGui::Button("Add Object")) {
            if (tab.scene_new_name[0]) {
                crude_json::value n{
                    crude_json::object{{"name", std::string(tab.scene_new_name)}, {"type", std::string(tab.scene_new_type)}}
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
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].type == EditorTabType::Scene) { scene_tab_idx = i; break; }
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

    // Collect all code tabs
    std::vector<int> code_tab_indices;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].type == EditorTabType::Code) {
            code_tab_indices.push_back(i);
        }
    }

    if (code_tab_indices.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a file to start editing");
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // Ensure m_active_tab points to a code tab if possible
    bool active_is_code = false;
    for (int idx : code_tab_indices) {
        if (idx == m_active_tab) { active_is_code = true; break; }
    }
    if (!active_is_code) {
        m_active_tab = code_tab_indices[0];
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) { SaveTab(m_tabs[m_active_tab]); }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W)) { CloseTab(m_active_tab); ImGui::End(); return; }
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) RunGame();

    // Tab bar for code files
    if (ImGui::BeginTabBar("##CodeTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll)) {
        for (int idx : code_tab_indices) {
            auto& t = m_tabs[idx];
            if (!t.code_editor) continue;

            std::string label = t.title;
            if (t.modified) label += " *";

            ImGuiTabItemFlags tab_flags = (idx == m_active_tab) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            bool open = true;
            if (ImGui::BeginTabItem(label.c_str(), &open, tab_flags)) {
                if (idx != m_active_tab) {
                    m_active_tab = idx;
                }
                ImGui::EndTabItem();
            }
            if (!open) {
                CloseTab(idx);
                ImGui::EndTabBar();
                ImGui::End();
                return;
            }
        }
        ImGui::EndTabBar();
    }

    // Render active code editor
    auto& tab = m_tabs[m_active_tab];
    if (!tab.code_editor) { ImGui::End(); return; }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    tab.code_editor->Render(tab.title.c_str(), avail);

    if (tab.code_editor->IsTextChanged()) tab.modified = true;

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
        if (type == "sprite") return ImVec2(64, 64);
        if (type == "animated_sprite") return ImVec2(64, 64);
        if (type == "text") return ImVec2(128, 24);
        if (type == "camera") return ImVec2(32, 32);
        if (type == "point_light") return ImVec2(48, 48);
        if (type == "ambient_light") return ImVec2(160, 160);
        if (type == "particle_system") return ImVec2(48, 48);
        if (type == "tileset") return ImVec2(128, 128);
        return ImVec2(64, 64);
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
        return IM_COL32(150, 150, 150, 200);
    };

    // Toolbar
    {
        if (ImGui::Button("Save", ImVec2(80, 0))) SaveTab(tab);
        ImGui::SameLine();
        if (ImGui::Button("Export Lua", ImVec2(100, 0))) {
            std::string name = doc.contains("name") && doc["name"].is_string()
                ? doc["name"].get<crude_json::string>() : "scene";
            std::string lua;
            lua += "-- Auto-generated scene loader: " + name + "\n";
            lua += "local " + name + " = BlazeBolt.LoadSceneFile(\"" + tab.file_path + "\")\n";
            lua += "-- Object IDs:\n";
            if (doc.contains("objects") && doc["objects"].is_array()) {
                for (auto& obj : doc["objects"].get<crude_json::array>()) {
                    if (obj.is_object() && obj.contains("name") && obj["name"].is_string()) {
                        lua += "-- " + name + "[\"" + obj["name"].get<crude_json::string>() + "\"] = entity ID\n";
                    }
                }
            }
            std::string lua_path = tab.file_path;
            auto dot_pos = lua_path.rfind('.');
            if (dot_pos != std::string::npos) lua_path = lua_path.substr(0, dot_pos);
            lua_path += ".lua";
            { std::ofstream f(lua_path, std::ios::binary); if (f.is_open()) { f.write(lua.data(), lua.size()); } }
            OpenFileInTab(lua_path);
        }
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
            if (tp == "point_light") { sx = getNum(obj, "radius", 60); sy = sx; }
            if (tp == "tileset") { sx = getNum(obj, "tile_width", 32) * getNum(obj, "atlas_cols", 4); sy = getNum(obj, "tile_height", 32) * getNum(obj, "atlas_rows", 4); }
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
                dl->AddCircleFilled(cen, r, IM_COL32(255, 200, 80, 40));
                dl->AddCircle(cen, r, IM_COL32(255, 200, 80, 160), 0, 2.0f * z);
                dl->AddCircleFilled(cen, 5 * z, col);
            } else if (tp == "ambient_light") {
                float r = (float)sx * z;
                dl->AddCircleFilled(cen, r, IM_COL32(255, 240, 140, 25));
                dl->AddCircle(cen, r, IM_COL32(255, 240, 140, 80), 0, 1.0f * z);
            } else if (tp == "camera") {
                float szz = 20 * z;
                dl->AddTriangleFilled(ImVec2(cen.x - szz, cen.y - szz), ImVec2(cen.x + szz, cen.y - szz), ImVec2(cen.x, cen.y + szz), col);
                dl->AddTriangle(ImVec2(cen.x - szz, cen.y - szz), ImVec2(cen.x + szz, cen.y - szz), ImVec2(cen.x, cen.y + szz), IM_COL32(255, 255, 255, 100), 1.5f * z);
            } else if (tp == "particle_system") {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddCircleFilled(w2s((float)(px + sx * 0.3), (float)(py + sy * 0.3)), 3 * z, IM_COL32(255, 255, 255, 180));
                dl->AddCircleFilled(w2s((float)(px + sx * 0.7), (float)(py + sy * 0.7)), 3 * z, IM_COL32(255, 255, 255, 180));
                dl->AddCircleFilled(w2s((float)(px + sx * 0.5), (float)(py + sy * 0.3)), 2 * z, IM_COL32(255, 255, 255, 180));
            } else if (tp == "text") {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddText(ImVec2(sc_min.x + 4 * z, sc_min.y + 4 * z), IM_COL32(255, 255, 255, 220), "T");
            } else if (tp == "tileset") {
                dl->AddRectFilled(sc_min, sc_max, col);
                float tw = (float)getNum(obj, "tile_width", 32), th = (float)getNum(obj, "tile_height", 32);
                int ac = (int)getNum(obj, "atlas_cols", 4), ar = (int)getNum(obj, "atlas_rows", 4);
                for (int ri = 0; ri < ar; ri++) for (int ci = 0; ci < ac; ci++) {
                    ImVec2 t_tl = w2s((float)(wl + ci * tw), (float)(wb + ri * th));
                    ImVec2 t_br = w2s((float)(wl + (ci + 1) * tw), (float)(wb + (ri + 1) * th));
                    dl->AddRect(t_tl, t_br, IM_COL32(0, 0, 0, 80), 0, 0, 1.0f * z);
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
                        sel ? (2.0f * z) : (1.0f * z));
                } else {
                    dl->AddRectFilled(sc_min, sc_max, col);
                    dl->AddLine(ImVec2(sc_min.x, sc_min.y), ImVec2(sc_max.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
                    dl->AddLine(ImVec2(sc_max.x, sc_min.y), ImVec2(sc_min.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
                }
                if (tp == "animated_sprite") dl->AddCircleFilled(cen, 4 * z, IM_COL32(255, 255, 255, 120));
            } else {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddLine(ImVec2(sc_min.x, sc_min.y), ImVec2(sc_max.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
                dl->AddLine(ImVec2(sc_max.x, sc_min.y), ImVec2(sc_min.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
            }
            // Name label
            if (z > 0.3f) {
                ImVec2 lp = ImVec2(sc_min.x, sc_max.y + 2 * z);
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
                    if (st == "point_light") { ssx = getNum(sobj, "radius", 48); ssy = ssx; }
                    if (st == "tileset") { ssx = getNum(sobj, "tile_width", 32) * getNum(sobj, "atlas_cols", 4); ssy = getNum(sobj, "tile_height", 32) * getNum(sobj, "atlas_rows", 4); }
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
