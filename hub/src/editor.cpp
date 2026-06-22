#include "editor.hpp"
#include "title_bar.hpp"
#include "node_editor.hpp"
#include "stb_image.h"
#include "stb_image_write.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>

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

bool Editor::IsNodeGraphFile(const std::string& ext) {
    return ext == ".nodemap";
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
    } else if (IsNodeGraphFile(ext)) {
        tab.type = EditorTabType::NodeGraph;
        tab.node_editor = std::make_unique<NodeEditor>();
        tab.node_editor->LoadFromFile(path);
        tab.title = p.filename().string();
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
    } else if (tab.type == EditorTabType::NodeGraph && tab.node_editor) {
        if (tab.node_editor->SaveToFile(tab.file_path)) {
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

    float title_bar_h = GetTitleBar().GetHeight();

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + title_bar_h));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y - title_bar_h));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##Editor", nullptr, flags);
    ImGui::PopStyleVar(3);

    RenderMenuBar();
    RenderToolBar();

    float top_offset = ImGui::GetFrameHeightWithSpacing() + 40.0f;
    float side_panel_w = 260.0f;

    ImGui::SetCursorPosY(top_offset);
    ImGui::BeginChild("##SidePanel", ImVec2(side_panel_w, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_Borders);
    RenderSidePanel();
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::SetCursorPosY(top_offset);
    ImGui::BeginChild("##MainArea", ImVec2(0, 0));

    RenderTabBar();
    RenderTabContent();

    ImGui::EndChild();
    ImGui::End();
}

// ======================== MENU BAR ========================

void Editor::RenderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New File", "Ctrl+N")) {
                int idx = 1;
                std::string name;
                do { name = "untitled_" + std::to_string(idx++) + ".lua"; }
                while (fs::exists(m_fm_current_dir / name));
                FMCreateFile(name);
                OpenFileInTab((m_fm_current_dir / name).string());
            }
            if (ImGui::MenuItem("New Node Graph")) {
                int idx = 1;
                std::string name;
                do { name = "script_" + std::to_string(idx++) + ".nodemap"; }
                while (fs::exists(m_fm_current_dir / name));
                std::string full_path = (m_fm_current_dir / name).string();
                CreateNewNodeGraph(full_path);
                RefreshFM();
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
        ImGui::EndMainMenuBar();
    }
}

// ======================== TOOLBAR ========================

void Editor::RenderToolBar() {
    float menu_h = ImGui::GetFrameHeight();
    ImGui::SetCursorPosY(menu_h);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.60f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.72f, 0.36f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.52f, 0.24f, 1.0f));
    if (ImGui::Button("Start", ImVec2(80, 30))) RunGame();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    if (m_active_tab >= 0 && m_active_tab < (int)m_tabs.size()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "%s", m_tabs[m_active_tab].file_path.c_str());
    }
    ImGui::SameLine(0, 20.0f);
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "Project: %s", m_project_path.filename().string().c_str());
}

// ======================== SIDE PANEL / FILE MANAGER ========================

void Editor::RenderSidePanel() {
    bool is_node_graph = false;
    if (m_active_tab >= 0 && m_active_tab < (int)m_tabs.size()) {
        is_node_graph = (m_tabs[m_active_tab].type == EditorTabType::NodeGraph);
    }

    if (is_node_graph) {
        ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Nodes");
        ImGui::Separator();

        auto& tab = m_tabs[m_active_tab];
        if (tab.node_editor) {
            ImGui::BeginChild("##NodePaletteSection", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.55f), ImGuiChildFlags_Borders);
            tab.node_editor->RenderPalettePanel();
            ImGui::EndChild();

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::Separator();
        }
    }

    ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.90f, 1.0f), "Files");
    ImGui::Separator();

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
        else if (IsNodeGraphFile(ext)) ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.9f, 1.0f), "%s", "<>");
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
}

// ======================== TAB BAR ========================

void Editor::RenderTabBar() {
    if (ImGui::BeginTabBar("##TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs)) {
        for (int i = 0; i < (int)m_tabs.size(); i++) {
            auto& tab = m_tabs[i];
            std::string label = tab.title;
            if (tab.modified) label += " *";
            bool open = true;
            ImGuiTabItemFlags flags = tab.modified ? ImGuiTabItemFlags_UnsavedDocument : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem(label.c_str(), &open, flags)) { m_active_tab = i; ImGui::EndTabItem(); }
            if (!open) { CloseTab(i); i--; }
        }
        ImGui::EndTabBar();
    }
}

// ======================== TAB CONTENT ========================

void Editor::RenderTabContent() {
    if (m_active_tab < 0 || m_active_tab >= (int)m_tabs.size()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y * 0.5f - 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));
        ImGui::Text("Open a file to start editing");
        ImGui::PopStyleColor();
        return;
    }
    auto& tab = m_tabs[m_active_tab];
    if (tab.type == EditorTabType::Code) RenderCodeEditor(tab);
    else if (tab.type == EditorTabType::Image) RenderImageEditor(tab);
    else if (tab.type == EditorTabType::NodeGraph) RenderNodeEditor(tab);
    else if (tab.type == EditorTabType::Scene) RenderSceneEditor(tab);
}

// ======================== CODE EDITOR (TextEditor) ========================

void Editor::RenderCodeEditor(EditorTab& tab) {
    if (!tab.code_editor) return;

    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveTab(tab);
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_W)) { int idx = m_active_tab; if (idx >= 0) CloseTab(idx); return; }
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) RunGame();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    tab.code_editor->Render(tab.title.c_str(), avail);

    if (tab.code_editor->IsTextChanged()) tab.modified = true;
}

