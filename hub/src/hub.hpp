#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "imgui.h"

namespace fs = std::filesystem;

enum class ProjectType { Code, NodeGraph };

class Hub {
public:
    Hub(fs::path engine_root);
    ~Hub();

    void Render();
    bool ShouldOpenEditor() const;
    fs::path GetSelectedProjectPath() const;
    void ResetOpenFlag();

private:
    struct Project {
        std::string name;
        std::string path;
        ImTextureID icon_texture = (ImTextureID)0;
        int icon_w = 0, icon_h = 0;
    };

    fs::path m_engine_root;
    fs::path m_projects_file;
    std::vector<Project> m_projects;
    bool m_open_editor = false;
    fs::path m_selected_path;

    bool m_show_create_popup = false;
    char m_new_name[256] = "";
    char m_new_path[1024] = "";
    char m_error_msg[512] = "";
    bool m_show_error = false;
    ProjectType m_new_project_type = ProjectType::Code;

    int m_current_theme = 2;

    void LoadProjects();
    void SaveProjects();
    void CreateProject(const std::string& name, const std::string& path, ProjectType type);
    void LoadIcon(Project& proj);
    void UnloadIcon(Project& proj);
    bool ExtractZip(const fs::path& zip, const fs::path& dest);
    void ApplyTheme(int idx);
    void LoadTheme();
    void SaveTheme();
};
