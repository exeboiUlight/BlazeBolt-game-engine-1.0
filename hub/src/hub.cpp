#include "hub.hpp"
#include "stb_image.h"
#include <glad/glad.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#undef LoadIcon

static std::string PickFolder(const char* title = "Select Folder") {
    std::string result;
    WCHAR wtitle[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 256);
    BROWSEINFOW bi = {0};
    bi.lpszTitle = wtitle;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        WCHAR path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
            if (len > 0) {
                result.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, path, -1, &result[0], len, NULL, NULL);
            }
        }
        IMalloc* pMalloc = nullptr;
        if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
            pMalloc->Free(pidl);
            pMalloc->Release();
        }
    }
    return result;
}
#else
#include <cstring>

static std::string PickFolder(const char* title = "Select Folder") {
    std::string cmd = "zenity --file-selection --directory --title=\"";
    cmd += title;
    cmd += "\" 2>/dev/null";
    std::string result;
    FILE* f = popen(cmd.c_str(), "r");
    if (f) {
        char buf[1024];
        if (fgets(buf, sizeof(buf), f)) {
            result = buf;
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
        }
        pclose(f);
    }
    return result;
}
#endif

Hub::Hub(fs::path engine_root)
    : m_engine_root(engine_root)
    , m_projects_file(engine_root / "projects.txt")
{
    LoadProjects();
    LoadTheme();
}

Hub::~Hub() {
    for (auto& p : m_projects) UnloadIcon(p);
}

void Hub::LoadProjects() {
    m_projects.clear();
    std::ifstream file(m_projects_file);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t sep = line.find('|');
        if (sep == std::string::npos) continue;
        Project proj;
        proj.name = line.substr(0, sep);
        proj.path = line.substr(sep + 1);
        if (fs::exists(proj.path)) {
            LoadIcon(proj);
            m_projects.push_back(proj);
        }
    }
}

void Hub::SaveProjects() {
    std::ofstream file(m_projects_file, std::ios::trunc);
    for (auto& p : m_projects) {
        file << p.name << "|" << p.path << "\n";
    }
}

void Hub::CreateProject(const std::string& name, const std::string& path, ProjectType type) {
    fs::path project_dir = fs::path(path) / name;
    fs::path zip_path = m_engine_root / "void.zip";

    if (!fs::exists(zip_path)) {
        snprintf(m_error_msg, sizeof(m_error_msg), "void.zip not found in engine root");
        m_show_error = true;
        return;
    }

    if (fs::exists(project_dir)) {
        snprintf(m_error_msg, sizeof(m_error_msg), "Directory already exists: %s", project_dir.string().c_str());
        m_show_error = true;
        return;
    }

    fs::create_directories(project_dir);

    if (!ExtractZip(zip_path, project_dir)) {
        snprintf(m_error_msg, sizeof(m_error_msg), "Failed to extract void.zip");
        m_show_error = true;
        fs::remove_all(project_dir);
        return;
    }

    if (type == ProjectType::Code) {
        fs::path engine_dir = project_dir / "engine";
        if (!fs::exists(engine_dir)) {
            fs::create_directories(engine_dir);
        }
    }

    Project proj;
    proj.name = name;
    proj.path = project_dir.string();
    LoadIcon(proj);
    m_projects.push_back(proj);
    SaveProjects();
}