// ======================== IMAGE EDITOR ========================

void Editor::RenderImageEditor(EditorTab& tab) {
    if (!tab.image_data || !tab.image_texture) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Failed to load image");
        return;
    }

    ImVec2 content_size = ImGui::GetContentRegionAvail();
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::Button("Fit")) { tab.zoom = 1.0f; tab.pan_x = 0; tab.pan_y = 0; }
    ImGui::SameLine();
    if (ImGui::Button("+")) tab.zoom *= 1.25f;
    ImGui::SameLine();
    if (ImGui::Button("-")) tab.zoom /= 1.25f;
    ImGui::SameLine();
    ImGui::Text("Zoom: %.0f%%  %dx%d", tab.zoom * 100.0f, tab.img_w, tab.img_h);
    ImGui::Separator();

    ImVec2 canvas_size(content_size.x, content_size.y - ImGui::GetFrameHeightWithSpacing() - 20);
    ImGui::BeginChild("##ImageCanvas", canvas_size, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    float disp_w = tab.img_w * tab.zoom;
    float disp_h = tab.img_h * tab.zoom;
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float img_x = (avail.x - disp_w) * 0.5f + tab.pan_x;
    float img_y = (avail.y - disp_h) * 0.5f + tab.pan_y;
    ImGui::SetCursorPos(ImVec2(img_x, img_y));

    ImVec2 image_min = ImGui::GetCursorScreenPos();
    ImVec2 image_max(image_min.x + disp_w, image_min.y + disp_h);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float check_size = 8.0f * tab.zoom;
    if (check_size < 4.0f) check_size = 4.0f;
    for (float y = image_min.y; y < image_max.y; y += check_size) {
        for (float x = image_min.x; x < image_max.x; x += check_size) {
            bool light = ((int)((x - image_min.x) / check_size) + (int)((y - image_min.y) / check_size)) % 2 == 0;
            draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + check_size, y + check_size),
                light ? IM_COL32(200, 200, 200, 255) : IM_COL32(160, 160, 160, 255));
        }
    }

    if (ImGui::IsWindowHovered()) {
        if (io.MouseWheel != 0.0f) {
            tab.zoom *= (io.MouseWheel > 0) ? 1.1f : 0.9f;
            tab.zoom = std::max(0.05f, std::min(tab.zoom, 20.0f));
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            tab.pan_x += io.MouseDelta.x;
            tab.pan_y += io.MouseDelta.y;
        }
        if (io.KeyCtrl && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            tab.pan_x += io.MouseDelta.x;
            tab.pan_y += io.MouseDelta.y;
        }
    }
    ImGui::EndChild();
}

// ======================== NODE EDITOR ========================

void Editor::RenderNodeEditor(EditorTab& tab) {
    if (!tab.node_editor) return;

    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) SaveTab(tab);

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Toolbar for node editor
    if (ImGui::Button("Save")) SaveTab(tab);
    ImGui::SameLine();
    if (ImGui::Button("Export Lua")) ExportNodeGraphToLua(tab);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        tab.node_editor->Clear();
        tab.modified = true;
    }
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "Node Graph Editor");
    ImGui::Separator();

    tab.node_editor->Render(tab);
    if (tab.node_editor->IsModified()) tab.modified = true;
}

void Editor::OpenNodeGraph(const std::string& path) {
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        if (m_tabs[i].file_path == path) {
            m_active_tab = i;
            return;
        }
    }

    EditorTab tab;
    tab.file_path = path;
    tab.title = fs::path(path).filename().string();
    tab.type = EditorTabType::NodeGraph;
    tab.node_editor = std::make_unique<NodeEditor>();
    tab.node_editor->LoadFromFile(path);

    m_tabs.push_back(std::move(tab));
    m_active_tab = (int)m_tabs.size() - 1;
}

void Editor::CreateNewNodeGraph(const std::string& path) {
    EditorTab tab;
    tab.file_path = path;
    tab.title = fs::path(path).filename().string();
    tab.type = EditorTabType::NodeGraph;
    tab.node_editor = std::make_unique<NodeEditor>();
    tab.modified = true;

    m_tabs.push_back(std::move(tab));
    m_active_tab = (int)m_tabs.size() - 1;
}

