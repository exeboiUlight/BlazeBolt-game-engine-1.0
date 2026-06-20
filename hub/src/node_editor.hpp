#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "imgui.h"
#include "imgui_node_editor.h"
#include "node_types.hpp"

struct EditorTab;

namespace ed = ax::NodeEditor;

struct NodePin {
    int id;
    std::string name;
    PinType type;
    bool is_input;
    ImVec2 position;
};

struct NodeConnection {
    int from_node_id;
    int from_pin_index;
    int to_node_id;
    int to_pin_index;
};

struct NodeInstance {
    int id;
    int type_id;
    ImVec2 position;
    ImVec2 size;

    std::unordered_map<int, std::string> pin_values;

    std::vector<NodePin> input_pins;
    std::vector<NodePin> output_pins;
};

class NodeEditor {
public:
    NodeEditor();
    ~NodeEditor();

    void Render(EditorTab& tab);

    bool SaveToFile(const std::string& path);
    bool LoadFromFile(const std::string& path);

    std::string GenerateLua();

    bool IsModified() const { return m_modified; }
    void ClearModified() { m_modified = false; }

    void Clear();
    void RenderPalettePanel();

private:
    ed::EditorContext* m_context = nullptr;

    std::vector<NodeInstance> m_nodes;
    std::vector<NodeConnection> m_connections;

    std::vector<int> m_selected_nodes;
    bool m_modified = false;
    std::string m_current_file_path;

    char m_palette_search[256] = "";
    char m_palette_search_panel[256] = "";

    std::vector<NodeInstance> m_clipboard_nodes;
    std::vector<NodeConnection> m_clipboard_connections;

    static int s_next_node_id;
    static int GenerateNodeId();

    void RenderPalette();

    void ComputeNodeSize(NodeInstance& node);
    void ComputePinPositions(NodeInstance& node);

    void AddNode(int type_id, ImVec2 position);
    void DeleteSelectedNodes();
    void DeleteNode(int node_id);

    void AddConnection(int from_node, int from_pin, int to_node, int to_pin);
    void RemoveAllConnectionsForNode(int node_id);

    void ClearSelection();
    void SelectNode(int node_id, bool add_to_selection = false);
    bool IsNodeSelected(int node_id);

    void CopySelectedNodes();
    void CutSelectedNodes();
    void PasteNodes();

    NodeInstance* FindNode(int node_id);
    const NodeTypeDef* GetTypeDef(const NodeInstance& node);

    std::string GenerateNodeCode(NodeInstance& node, std::unordered_map<int, std::string>& output_vars, int& var_counter);
    std::string GetNodeOutputVar(int node_id, int pin_index, std::unordered_map<int, std::string>& output_vars, int& var_counter);
    std::string GetNodeInputValue(NodeInstance& node, int pin_index, std::unordered_map<int, std::string>& output_vars, int& var_counter);
    bool HasInputConnection(int node_id, int pin_index);
    int FindConnectedNode(int node_id, int pin_index);

    void RenderInlineEditor(NodeInstance& node);

    ImU32 GetPinColor(PinType type);
    ImU32 GetCategoryHeaderColor(NodeCategory cat);
};
