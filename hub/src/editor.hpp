#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include <cstring>
#include <memory>
#include "imgui.h"
#include "imgui_internal.h"
#include "TextEditor.h"
#include <cstdint>
typedef unsigned int GLuint;

namespace fs = std::filesystem;

enum class EditorTabType { Code, Image, None };

struct EditorTab {
    EditorTabType type = EditorTabType::None;
    std::string file_path;
    std::string title;
    bool modified = false;

    std::unique_ptr<TextEditor> code_editor;

    unsigned char* image_data = nullptr;
    int img_w = 0, img_h = 0, img_channels = 0;
    ImTextureID image_texture = (ImTextureID)0;
    float zoom = 1.0f;
    float pan_x = 0.0f, pan_y = 0.0f;
};

struct ClipboardEntry {
    std::string path;
    bool cut = false;
};

class Editor {
public:
    Editor(fs::path engine_root);
    ~Editor();

    void OpenProject(fs::path project_path);
    void Render();
    bool ShouldReturnToHub() const;
    void ResetReturnFlag();

private:
    fs::path m_engine_root;
    fs::path m_project_path;
    fs::path m_fm_root;
    bool m_return_to_hub = false;

    std::vector<EditorTab> m_tabs;
    int m_active_tab = -1;

    fs::path m_fm_current_dir;
    std::vector<fs::directory_entry> m_fm_entries;
    ClipboardEntry m_clipboard;
    bool m_fm_show_hidden = false;
    char m_fm_rename_buf[512] = "";
    int m_fm_renaming_idx = -1;

    int m_current_theme = 0;

    void RenderMenuBar();
    void RenderToolBar();
    void RenderSidePanel();
    void RenderTabBar();
    void RenderTabContent();

    void RenderCodeEditor(EditorTab& tab);
    void RenderImageEditor(EditorTab& tab);

    void RefreshFM();
    void FMNavigateUp();
    void FMCreateFile(const std::string& name);
    void FMCreateFolder(const std::string& name);
    void FMRename(int idx, const std::string& new_name);
    void FMCopy(const std::string& src);
    void FMCut(const std::string& src);
    void FMPaste();
    void FMOpenFile(const std::string& path);

    void OpenFileInTab(const std::string& path);
    void CloseTab(int idx);
    void SaveTab(EditorTab& tab);
    void RunGame();

    void ApplyTheme(int idx);
    void ApplyThemeDark();
    void ApplyThemeLight();
    void ApplyThemeClassic();
    void ApplyThemeCyberpunk();
    void ApplyThemeDracula();
    void LoadTheme();
    void SaveTheme();

    bool IsImageFile(const std::string& ext);
    bool IsCodeFile(const std::string& ext);
};