void Editor::ExportNodeGraphToLua(EditorTab& tab) {
    if (!tab.node_editor) return;

    std::string lua_code = tab.node_editor->GenerateLua();

    // Save as .lua file next to the .nodemap
    std::string lua_path = tab.file_path;
    auto dot_pos = lua_path.rfind('.');
    if (dot_pos != std::string::npos) lua_path = lua_path.substr(0, dot_pos);
    lua_path += ".lua";

    std::ofstream f(lua_path, std::ios::binary);
    if (f.is_open()) {
        f.write(lua_code.data(), lua_code.size());
        f.close();
    }

    // Also open in a code tab for review
    OpenFileInTab(lua_path);
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

    // Helper to get a numeric property with fallback
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

    // Layout: left tree | center viewport | right properties
    float panel_w = 260.0f;
    ImVec2 avail = ImGui::GetContentRegionAvail();

    // ---- LEFT: Object tree ----
    ImGui::BeginChild("##SceneObjects", ImVec2(panel_w, avail.y), ImGuiChildFlags_Borders);
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
        ImGui::InputText("Name", tab.scene_new_name, sizeof(tab.scene_new_name));
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

    ImGui::SameLine(0, 0);

    // ---- CENTER: 2D Viewport ----
    float center_w = ImGui::GetContentRegionAvail().x - panel_w;
    if (center_w < 100) center_w = 100;
    ImGui::BeginChild("##SceneViewport", ImVec2(center_w, avail.y), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        if (canvas_sz.x < 50) canvas_sz.x = 50;
        if (canvas_sz.y < 50) canvas_sz.y = 50;
        ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(30, 30, 35, 255));

        // Viewport offset: center of canvas in world space
        float offset_x = canvas_p0.x + canvas_sz.x * 0.5f + tab.pan_x;
        float offset_y = canvas_p0.y + canvas_sz.y * 0.5f + tab.pan_y;
        float z = tab.zoom;

        // World<->Screen transforms (OpenGL: Y up)
        auto worldToScreen = [&](float wx, float wy) -> ImVec2 {
            return ImVec2(wx * z + offset_x, -wy * z + offset_y);
        };
        auto screenToWorld = [&](float sx, float sy) -> ImVec2 {
            return ImVec2((sx - offset_x) / z, -(sy - offset_y) / z);
        };
        // Convert world rect (bottom-left corner + size) to screen rect (top-left, bottom-right)
        auto worldRectToScreen = [&](double wx, double wy, double sx, double sy) -> std::pair<ImVec2, ImVec2> {
            ImVec2 bl = worldToScreen((float)wx, (float)wy);
            ImVec2 br = worldToScreen((float)(wx + sx), (float)wy);
            ImVec2 tl = worldToScreen((float)wx, (float)(wy + sy));
            ImVec2 tr = worldToScreen((float)(wx + sx), (float)(wy + sy));
            float min_x = std::min({bl.x, br.x, tl.x, tr.x});
            float max_x = std::max({bl.x, br.x, tl.x, tr.x});
            float min_y = std::min({bl.y, br.y, tl.y, tr.y});
            float max_y = std::max({bl.y, br.y, tl.y, tr.y});
            return {ImVec2(min_x, min_y), ImVec2(max_x, max_y)};
        };

        // Grid (adaptive world-unit steps using clean step values)
        static const float grid_steps[] = {10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
        float grid_step = 1000;
        for (float s : grid_steps) {
            if (s * z >= 60.0f) { grid_step = s; break; }
        }
        float grid_minor_pixels = grid_step * z * 0.2f; // minor every 1/5 of major
        bool draw_minor = (grid_minor_pixels > 8.0f);
        ImVec2 world_tl = screenToWorld(canvas_p0.x, canvas_p0.y);
        ImVec2 world_br = screenToWorld(canvas_p1.x, canvas_p1.y);
        float grid_start_x = floorf(world_tl.x / grid_step) * grid_step;
        float grid_start_y = floorf(world_tl.y / grid_step) * grid_step;
        if (draw_minor) {
            float mstep = grid_step * 0.2f;
            for (float wx = grid_start_x - grid_step; wx <= world_br.x + grid_step; wx += mstep) {
                ImVec2 a = worldToScreen(wx, world_tl.y);
                ImVec2 b = worldToScreen(wx, world_br.y);
                dl->AddLine(a, b, IM_COL32(36, 36, 42, 255));
            }
            for (float wy = grid_start_y - grid_step; wy <= world_br.y + grid_step; wy += mstep) {
                ImVec2 a = worldToScreen(world_tl.x, wy);
                ImVec2 b = worldToScreen(world_br.x, wy);
                dl->AddLine(a, b, IM_COL32(36, 36, 42, 255));
            }
        }
        for (float wx = grid_start_x; wx <= world_br.x + grid_step; wx += grid_step) {
            ImVec2 a = worldToScreen(wx, world_tl.y);
            ImVec2 b = worldToScreen(wx, world_br.y);
            dl->AddLine(a, b, IM_COL32(55, 55, 68, 255));
        }
        for (float wy = grid_start_y; wy <= world_br.y + grid_step; wy += grid_step) {
            ImVec2 a = worldToScreen(world_tl.x, wy);
            ImVec2 b = worldToScreen(world_br.x, wy);
            dl->AddLine(a, b, IM_COL32(55, 55, 68, 255));
        }
        // Origin axes
        {
            ImVec2 o = worldToScreen(0, 0);
            if (o.x >= canvas_p0.x && o.x <= canvas_p1.x) dl->AddLine(ImVec2(o.x, canvas_p0.y), ImVec2(o.x, canvas_p1.y), IM_COL32(80, 50, 50, 200));
            if (o.y >= canvas_p0.y && o.y <= canvas_p1.y) dl->AddLine(ImVec2(canvas_p0.x, o.y), ImVec2(canvas_p1.x, o.y), IM_COL32(50, 80, 50, 200));
        }

        // Draw objects
        int hovered_obj = -1;
        for (size_t i = 0; i < objects.size(); i++) {
            auto& obj = objects[i];
            if (!obj.is_object()) continue;

            double ox = getNum(obj, "pos_x", 0);
            double oy = getNum(obj, "pos_y", 0);
            std::string type = getStr(obj, "type", "");
            std::string name = getStr(obj, "name", "");
            ImVec2 sz = getDefaultSize(type);
            double sx = getNum(obj, "size_x", sz.x);
            double sy = getNum(obj, "size_y", sz.y);
            if (type == "point_light") { sx = getNum(obj, "radius", 60); sy = sx; }
            if (type == "tileset") { sx = getNum(obj, "tile_width", 32) * getNum(obj, "atlas_cols", 4); sy = getNum(obj, "tile_height", 32) * getNum(obj, "atlas_rows", 4); }

            double vx = ox;
            double vy = oy;
            double vsx = sx;
            double vsy = sy;

            // Compute world-space bounding box accounting for origin
            double wl = vx, wb = vy, wr = vx + vsx, wt = vy + vsy;
            if (type == "sprite" || type == "animated_sprite") {
                double origin_x = getNum(obj, "origin_x", 0.5);
                double origin_y = getNum(obj, "origin_y", 0.5);
                wl = vx - vsx * origin_x;
                wb = vy - vsy * origin_y;
                wr = wl + vsx;
                wt = wb + vsy;
            }

            auto [sc_min, sc_max] = worldRectToScreen(wl, wb, wr - wl, wt - wb);
            ImU32 col = getTypeColor(type);
            bool selected = ((int)i == tab.scene_selected_object);
            ImVec2 center = worldToScreen((float)vx, (float)vy);

            if (type == "point_light") {
                float r = (float)vsx * z;
                dl->AddCircleFilled(center, r, IM_COL32(255, 200, 80, 40));
                dl->AddCircle(center, r, IM_COL32(255, 200, 80, 160), 0, 2.0f * z);
                dl->AddCircleFilled(center, 5 * z, col);
            } else if (type == "ambient_light") {
                float r = (float)vsx * z;
                dl->AddCircleFilled(center, r, IM_COL32(255, 240, 140, 25));
                dl->AddCircle(center, r, IM_COL32(255, 240, 140, 80), 0, 1.0f * z);
            } else if (type == "camera") {
                float szz = 20 * z;
                dl->AddTriangleFilled(ImVec2(center.x - szz, center.y - szz), ImVec2(center.x + szz, center.y - szz), ImVec2(center.x, center.y + szz), col);
                dl->AddTriangle(ImVec2(center.x - szz, center.y - szz), ImVec2(center.x + szz, center.y - szz), ImVec2(center.x, center.y + szz), IM_COL32(255, 255, 255, 100), 1.5f * z);
            } else if (type == "particle_system") {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddCircleFilled(worldToScreen((float)(vx + vsx * 0.3), (float)(vy + vsy * 0.3)), 3 * z, IM_COL32(255, 255, 255, 180));
                dl->AddCircleFilled(worldToScreen((float)(vx + vsx * 0.7), (float)(vy + vsy * 0.7)), 3 * z, IM_COL32(255, 255, 255, 180));
                dl->AddCircleFilled(worldToScreen((float)(vx + vsx * 0.5), (float)(vy + vsy * 0.3)), 2 * z, IM_COL32(255, 255, 255, 180));
            } else if (type == "text") {
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddText(ImVec2(sc_min.x + 4 * z, sc_min.y + 4 * z), IM_COL32(255, 255, 255, 220), "T");
            } else if (type == "tileset") {
                dl->AddRectFilled(sc_min, sc_max, col);
                float tw = (float)getNum(obj, "tile_width", 32);
                float th = (float)getNum(obj, "tile_height", 32);
                int ac = (int)getNum(obj, "atlas_cols", 4);
                int ar = (int)getNum(obj, "atlas_rows", 4);
                for (int ri = 0; ri < ar; ri++) {
                    for (int ci = 0; ci < ac; ci++) {
                        ImVec2 t_tl = worldToScreen((float)(wl + ci * tw), (float)(wb + ri * th));
                        ImVec2 t_br = worldToScreen((float)(wl + (ci + 1) * tw), (float)(wb + (ri + 1) * th));
                        dl->AddRect(t_tl, t_br, IM_COL32(0, 0, 0, 80), 0, 0, 1.0f * z);
                    }
                }
            } else {
                // sprite / animated_sprite / fallback
                dl->AddRectFilled(sc_min, sc_max, col);
                dl->AddLine(ImVec2(sc_min.x, sc_min.y), ImVec2(sc_max.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
                dl->AddLine(ImVec2(sc_max.x, sc_min.y), ImVec2(sc_min.x, sc_max.y), IM_COL32(255, 255, 255, 40), 1.0f * z);
                if (type == "animated_sprite") {
                    dl->AddCircleFilled(center, 4 * z, IM_COL32(255, 255, 255, 120));
                }
            }

            // Name label below the object
            if (z > 0.3f) {
                ImVec2 label_pos = ImVec2(sc_min.x, sc_max.y + 2 * z);
                dl->AddText(ImVec2(label_pos.x + 1, label_pos.y + 1), IM_COL32(0, 0, 0, 180), name.c_str());
                dl->AddText(label_pos, IM_COL32(220, 220, 230, 220), name.c_str());
            }

            // Gizmo (mode-specific handles)
            if (selected && z > 0.05f) {
                float pad = 5 * z;
                ImU32 outline_col = IM_COL32(255, 200, 50, 255);
                float cx = (float)(wl + wr) * 0.5f;
                float cy = (float)(wb + wt) * 0.5f;
                ImVec2 oc = worldToScreen(cx, cy); // object center in screen

                // Selection outline (all modes)
                dl->AddRect(ImVec2(sc_min.x - pad, sc_min.y - pad), ImVec2(sc_max.x + pad, sc_max.y + pad),
                    outline_col, 0, 0, 2.0f * z);

                if (tab.scene_gizmo_mode == 0) {
                    // ---- MOVE gizmo: colored arrows ----
                    float arrow_len = 36 * z;
                    float head_len = 10 * z;
                    float head_w = 6 * z;
                    float line_w = 3.0f;

                    // X arrow (red) - points right
                    ImVec2 x_end = ImVec2(oc.x + arrow_len, oc.y);
                    ImU32 x_col = (tab.scene_drag_handle == 10) ? IM_COL32(255, 230, 50, 255) : IM_COL32(220, 70, 70, 255);
                    dl->AddLine(oc, x_end, x_col, line_w);
                    dl->AddTriangleFilled(
                        ImVec2(x_end.x + head_len, oc.y),
                        ImVec2(x_end.x, oc.y - head_w),
                        ImVec2(x_end.x, oc.y + head_w), x_col);

                    // Y arrow (green) - points up
                    ImVec2 y_end = ImVec2(oc.x, oc.y - arrow_len);
                    ImU32 y_col = (tab.scene_drag_handle == 11) ? IM_COL32(255, 230, 50, 255) : IM_COL32(70, 180, 70, 255);
                    dl->AddLine(oc, y_end, y_col, line_w);
                    dl->AddTriangleFilled(
                        ImVec2(oc.x, y_end.y - head_len),
                        ImVec2(oc.x - head_w, y_end.y),
                        ImVec2(oc.x + head_w, y_end.y), y_col);

                    // Center square
                    float cs = 5 * z;
                    ImU32 center_col = (tab.scene_drag_handle == 9) ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 230);
                    dl->AddRectFilled(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), center_col);
                    dl->AddRect(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), IM_COL32(60, 60, 60, 200));

                } else if (tab.scene_gizmo_mode == 1) {
                    // ---- ROTATE gizmo: circle + handle ----
                    float rot_r = std::max((float)vsx, (float)vsy) * 0.5f * z + 16 * z;
                    ImU32 circle_col = IM_COL32(80, 180, 255, 180);
                    dl->AddCircle(oc, rot_r, circle_col, 0, 2.0f * z);

                    // Rotation handle on the circle
                    double angle = getNum(obj, "rot", 0);
                    float rad = (float)(angle * 3.141592653589793 / 180.0);
                    ImVec2 handle_pos = ImVec2(oc.x + cosf(rad) * rot_r, oc.y - sinf(rad) * rot_r);
                    float hr = 6 * z;
                    ImU32 hcol = (tab.scene_drag_handle == 8) ? IM_COL32(255, 230, 50, 255) : IM_COL32(80, 200, 255, 230);
                    dl->AddLine(oc, handle_pos, IM_COL32(80, 180, 255, 120), 1.5f * z);
                    dl->AddCircleFilled(handle_pos, hr, hcol);
                    dl->AddCircle(handle_pos, hr, outline_col, 0, 1.5f * z);

                    // Rotation indicator line from center
                    float indicator_len = std::max((float)vsx, (float)vsy) * 0.5f * z + 12 * z;
                    ImVec2 dir = ImVec2(cosf(rad) * indicator_len, -sinf(rad) * indicator_len);
                    dl->AddLine(oc, ImVec2(oc.x + dir.x, oc.y + dir.y), IM_COL32(255, 200, 50, 200), 2.0f * z);
                    dl->AddCircleFilled(ImVec2(oc.x + dir.x, oc.y + dir.y), 3 * z, IM_COL32(255, 200, 50, 220));

                } else if (tab.scene_gizmo_mode == 2) {
                    // ---- SCALE gizmo: lines from center + cubes at corners/edges ----
                    float hs = 8 * z;
                    float hhs = hs * 0.5f;
                    float spad = pad;

                    ImVec2 corners[4] = {
                        ImVec2(sc_min.x - spad - hhs, sc_min.y - spad - hhs),
                        ImVec2(sc_max.x + spad - hhs, sc_min.y - spad - hhs),
                        ImVec2(sc_max.x + spad - hhs, sc_max.y + spad - hhs),
                        ImVec2(sc_min.x - spad - hhs, sc_max.y + spad - hhs)
                    };
                    ImVec2 edges[4] = {
                        ImVec2((sc_min.x + sc_max.x) * 0.5f - hhs, sc_min.y - spad - hhs),
                        ImVec2(sc_max.x + spad - hhs, (sc_min.y + sc_max.y) * 0.5f - hhs),
                        ImVec2((sc_min.x + sc_max.x) * 0.5f - hhs, sc_max.y + spad - hhs),
                        ImVec2(sc_min.x - spad - hhs, (sc_min.y + sc_max.y) * 0.5f - hhs)
                    };

                    // Lines from center to handles
                    for (int ci = 0; ci < 4; ci++) {
                        dl->AddLine(oc, ImVec2(corners[ci].x + hhs, corners[ci].y + hhs), IM_COL32(200, 200, 200, 80), 1.0f * z);
                    }
                    for (int ci = 0; ci < 4; ci++) {
                        dl->AddLine(oc, ImVec2(edges[ci].x + hhs, edges[ci].y + hhs), IM_COL32(200, 200, 200, 60), 1.0f * z);
                    }

                    // Corner cubes (0-3)
                    for (int ci = 0; ci < 4; ci++) {
                        ImU32 c = (tab.scene_drag_handle == ci) ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 230);
                        dl->AddRectFilled(corners[ci], ImVec2(corners[ci].x + hs, corners[ci].y + hs), c);
                        dl->AddRect(corners[ci], ImVec2(corners[ci].x + hs, corners[ci].y + hs), IM_COL32(100, 100, 100, 200));
                    }
                    // Edge cubes (4-7)
                    for (int ci = 0; ci < 4; ci++) {
                        ImU32 c = (tab.scene_drag_handle == ci + 4) ? IM_COL32(255, 230, 50, 255) : IM_COL32(200, 200, 220, 230);
                        dl->AddRectFilled(edges[ci], ImVec2(edges[ci].x + hs, edges[ci].y + hs), c);
                        dl->AddRect(edges[ci], ImVec2(edges[ci].x + hs, edges[ci].y + hs), IM_COL32(100, 100, 100, 200));
                    }

                    // Center square for uniform scale
                    float cs = 5 * z;
                    ImU32 center_col = (tab.scene_drag_handle == 9) ? IM_COL32(255, 230, 50, 255) : IM_COL32(255, 255, 255, 200);
                    dl->AddRectFilled(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), center_col);
                    dl->AddRect(ImVec2(oc.x - cs, oc.y - cs), ImVec2(oc.x + cs, oc.y + cs), IM_COL32(60, 60, 60, 200));
                }
            }

            // Hit-test for hover (world space, Y-up, origin-aware)
            if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(canvas_p0, canvas_p1)) {
                ImVec2 mouse_world = screenToWorld(io.MousePos.x, io.MousePos.y);
                if (mouse_world.x >= wl && mouse_world.x <= wr
                    && mouse_world.y >= wb && mouse_world.y <= wt) {
                    hovered_obj = (int)i;
                }
            }
        }

        // Viewport interaction
        if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(canvas_p0, canvas_p1)) {
            // Zoom
            if (io.MouseWheel != 0.0f) {
                tab.zoom *= (io.MouseWheel > 0) ? 1.1f : 0.9f;
                tab.zoom = std::max(0.05f, std::min(tab.zoom, 20.0f));
                z = tab.zoom;
                // Zoom towards mouse
                ImVec2 mouse_w = screenToWorld(io.MousePos.x, io.MousePos.y);
                offset_x = io.MousePos.x - mouse_w.x * z;
                offset_y = io.MousePos.y - mouse_w.y * z;
                tab.pan_x = offset_x - (canvas_p0.x + canvas_sz.x * 0.5f);
                tab.pan_y = offset_y - (canvas_p0.y + canvas_sz.y * 0.5f);
            }

            // Pan with middle mouse
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
                tab.pan_x += io.MouseDelta.x;
                tab.pan_y += io.MouseDelta.y;
            }

            // Gizmo mode interaction
            int sel = tab.scene_selected_object;
            bool on_handle = false;

            // Keyboard shortcuts for gizmo mode switch
            if (ImGui::IsKeyPressed(ImGuiKey_W)) { tab.scene_gizmo_mode = 0; tab.scene_drag_handle = -1; }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) { tab.scene_gizmo_mode = 1; tab.scene_drag_handle = -1; }
            if (ImGui::IsKeyPressed(ImGuiKey_R)) { tab.scene_gizmo_mode = 2; tab.scene_drag_handle = -1; }

            if (sel >= 0 && sel < (int)objects.size()) {
                auto& sobj = objects[sel];
                if (sobj.is_object()) {
                    // Recompute bounds for the selected object
                    std::string stype = getStr(sobj, "type", "");
                    ImVec2 ssz = getDefaultSize(stype);
                    double sox = getNum(sobj, "pos_x", 0);
                    double soy = getNum(sobj, "pos_y", 0);
                    double ssx = getNum(sobj, "size_x", ssz.x);
                    double ssy = getNum(sobj, "size_y", ssz.y);
                    if (stype == "point_light") { ssx = getNum(sobj, "radius", 48); ssy = ssx; }
                    if (stype == "tileset") { ssx = getNum(sobj, "tile_width", 32) * getNum(sobj, "atlas_cols", 4); ssy = getNum(sobj, "tile_height", 32) * getNum(sobj, "atlas_rows", 4); }
                    double svx = sox, svy = soy;
                    double svsx = ssx, svsy = ssy;
                    double swl = svx, swb = svy, swr = svx + svsx, swt = svy + svsy;
                    if (stype == "sprite" || stype == "animated_sprite") {
                        double sorigin_x = getNum(sobj, "origin_x", 0.5);
                        double sorigin_y = getNum(sobj, "origin_y", 0.5);
                        swl = svx - svsx * sorigin_x;
                        swb = svy - svsy * sorigin_y;
                        swr = swl + svsx;
                        swt = swb + svsy;
                    }
                    // Screen-space bbox
                    auto [ssc_min, ssc_max] = worldRectToScreen(swl, swb, swr - swl, swt - swb);
                    float spad = 5 * z;
                    float center_x = (float)(swl + swr) * 0.5f;
                    float center_y = (float)(swb + swt) * 0.5f;
                    ImVec2 oc = worldToScreen(center_x, center_y);

                    ImVec2 mouse = io.MousePos;

                    if (tab.scene_drag_handle >= 0) {
                        on_handle = true;
                        ImVec2 mouse_world = screenToWorld(mouse.x, mouse.y);

                        if (tab.scene_gizmo_mode == 0) {
                            // --- MOVE drag ---
                            if (tab.scene_drag_handle == 10) {
                                // X axis only
                                double dx = mouse_world.x - tab.scene_drag_vx;
                                setNum(sobj, "pos_x", tab.scene_drag_vx + dx);
                            } else if (tab.scene_drag_handle == 11) {
                                // Y axis only
                                double dy = mouse_world.y - tab.scene_drag_vy;
                                setNum(sobj, "pos_y", tab.scene_drag_vy + dy);
                            } else {
                                // Free move (center square or object body)
                                setNum(sobj, "pos_x", sox + io.MouseDelta.x / z);
                                setNum(sobj, "pos_y", soy - io.MouseDelta.y / z);
                            }
                            tab.modified = true;

                        } else if (tab.scene_gizmo_mode == 1) {
                            // --- ROTATE drag ---
                            float dx = mouse_world.x - center_x;
                            float dy = mouse_world.y - center_y;
                            double deg = atan2(dy, dx) * 180.0 / 3.141592653589793;
                            setNum(sobj, "rot", deg);
                            tab.modified = true;

                        } else if (tab.scene_gizmo_mode == 2) {
                            // --- SCALE drag (same as before) ---
                            double nwl = tab.scene_drag_wl, nwb = tab.scene_drag_wb;
                            double nwr = tab.scene_drag_wr, nwt = tab.scene_drag_wt;

                            if (tab.scene_drag_handle == 9) {
                                // Uniform scale via center square
                                double dx = io.MouseDelta.x / z;
                                double dy = -io.MouseDelta.y / z;
                                double avg = (dx + dy) * 0.5;
                                double nsx = tab.scene_drag_vsx + avg;
                                double nsy = tab.scene_drag_vsy + avg;
                                if (nsx < 0.1) nsx = 0.1;
                                if (nsy < 0.1) nsy = 0.1;
                                double sorigin_x = getNum(sobj, "origin_x", 0.5);
                                double sorigin_y = getNum(sobj, "origin_y", 0.5);
                                // Keep origin fixed during uniform scale
                                double wl_orig = tab.scene_drag_vx - tab.scene_drag_vsx * sorigin_x;
                                double wb_orig = tab.scene_drag_vy - tab.scene_drag_vsy * sorigin_y;
                                setNum(sobj, "pos_x", wl_orig + nsx * sorigin_x);
                                setNum(sobj, "pos_y", wb_orig + nsy * sorigin_y);
                                setNum(sobj, "size_x", nsx);
                                setNum(sobj, "size_y", nsy);
                            } else {
                                switch (tab.scene_drag_handle) {
                                    case 0: nwl = mouse_world.x; nwt = mouse_world.y; break;
                                    case 1: nwr = mouse_world.x; nwt = mouse_world.y; break;
                                    case 2: nwr = mouse_world.x; nwb = mouse_world.y; break;
                                    case 3: nwl = mouse_world.x; nwb = mouse_world.y; break;
                                    case 4: nwt = mouse_world.y; break;
                                    case 5: nwr = mouse_world.x; break;
                                    case 6: nwb = mouse_world.y; break;
                                    case 7: nwl = mouse_world.x; break;
                                }
                                double min_size = 0.1;
                                int dh = tab.scene_drag_handle;
                                if (dh == 1 || dh == 2 || dh == 5) {
                                    if (nwr - nwl < min_size) nwr = nwl + min_size;
                                } else if (dh == 0 || dh == 3 || dh == 7) {
                                    if (nwr - nwl < min_size) nwl = nwr - min_size;
                                }
                                if (dh == 0 || dh == 1 || dh == 4) {
                                    if (nwt - nwb < min_size) nwt = nwb + min_size;
                                } else if (dh == 2 || dh == 3 || dh == 6) {
                                    if (nwt - nwb < min_size) nwb = nwt - min_size;
                                }
                                double new_sx = nwr - nwl;
                                double new_sy = nwt - nwb;
                                double sorigin_x = getNum(sobj, "origin_x", 0.5);
                                double sorigin_y = getNum(sobj, "origin_y", 0.5);
                                double new_px = nwl + new_sx * sorigin_x;
                                double new_py = nwb + new_sy * sorigin_y;
                                setNum(sobj, "pos_x", new_px);
                                setNum(sobj, "pos_y", new_py);
                                setNum(sobj, "size_x", new_sx);
                                setNum(sobj, "size_y", new_sy);
                            }
                            tab.modified = true;
                        }

                        // Release drag
                        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                            tab.scene_drag_handle = -1;
                        }

                    } else {
                        // Not dragging — check hover for gizmo parts
                        float hsz = 8 * z;

                        if (tab.scene_gizmo_mode == 0) {
                            // MOVE: check X arrow, Y arrow, center square
                            float arrow_len = 36 * z;
                            float arrow_w = 8 * z;
                            // X arrow hit area
                            float ax1 = oc.x - 5 * z, ax2 = oc.x + arrow_len + 10 * z;
                            float ay1 = oc.y - arrow_w, ay2 = oc.y + arrow_w;
                            // Y arrow hit area
                            float yx1 = oc.x - arrow_w, yx2 = oc.x + arrow_w;
                            float yy1 = oc.y - arrow_len - 10 * z, yy2 = oc.y + 5 * z;
                            // Center square
                            float cs = 7 * z;
                            bool on_x = (mouse.x >= ax1 && mouse.x <= ax2 && mouse.y >= ay1 && mouse.y <= ay2);
                            bool on_y = (mouse.x >= yx1 && mouse.x <= yx2 && mouse.y >= yy1 && mouse.y <= yy2);
                            bool on_ct = (mouse.x >= oc.x - cs && mouse.x <= oc.x + cs && mouse.y >= oc.y - cs && mouse.y <= oc.y + cs);

                            if (on_x) {
                                on_handle = true;
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                    tab.scene_drag_handle = 10;
                                    tab.scene_drag_vx = svx;
                                    tab.scene_drag_vy = svy;
                                }
                            } else if (on_y) {
                                on_handle = true;
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                    tab.scene_drag_handle = 11;
                                    tab.scene_drag_vx = svx;
                                    tab.scene_drag_vy = svy;
                                }
                            } else if (on_ct) {
                                on_handle = true;
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                    tab.scene_drag_handle = 9; // center square = free move
                                    tab.scene_drag_vx = svx;
                                    tab.scene_drag_vy = svy;
                                }
                            }

                        } else if (tab.scene_gizmo_mode == 1) {
                            // ROTATE: check circle handle
                            float rot_r = std::max((float)svsx, (float)svsy) * 0.5f * z + 16 * z;
                            double angle = getNum(sobj, "rot", 0);
                            float rad = (float)(angle * 3.141592653589793 / 180.0);
                            float hx = oc.x + cosf(rad) * rot_r;
                            float hy = oc.y - sinf(rad) * rot_r;
                            float hr = 8 * z;
                            if (mouse.x >= hx - hr && mouse.x <= hx + hr && mouse.y >= hy - hr && mouse.y <= hy + hr) {
                                on_handle = true;
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                    tab.scene_drag_handle = 8;
                                    // Save initial pos for rotation
                                    tab.scene_drag_vx = svx;
                                    tab.scene_drag_vy = svy;
                                }
                            }

                        } else if (tab.scene_gizmo_mode == 2) {
                            // SCALE: check corner/edge/center handles
                            float shs = hsz * 0.5f;
                            ImVec2 sc[9];
                            sc[0] = ImVec2(ssc_min.x - spad - shs, ssc_min.y - spad - shs);
                            sc[1] = ImVec2(ssc_max.x + spad - shs, ssc_min.y - spad - shs);
                            sc[2] = ImVec2(ssc_max.x + spad - shs, ssc_max.y + spad - shs);
                            sc[3] = ImVec2(ssc_min.x - spad - shs, ssc_max.y + spad - shs);
                            sc[4] = ImVec2((ssc_min.x + ssc_max.x) * 0.5f - shs, ssc_min.y - spad - shs);
                            sc[5] = ImVec2(ssc_max.x + spad - shs, (ssc_min.y + ssc_max.y) * 0.5f - shs);
                            sc[6] = ImVec2((ssc_min.x + ssc_max.x) * 0.5f - shs, ssc_max.y + spad - shs);
                            sc[7] = ImVec2(ssc_min.x - spad - shs, (ssc_min.y + ssc_max.y) * 0.5f - shs);
                            float cs = 7 * z;
                            sc[8] = ImVec2(oc.x - cs, oc.y - cs); // center square (handle 9)

                            for (int hi = 0; hi < 9; hi++) {
                                int handle_id = (hi == 8) ? 9 : hi;
                                ImVec2 hmin = sc[hi];
                                ImVec2 hmax = (hi == 8)
                                    ? ImVec2(hmin.x + cs * 2, hmin.y + cs * 2)
                                    : ImVec2(hmin.x + hsz, hmin.y + hsz);
                                if (mouse.x >= hmin.x && mouse.x <= hmax.x && mouse.y >= hmin.y && mouse.y <= hmax.y) {
                                    on_handle = true;
                                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                        tab.scene_drag_handle = handle_id;
                                        tab.scene_drag_wl = swl; tab.scene_drag_wb = swb;
                                        tab.scene_drag_wr = swr; tab.scene_drag_wt = swt;
                                        tab.scene_drag_vx = svx; tab.scene_drag_vy = svy;
                                        tab.scene_drag_vsx = svsx; tab.scene_drag_vsy = svsy;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Select on click (only if not on handle)
            if (!on_handle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (hovered_obj >= 0) {
                    tab.scene_selected_object = hovered_obj;
                } else {
                    tab.scene_selected_object = -1;
                }
            }

            // Drag to move when not on gizmo (free move)
            if (!on_handle && tab.scene_drag_handle < 0
                && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && sel >= 0 && sel < (int)objects.size()) {
                auto& mobj = objects[sel];
                if (mobj.is_object()) {
                    double old_x = getNum(mobj, "pos_x", 0);
                    double old_y = getNum(mobj, "pos_y", 0);
                    setNum(mobj, "pos_x", old_x + io.MouseDelta.x / z);
                    setNum(mobj, "pos_y", old_y - io.MouseDelta.y / z);
                    tab.modified = true;
                }
            }

            // Clear drag if mouse released
            if (tab.scene_drag_handle >= 0 && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                tab.scene_drag_handle = -1;
            }
        }

        // Hover tooltip
        if (hovered_obj >= 0 && hovered_obj < (int)objects.size()) {
            auto& obj = objects[hovered_obj];
            std::string name = getStr(obj, "name", "Unnamed");
            std::string type = getStr(obj, "type", "?");
            ImVec2 dsz = getDefaultSize(type);
            double px = getNum(obj, "pos_x", 0);
            double py = getNum(obj, "pos_y", 0);
            double sx = getNum(obj, "size_x", dsz.x);
            double sy = getNum(obj, "size_y", dsz.y);
            double rot = getNum(obj, "rot", 0);
            ImGui::SetTooltip("[%s] %s\npos: %.2f, %.2f  size: %.2f x %.2f  rot: %.1f\xC2\xB0",
                type.c_str(), name.c_str(), px, py, sx, sy, rot);
        }
    }
    ImGui::EndChild();

    ImGui::SameLine(0, 0);

    // ---- RIGHT: Properties ----
    ImGui::BeginChild("##SceneProperties", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
    {
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
                if (ImGui::InputText("Name", name_buf, sizeof(name_buf))) {
                    obj["name"] = std::string(name_buf);
                    tab.modified = true;
                }

                char type_buf[64];
                strncpy(type_buf, type.c_str(), sizeof(type_buf) - 1);
                type_buf[sizeof(type_buf) - 1] = '\0';
                if (ImGui::InputText("Type", type_buf, sizeof(type_buf))) {
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
    }
    ImGui::EndChild();
}

bool Editor::ShouldReturnToHub() const { return m_return_to_hub; }
void Editor::ResetReturnFlag() { m_return_to_hub = false; }