void Hub::LoadIcon(Project& proj) {
    fs::path icon_path = fs::path(proj.path) / "icon.png";
    if (!fs::exists(icon_path)) {
        proj.icon_texture = (ImTextureID)0;
        return;
    }

    int w, h, channels;
    unsigned char* data = stbi_load(icon_path.string().c_str(), &w, &h, &channels, 4);
    if (!data) {
        proj.icon_texture = (ImTextureID)0;
        return;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    proj.icon_texture = (ImTextureID)(intptr_t)tex;
    proj.icon_w = w;
    proj.icon_h = h;

    stbi_image_free(data);
}

void Hub::UnloadIcon(Project& proj) {
    if (proj.icon_texture) {
        GLuint tex = (GLuint)(intptr_t)proj.icon_texture;
        glDeleteTextures(1, &tex);
        proj.icon_texture = (ImTextureID)0;
    }
}

bool Hub::ExtractZip(const fs::path& zip, const fs::path& dest) {
#ifdef PLATFORM_WINDOWS
    std::string cmd = "powershell -NoProfile -Command \"Expand-Archive -Path '" +
                      zip.string() + "' -DestinationPath '" + dest.string() + "' -Force\"";
#else
    std::string cmd = "unzip -o \"" + zip.string() + "\" -d \"" + dest.string() + "\"";
#endif
    int result = system(cmd.c_str());
    return result == 0;
}

void Hub::LoadTheme() {
    std::ifstream f(m_engine_root / "theme.txt");
    if (f.is_open()) {
        int idx = 2;
        f >> idx;
        if (idx >= 0 && idx <= 4) m_current_theme = idx;
    }
}

void Hub::SaveTheme() {
    std::ofstream f(m_engine_root / "theme.txt", std::ios::trunc);
    f << m_current_theme;
}

void Hub::ApplyTheme(int idx) {
    m_current_theme = idx;
    switch (idx) {
        case 0: ImGui::StyleColorsDark(); break;
        case 1: ImGui::StyleColorsLight(); break;
        case 2: ImGui::StyleColorsClassic(); break;
        case 3: ImGui::StyleColorsDark(); break;
        case 4: ImGui::StyleColorsDark(); break;
        default: ImGui::StyleColorsDark(); break;
    }
    ImGuiStyle& s = ImGui::GetStyle();
    if (idx == 0) {
        s.WindowRounding = 6; s.FrameRounding = 4; s.GrabRounding = 4; s.ScrollbarRounding = 6;
        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        c[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        c[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        c[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.19f, 1.0f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.31f, 1.0f);
        c[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.68f, 1.0f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.50f, 0.80f, 1.0f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.36f, 0.62f, 1.0f);
        c[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.68f, 0.40f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.40f, 0.68f, 0.60f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.40f, 0.68f, 0.80f);
        c[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 0.60f);
        c[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.34f, 1.0f);
        c[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.17f, 1.0f);
        c[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.42f, 0.70f, 0.80f);
        c[ImGuiCol_TabActive] = ImVec4(0.20f, 0.40f, 0.68f, 1.0f);
    } else if (idx == 1) {
        s.WindowRounding = 6; s.FrameRounding = 4; s.GrabRounding = 4; s.ScrollbarRounding = 6;
        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.96f, 1.0f);
        c[ImGuiCol_ChildBg] = ImVec4(0.90f, 0.90f, 0.92f, 1.0f);
        c[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 1.0f, 1.0f);
        c[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.88f, 0.92f, 0.98f, 1.0f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.88f, 0.98f, 1.0f);
        c[ImGuiCol_Button] = ImVec4(0.24f, 0.48f, 0.78f, 1.0f);
        c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.40f, 0.72f, 1.0f);
        c[ImGuiCol_ButtonActive] = ImVec4(0.14f, 0.34f, 0.66f, 1.0f);
        c[ImGuiCol_Header] = ImVec4(0.24f, 0.48f, 0.78f, 0.30f);
        c[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.48f, 0.78f, 0.50f);
        c[ImGuiCol_HeaderActive] = ImVec4(0.24f, 0.48f, 0.78f, 0.70f);
        c[ImGuiCol_Tab] = ImVec4(0.85f, 0.85f, 0.90f, 1.0f);
        c[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.48f, 0.78f, 0.70f);
        c[ImGuiCol_TabActive] = ImVec4(0.24f, 0.48f, 0.78f, 1.0f);
    } else if (idx == 2) {
        // Classic - no extra overrides needed
    } else if (idx == 3) {
        ImGui::StyleColorsDark();
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
    } else if (idx == 4) {
        ImGui::StyleColorsDark();
        s.WindowRounding = 8; s.FrameRounding = 6; s.GrabRounding = 6; s.ScrollbarRounding = 6;
        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.14f, 0.20f, 1.0f);
        c[ImGuiCol_ChildBg] = ImVec4(0.18f, 0.17f, 0.24f, 1.0f);
        c[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.17f, 0.24f, 1.0f);
        c[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.42f, 0.50f);
        c[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.21f, 0.30f, 1.0f);
        c[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.27f, 0.38f, 1.0f);
        c[ImGuiCol_FrameBgActive] = ImVec4(0.34f, 0.33f, 0.46f, 1.0f);
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
    }
}

