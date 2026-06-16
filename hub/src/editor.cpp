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
           ext == ".xml" || ext == ".yaml" || ext == ".yml" || ext == ".cfg" ||
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

    if (IsImageFile(ext)) {
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
    fs::path exe = m_project_path / "game.exe";
    if (!fs::exists(exe)) {
        system(("explorer \"" + m_project_path.string() + "\"").c_str());
        return;
    }
    std::string bat = "@echo off\ncd /d \"" + m_project_path.string() + "\"\nstart \"\" game.exe\n";
    fs::path bat_path = m_project_path / "_run_temp.bat";
    {
        std::ofstream f(bat_path);
        f << bat;
    }
    system(("cmd /c \"" + bat_path.string() + "\"").c_str());
    fs::remove(bat_path);
#else
    fs::path exe = m_project_path / "linux_release";
    if (!fs::exists(exe)) {
        system(("xdg-open \"" + m_project_path.string() + "\"").c_str());
        return;
    }
    system(("cd \"" + m_project_path.string() + "\" && ./linux_release &").c_str());
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

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

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

bool Editor::ShouldReturnToHub() const { return m_return_to_hub; }
void Editor::ResetReturnFlag() { m_return_to_hub = false; }
