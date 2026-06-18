#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "imgui.h"
#include "imgui_internal.h"
#include "node_types.hpp"

struct EditorTab;

inline int GenerateNodeId() {
    static int counter = 0;
    return counter++;
}

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

struct NodeEditorState {
    std::vector<NodeInstance> nodes;
    std::vector<NodeConnection> connections;

    ImVec2 offset = ImVec2(0, 0);
    float zoom = 1.0f;

    int selected_node_id = -1;
    int hovered_node_id = -1;
    int dragging_node_id = -1;
    ImVec2 drag_offset = ImVec2(0, 0);

    bool is_connecting = false;
    int connecting_from_node = -1;
    int connecting_from_pin = -1;
    bool connecting_from_output = true;
    ImVec2 connecting_mouse_pos = ImVec2(0, 0);

    bool is_panning = false;
    ImVec2 pan_start = ImVec2(0, 0);
    bool is_selecting = false;
    ImVec2 selection_start = ImVec2(0, 0);
    ImVec2 selection_end = ImVec2(0, 0);
    std::vector<int> selected_nodes;

    char palette_search[256] = "";
    int palette_selected_category = -1;

    std::vector<NodeInstance> clipboard_nodes;
    std::vector<NodeConnection> clipboard_connections;

    std::string current_file_path;
    bool modified = false;

    ImVec2 canvas_pos_cache = ImVec2(0, 0);
};

class NodeEditor {
public:
    NodeEditor();
    ~NodeEditor();

    void Render(EditorTab& tab);

    bool SaveToFile(const std::string& path);
    bool LoadFromFile(const std::string& path);

    std::string GenerateLua();

    NodeEditorState& GetState() { return m_state; }
    bool IsModified() const { return m_state.modified; }
    void ClearModified() { m_state.modified = false; }

    void Clear();

private:
    NodeEditorState m_state;

    void RenderCanvas();
    void RenderGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size);
    void RenderNodes(ImDrawList* draw_list);
    void RenderConnections(ImDrawList* draw_list);
    void RenderConnectionBeingCreated(ImDrawList* draw_list);
    void RenderSelectionRect(ImDrawList* draw_list);

    void RenderNode(ImDrawList* draw_list, NodeInstance& node);
    void ComputeNodeSize(NodeInstance& node);
    void ComputePinPositions(NodeInstance& node);
    ImVec2 GetPinPosition(const NodeInstance& node, int pin_index, bool is_input);

    void HandleCanvasInteraction();
    void HandleNodeInteraction(NodeInstance& node);
    void HandlePinInteraction(NodeInstance& node, int pin_index, bool is_input);

    void RenderPalette();
    void AddNode(int type_id, ImVec2 position);
    void DeleteSelectedNodes();
    void DeleteNode(int node_id);

    void AddConnection(int from_node, int from_pin, int to_node, int to_pin);
    void RemoveConnectionsForPin(int node_id, int pin_index, bool is_input);
    void RemoveAllConnectionsForNode(int node_id);

    void ClearSelection();
    void SelectNode(int node_id, bool add_to_selection = false);
    bool IsNodeSelected(int node_id);

    void CopySelectedNodes();
    void CutSelectedNodes();
    void PasteNodes();

    ImVec2 ScreenToCanvas(ImVec2 screen_pos);
    ImVec2 CanvasToScreen(ImVec2 canvas_pos);

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