void Hub::Render() {
    ApplyTheme(m_current_theme);

    ImGuiStyle& hub_style = ImGui::GetStyle();
    hub_style.WindowRounding = 8.0f;
    hub_style.FrameRounding = 6.0f;
    hub_style.GrabRounding = 6.0f;
    hub_style.ScrollbarRounding = 6.0f;
    hub_style.WindowBorderSize = 0.0f;
    hub_style.FrameBorderSize = 0.0f;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##Hub", nullptr, flags);

    float window_h = ImGui::GetContentRegionAvail().y;
    float bottom_bar_h = 60.0f;

    ImGui::SetCursorPosY(20.0f);
    ImVec2 title_size = ImGui::CalcTextSize("BlazeBolt Engine");
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - title_size.x) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.90f, 1.0f));
    ImGui::Text("BlazeBolt Engine");
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
    ImGui::Separator();
    ImGui::Spacing();

    float cards_area_h = window_h - bottom_bar_h - ImGui::GetCursorPosY();
    ImGui::BeginChild("##CardsArea", ImVec2(0, cards_area_h));

    float available_width = ImGui::GetContentRegionAvail().x;
    float card_width = 320.0f;
    int columns = std::max(1, (int)((available_width + ImGui::GetStyle().ItemSpacing.x) / (card_width + ImGui::GetStyle().ItemSpacing.x)));
    float total_cards_width = columns * card_width + (columns - 1) * ImGui::GetStyle().ItemSpacing.x;
    float cards_start_x = (available_width - total_cards_width) * 0.5f;
    if (cards_start_x < 0) cards_start_x = 0;

    ImGui::SetCursorPosX(cards_start_x);

    int col = 0;
    for (size_t i = 0; i < m_projects.size(); i++) {
        auto& proj = m_projects[i];

        ImGui::PushID((int)i);

        // FIXME: I've added ImGuiChildFlags_AutoResizeY out of context, idk what it related to, I just wanted to fix this error: "Must use ImGuiChildFlags_AutoResizeX or ImGuiChildFlags_AutoResizeY with ImGuiChildFlags_AlwaysAutoResize!"
        ImGui::BeginChild(("project_card_" + std::to_string(i)).c_str(), ImVec2(card_width, 72.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY);

        float card_h = ImGui::GetContentRegionAvail().y;
        float icon_size = 48.0f;
        float padding = 8.0f;

        if (proj.icon_texture) {
            ImGui::SetCursorPosX(padding);
            ImGui::SetCursorPosY((card_h - icon_size) * 0.5f);
            ImGui::Image(proj.icon_texture, ImVec2(icon_size, icon_size));
        } else {
            ImGui::SetCursorPosX(padding);
            ImGui::SetCursorPosY((card_h - icon_size) * 0.5f);
            ImVec4 tint(0.3f, 0.5f, 0.8f, 1.0f);
            ImGui::Dummy(ImVec2(icon_size, icon_size));
            ImVec2 dmin = ImGui::GetItemRectMin();
            ImVec2 dmax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddRectFilled(dmin, dmax, IM_COL32(40, 60, 100, 255), 6.0f);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(dmin.x + icon_size * 0.3f, dmin.y + icon_size * 0.3f),
                IM_COL32(200, 200, 210, 255), "BB");
        }

        float text_x = icon_size + padding * 2.0f;
        float text_max_w = card_width - text_x - padding;

        ImGui::SetCursorPosX(text_x);
        ImGui::SetCursorPosY(padding + 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.92f, 0.95f, 1.0f));
        ImGui::PushTextWrapPos(text_x + text_max_w);
        ImGui::TextUnformatted(proj.name.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        ImGui::SetCursorPosX(text_x);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));
        ImGui::PushTextWrapPos(text_x + text_max_w);
        float small_font_scale = 0.85f;
        ImGui::SetWindowFontScale(small_font_scale);
        ImGui::TextUnformatted(proj.path.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
            m_selected_path = proj.path;
            m_open_editor = true;
        }

        if (ImGui::BeginPopupContextItem(("ProjectContextMenu_" + std::to_string(i)).c_str())) {
            if (ImGui::MenuItem("Open")) {
                m_selected_path = proj.path;
                m_open_editor = true;
            }
            if (ImGui::MenuItem("Show in Explorer")) {
                std::string cmd;
#ifdef PLATFORM_WINDOWS
                cmd = "explorer \"" + proj.path + "\"";
#else
                cmd = "xdg-open \"" + proj.path + "\"";
#endif
                system(cmd.c_str());
            }
            if (ImGui::MenuItem("Remove from List")) {
                UnloadIcon(proj);
                m_projects.erase(m_projects.begin() + i);
                SaveProjects();
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            if (ImGui::MenuItem("Delete Project")) {
                fs::remove_all(proj.path);
                UnloadIcon(proj);
                m_projects.erase(m_projects.begin() + i);
                SaveProjects();
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();

        col++;
        if (col < columns) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        } else {
            col = 0;
        }

        ImGui::PopID();
    }

    if (m_projects.empty()) {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 60.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.50f, 1.0f));
        ImVec2 text_size = ImGui::CalcTextSize("No projects yet. Click \"New Project\" to get started.");
        ImGui::SetCursorPosX((available_width - text_size.x) * 0.5f);
        ImGui::TextUnformatted("No projects yet. Click \"New Project\" to get started.");
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    ImGui::Separator();

    float button_width = 140.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float total_buttons_width = button_width * 2 + spacing;
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - total_buttons_width) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.0f, 10.0f));

    if (ImGui::Button("New Project", ImVec2(button_width, 0))) {
        m_show_create_popup = true;
        m_new_project_type = ProjectType::Code;
        memset(m_new_name, 0, sizeof(m_new_name));
        memset(m_new_path, 0, sizeof(m_new_path));
        fs::path default_path = m_engine_root / "projects";
        strncpy(m_new_path, default_path.string().c_str(), sizeof(m_new_path) - 1);
    }
    ImGui::SameLine();
    if (ImGui::Button("Open Folder", ImVec2(button_width, 0))) {
        std::string folder = PickFolder("Select Project Folder");
        if (!folder.empty()) {
            m_selected_path = folder;
            m_open_editor = true;
        }
    }

    ImGui::PopStyleVar(2);

    ImGui::SameLine(0, 40.0f);
    const char* themes[] = { "Dark", "Light", "Classic", "Cyberpunk", "Dracula" };
    if (ImGui::BeginCombo("##Theme", themes[m_current_theme])) {
        for (int i = 0; i < 5; i++) {
            bool selected = (m_current_theme == i);
            if (ImGui::Selectable(themes[i], selected)) {
                ApplyTheme(i);
                SaveTheme();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::End();

    if (m_show_create_popup) {
        ImGui::OpenPopup("Create New Project");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    float popup_w = ImGui::GetMainViewport()->Size.x * 0.75f;
    ImGui::SetNextWindowSize(ImVec2(popup_w, 0.0f));
    if (ImGui::BeginPopupModal("Create New Project", nullptr)) {
        ImGui::Text("Project Name:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##name", m_new_name, sizeof(m_new_name));

        ImGui::Spacing();
        ImGui::Text("Location:");

        float browse_w = 80.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float input_w = ImGui::GetContentRegionAvail().x - browse_w - spacing;
        ImGui::SetNextItemWidth(input_w);
        ImGui::InputText("##path", m_new_path, sizeof(m_new_path));
        ImGui::SameLine();
        if (ImGui::Button("Browse...", ImVec2(browse_w, 0))) {
            std::string folder = PickFolder("Select Project Location");
            if (!folder.empty()) {
                strncpy(m_new_path, folder.c_str(), sizeof(m_new_path) - 1);
            }
        }

        if (m_new_name[0] != '\0') {
            fs::path preview = fs::path(m_new_path) / m_new_name;
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "Will create: %s", preview.string().c_str());
        }

        ImGui::Text("Project type: Code Editor (Lua)");

        ImGui::Spacing();

        bool can_create = m_new_name[0] != '\0' && m_new_path[0] != '\0';
        if (!can_create) ImGui::BeginDisabled();
        if (ImGui::Button("Create", ImVec2(120, 0))) {
            CreateProject(m_new_name, m_new_path, m_new_project_type);
            if (!m_show_error) {
                ImGui::CloseCurrentPopup();
                m_show_create_popup = false;
            }
        }
        if (!can_create) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            m_show_create_popup = false;
        }

        ImGui::EndPopup();
    }

    if (m_show_error) {
        ImGui::OpenPopup("Error");
        m_show_error = false;
    }
    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", m_error_msg);
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

bool Hub::ShouldOpenEditor() const { return m_open_editor; }
fs::path Hub::GetSelectedProjectPath() const { return m_selected_path; }
void Hub::ResetOpenFlag() { m_open_editor = false; }
