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
#include "crude_json.h"
#include <cstdint>
typedef unsigned int GLuint;

class NodeEditor;

namespace fs = std::filesystem;

enum class EditorTabType { Code, Image, NodeGraph, Scene, None };

struct EditorTab {
    EditorTabType type = EditorTabType::None;
    std::string file_path;
    std::string title;
    bool modified = false;

    std::unique_ptr<TextEditor> code_editor;
    std::unique_ptr<NodeEditor> node_editor;

    // Scene editor data
    crude_json::value scene_json;
    int scene_selected_object = -1;
    int scene_gizmo_mode = 0; // 0=MOVE, 1=ROTATE, 2=SCALE
    int scene_drag_handle = -1; // -1=none, 0-3=corners, 4-7=edges, 8=rotation, 9=center, 10=Xarrow, 11=Yarrow
    double scene_drag_wl, scene_drag_wb, scene_drag_wr, scene_drag_wt; // initial bbox when drag started
    double scene_drag_vx, scene_drag_vy; // initial position when drag started
    double scene_drag_vsx, scene_drag_vsy; // initial size when drag started
    char scene_new_name[256] = "";
    char scene_new_type[64] = "sprite";

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
    void RenderNodeEditor(EditorTab& tab);
    void RenderSceneEditor(EditorTab& tab);

    void OpenNodeGraph(const std::string& path);
    void OpenSceneEditor(const std::string& path);
    void CreateNewNodeGraph(const std::string& path);
    void ExportNodeGraphToLua(EditorTab& tab);

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
    bool IsNodeGraphFile(const std::string& ext);
};
