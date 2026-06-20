#include "node_editor.hpp"
#include "editor.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <queue>

namespace ed = ax::NodeEditor;

static inline ed::NodeId MakeNodeId(int id) {
    return ed::NodeId(reinterpret_cast<void*>(static_cast<intptr_t>(id)));
}

static inline ed::PinId MakePinId(int id) {
    return ed::PinId(reinterpret_cast<void*>(static_cast<intptr_t>(id)));
}

static inline ed::LinkId MakeLinkId(int id) {
    return ed::LinkId(reinterpret_cast<void*>(static_cast<intptr_t>(id)));
}

static inline int IdFromPointer(void* ptr) {
    return static_cast<int>(reinterpret_cast<intptr_t>(ptr));
}

static const float NODE_HEADER_HEIGHT = 26.0f;
static const float NODE_PIN_SPACING   = 22.0f;
static const float NODE_PADDING       = 8.0f;
static const float NODE_MIN_WIDTH     = 150.0f;
static const float NODE_INLINE_HEIGHT = 24.0f;

int NodeEditor::s_next_node_id = 1;

int NodeEditor::GenerateNodeId() {
    return s_next_node_id++;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

NodeEditor::NodeEditor() {
    ed::Config config;
    config.SettingsFile = nullptr;
    m_context = ed::CreateEditor(&config);
}

NodeEditor::~NodeEditor() {
    if (m_context) {
        ed::DestroyEditor(m_context);
        m_context = nullptr;
    }
}

void NodeEditor::Clear() {
    m_nodes.clear();
    m_connections.clear();
    m_selected_nodes.clear();
    m_modified = false;
    m_current_file_path.clear();
    if (m_context) {
        ed::DestroyEditor(m_context);
        ed::Config config;
        config.SettingsFile = nullptr;
        m_context = ed::CreateEditor(&config);
    }
}

// ============================================================================
// Helpers
// ============================================================================

NodeInstance* NodeEditor::FindNode(int node_id) {
    for (auto& n : m_nodes)
        if (n.id == node_id) return &n;
    return nullptr;
}

const NodeTypeDef* NodeEditor::GetTypeDef(const NodeInstance& node) {
    return FindNodeTypeDef(node.type_id);
}

int NodeEditor::FindConnectedNode(int node_id, int pin_index) {
    for (auto& c : m_connections)
        if (c.to_node_id == node_id && c.to_pin_index == pin_index)
            return c.from_node_id;
    return -1;
}

bool NodeEditor::HasInputConnection(int node_id, int pin_index) {
    return FindConnectedNode(node_id, pin_index) != -1;
}

ImU32 NodeEditor::GetPinColor(PinType type) {
    switch (type) {
        case PinType::Flow:    return IM_COL32(255, 255, 255, 255);
        case PinType::Number:  return IM_COL32(80, 200, 120, 255);
        case PinType::Integer: return IM_COL32(80, 180, 120, 255);
        case PinType::String:  return IM_COL32(220, 200, 80, 255);
        case PinType::Bool:    return IM_COL32(220, 60, 60, 255);
        case PinType::Entity:  return IM_COL32(80, 200, 220, 255);
        case PinType::Table:   return IM_COL32(180, 100, 220, 255);
        case PinType::Vector2: return IM_COL32(80, 180, 240, 255);
        case PinType::Vector3: return IM_COL32(80, 160, 240, 255);
        case PinType::Any:     return IM_COL32(160, 160, 160, 255);
        default:               return IM_COL32(128, 128, 128, 255);
    }
}

ImU32 NodeEditor::GetCategoryHeaderColor(NodeCategory cat) {
    ImColor c = GetCategoryColor(cat);
    return IM_COL32(c.Value.x * 255, c.Value.y * 255, c.Value.z * 255, 255);
}

void NodeEditor::ComputeNodeSize(NodeInstance& node) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def) return;

    float max_pins = (float)std::max(def->inputs.size(), def->outputs.size());
    float height = NODE_HEADER_HEIGHT + NODE_PADDING * 2 + max_pins * NODE_PIN_SPACING;
    if (def->has_inline)
        height += NODE_INLINE_HEIGHT + NODE_PADDING;

    ImGui::PushFont(nullptr);
    float name_width = ImGui::CalcTextSize(def->name).x;
    ImGui::PopFont();

    float width = std::max(NODE_MIN_WIDTH, name_width + NODE_PADDING * 2 + 20.0f);
    node.size = ImVec2(width, height);
}

void NodeEditor::ComputePinPositions(NodeInstance& node) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def) return;

    node.input_pins.clear();
    node.output_pins.clear();

    for (size_t i = 0; i < def->inputs.size(); i++) {
        NodePin pin;
        pin.id = node.id * 100 + (int)i;
        pin.name = def->inputs[i].name;
        pin.type = def->inputs[i].type;
        pin.is_input = true;
        pin.position = ImVec2(0.0f, NODE_HEADER_HEIGHT + NODE_PADDING + i * NODE_PIN_SPACING);
        node.input_pins.push_back(pin);
    }

    for (size_t i = 0; i < def->outputs.size(); i++) {
        NodePin pin;
        pin.id = node.id * 100 + 50 + (int)i;
        pin.name = def->outputs[i].name;
        pin.type = def->outputs[i].type;
        pin.is_input = false;
        pin.position = ImVec2(node.size.x, NODE_HEADER_HEIGHT + NODE_PADDING + i * NODE_PIN_SPACING);
        node.output_pins.push_back(pin);
    }
}

// ============================================================================
// Main render
// ============================================================================

void NodeEditor::Render(EditorTab& tab) {
    if (!m_context) return;

    ImGui::BeginChild("##NodeCanvas", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

    ed::SetCurrentEditor(m_context);
    ed::Begin("NodeEditor");

    // ----------------------------------------------------------------
    //  Configure the editor style (safe to call each frame)
    // ----------------------------------------------------------------
    {
        auto& style = ed::GetStyle();
        style.NodeRounding             = 6.0f;
        style.NodeBorderWidth          = 1.0f;
        style.HoveredNodeBorderWidth   = 2.0f;
        style.SelectedNodeBorderWidth  = 2.5f;
        style.SelectedNodeBorderOffset = 0.0f;
        style.HoverNodeBorderOffset    = 0.0f;
        style.PinRounding              = 0.0f;
        style.PinBorderWidth           = 0.0f;
        style.LinkStrength             = 100.0f;
        style.SourceDirection          = ImVec2(1.0f, 0.0f);
        style.TargetDirection          = ImVec2(-1.0f, 0.0f);
        style.FlowSpeed                = 200.0f;
        style.FlowDuration             = 2.0f;
        style.FlowMarkerDistance       = 30.0f;
        style.HighlightConnectedLinks  = 1.0f;
        style.SnapLinkToPinDir         = 1.0f;

        style.Colors[ed::StyleColor_Bg]                = ImVec4(0.16f, 0.16f, 0.19f, 1.0f);
        style.Colors[ed::StyleColor_Grid]              = ImVec4(0.22f, 0.22f, 0.26f, 0.50f);
        style.Colors[ed::StyleColor_NodeBg]            = ImVec4(0.14f, 0.14f, 0.17f, 1.0f);
        style.Colors[ed::StyleColor_NodeBorder]        = ImVec4(0.30f, 0.30f, 0.35f, 0.80f);
        style.Colors[ed::StyleColor_HovNodeBorder]     = ImVec4(0.20f, 0.50f, 0.85f, 1.0f);
        style.Colors[ed::StyleColor_SelNodeBorder]     = ImVec4(1.00f, 0.68f, 0.20f, 1.0f);
        style.Colors[ed::StyleColor_NodeSelRect]       = ImVec4(0.02f, 0.50f, 1.00f, 0.25f);
        style.Colors[ed::StyleColor_NodeSelRectBorder] = ImVec4(0.02f, 0.50f, 1.00f, 0.50f);
        style.Colors[ed::StyleColor_HovLinkBorder]     = ImVec4(0.20f, 0.50f, 0.85f, 1.0f);
        style.Colors[ed::StyleColor_SelLinkBorder]     = ImVec4(1.00f, 0.68f, 0.20f, 1.0f);
        style.Colors[ed::StyleColor_Flow]              = ImVec4(1.00f, 0.50f, 0.25f, 1.0f);
        style.Colors[ed::StyleColor_FlowMarker]        = ImVec4(1.00f, 0.50f, 0.25f, 1.0f);
        style.Colors[ed::StyleColor_PinRect]           = ImVec4(0.24f, 0.70f, 1.00f, 0.40f);
        style.Colors[ed::StyleColor_PinRectBorder]     = ImVec4(0.24f, 0.70f, 1.00f, 0.60f);
        style.Colors[ed::StyleColor_GroupBg]           = ImVec4(0.0f, 0.0f, 0.0f, 0.16f);
        style.Colors[ed::StyleColor_GroupBorder]       = ImVec4(1.0f, 1.0f, 1.0f, 0.12f);
    }

    // ----------------------------------------------------------------
    //  Draw all nodes
    // ----------------------------------------------------------------
    for (auto& node : m_nodes) {
        ComputeNodeSize(node);
        ComputePinPositions(node);

        const NodeTypeDef* def = GetTypeDef(node);
        if (!def) continue;

        ed::BeginNode(MakeNodeId(node.id));
        ImGui::PushID(node.id);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 nodeOrigin = ImGui::GetCursorScreenPos();

        // -- Header colored bar --
        ImU32 rawCat = GetCategoryHeaderColor(def->category);
        ImVec4 hdrF = ImGui::ColorConvertU32ToFloat4(rawCat);
        hdrF.x *= 0.55f; hdrF.y *= 0.55f; hdrF.z *= 0.55f; hdrF.w = 1.0f;
        ImU32 headerBg = ImGui::ColorConvertFloat4ToU32(hdrF);

        ImVec2 hMin = nodeOrigin;
        ImVec2 hMax = ImVec2(nodeOrigin.x + node.size.x, nodeOrigin.y + NODE_HEADER_HEIGHT);
        dl->AddRectFilled(hMin, hMax, headerBg, 6.0f, ImDrawFlags_RoundCornersTop);

        // -- Title (centered) --
        ImVec2 titlePos = ImGui::GetCursorScreenPos();
        float tw = ImGui::CalcTextSize(def->name).x;
        ImGui::SetCursorScreenPos(ImVec2(titlePos.x + (node.size.x - tw) * 0.5f, titlePos.y + 3.0f));
        ImGui::TextUnformatted(def->name);
        ImGui::SetCursorScreenPos(ImVec2(nodeOrigin.x, hMax.y));
        ImGui::Dummy(ImVec2(0, NODE_PADDING));

        // -- Input pins --
        for (size_t i = 0; i < node.input_pins.size(); i++) {
            auto& pin = node.input_pins[i];
            ed::BeginPin(MakePinId(pin.id), ed::PinKind::Input);

            ImVec2 cp = ImGui::GetCursorScreenPos();
            float cy = cp.y + ImGui::GetFrameHeight() * 0.5f;
            ImU32 col = GetPinColor(pin.type);

            dl->AddCircleFilled(ImVec2(cp.x + 5.0f, cy), 5.0f, col);
            dl->AddCircle(ImVec2(cp.x + 5.0f, cy), 5.0f, IM_COL32(0, 0, 0, 100));

            ImGui::Dummy(ImVec2(14.0f, 0));
            ImGui::SameLine();

            ImVec4 tc = ImGui::ColorConvertU32ToFloat4(col);
            tc.w = 1.0f;
            ImGui::PushStyleColor(ImGuiCol_Text, tc);
            ImGui::TextUnformatted(pin.name.c_str());
            ImGui::PopStyleColor();

            ed::EndPin();
        }

        // -- Inline editor --
        if (def->has_inline) {
            ImGui::Dummy(ImVec2(14.0f, 0));
            ImGui::SameLine();
            auto& value_ref = node.pin_values[-1];

            switch (def->inline_type) {
                case PinType::String: {
                    char buf[512];
                    strncpy(buf, value_ref.c_str(), sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = '\0';
                    ImGui::PushItemWidth(110.0f);
                    if (ImGui::InputText("##val", buf, sizeof(buf))) {
                        value_ref = buf;
                        m_modified = true;
                    }
                    ImGui::PopItemWidth();
                    break;
                }
                case PinType::Number: {
                    float fv = std::strtof(value_ref.c_str(), nullptr);
                    ImGui::PushItemWidth(110.0f);
                    if (ImGui::InputFloat("##val", &fv, 0.0f, 0.0f, "%.3f")) {
                        value_ref = std::to_string(fv);
                        m_modified = true;
                    }
                    ImGui::PopItemWidth();
                    break;
                }
                case PinType::Integer: {
                    int iv = std::atoi(value_ref.c_str());
                    ImGui::PushItemWidth(110.0f);
                    if (ImGui::InputInt("##val", &iv, 0, 0)) {
                        value_ref = std::to_string(iv);
                        m_modified = true;
                    }
                    ImGui::PopItemWidth();
                    break;
                }
                case PinType::Bool: {
                    bool bv = (value_ref == "true" || value_ref == "1");
                    if (ImGui::Checkbox("##val", &bv)) {
                        value_ref = bv ? "true" : "false";
                        m_modified = true;
                    }
                    break;
                }
                default: {
                    char buf[512];
                    strncpy(buf, value_ref.c_str(), sizeof(buf) - 1);
                    buf[sizeof(buf) - 1] = '\0';
                    ImGui::PushItemWidth(110.0f);
                    if (ImGui::InputText("##val", buf, sizeof(buf))) {
                        value_ref = buf;
                        m_modified = true;
                    }
                    ImGui::PopItemWidth();
                    break;
                }
            }
        }

        ImGui::PopID();
        ed::EndNode();

        node.position = ed::GetNodePosition(MakeNodeId(node.id));
    }

    // ----------------------------------------------------------------
    //  Draw all links
    // ----------------------------------------------------------------
    for (auto& conn : m_connections) {
        int start_pin_id = conn.from_node_id * 100 + 50 + conn.from_pin_index;
        int end_pin_id   = conn.to_node_id   * 100 + conn.to_pin_index;

        NodeInstance* from_node = FindNode(conn.from_node_id);
        PinType pin_type = PinType::Any;
        if (from_node && conn.from_pin_index >= 0 && conn.from_pin_index < (int)from_node->output_pins.size())
            pin_type = from_node->output_pins[conn.from_pin_index].type;

        int link_id = conn.from_node_id * 10000 + conn.from_pin_index * 100 + conn.to_node_id * 10 + conn.to_pin_index;

        ImVec4 col = ImGui::ColorConvertU32ToFloat4(GetPinColor(pin_type));
        col.w = 0.85f;
        ed::Link(MakeLinkId(link_id), MakePinId(start_pin_id), MakePinId(end_pin_id), col, 2.5f);
    }

    // ----------------------------------------------------------------
    //  Create new link
    // ----------------------------------------------------------------
    if (ed::BeginCreate(ImVec4(0.8f, 0.8f, 0.85f, 0.8f), 2.0f)) {
        ed::PinId startPinId, endPinId;
        if (ed::QueryNewLink(&startPinId, &endPinId)) {
            if (startPinId && endPinId) {
                int start_raw = IdFromPointer(startPinId.AsPointer());
                int end_raw   = IdFromPointer(endPinId.AsPointer());

                int start_node_id = start_raw / 100;
                int start_pin_idx = start_raw % 100;
                int end_node_id   = end_raw   / 100;
                int end_pin_idx   = end_raw   % 100;

                bool start_is_output = (start_pin_idx >= 50);
                bool end_is_input    = (end_pin_idx   < 50);

                int from_node = 0, from_pin = 0, to_node = 0, to_pin = 0;
                bool valid_dir = false;

                if (start_is_output && end_is_input) {
                    from_node = start_node_id;
                    from_pin  = start_pin_idx - 50;
                    to_node   = end_node_id;
                    to_pin    = end_pin_idx;
                    valid_dir = true;
                } else if (!start_is_output && !end_is_input) {
                    from_node = end_node_id;
                    from_pin  = end_pin_idx - 50;
                    to_node   = start_node_id;
                    to_pin    = start_pin_idx;
                    valid_dir = true;
                }

                if (!valid_dir) {
                    ed::RejectNewItem(ImVec4(1.0f, 0.3f, 0.3f, 0.8f), 2.0f);
                } else if (from_node == to_node) {
                    ed::RejectNewItem(ImVec4(1.0f, 0.3f, 0.3f, 0.8f), 2.0f);
                } else {
                    NodeInstance* src_node = FindNode(from_node);
                    NodeInstance* dst_node = FindNode(to_node);
                    bool compatible = true;

                    if (src_node && from_pin >= 0 && from_pin < (int)src_node->output_pins.size()) {
                        PinType src_type = src_node->output_pins[from_pin].type;
                        if (dst_node && to_pin >= 0 && to_pin < (int)dst_node->input_pins.size()) {
                            PinType dst_type = dst_node->input_pins[to_pin].type;
                            if (src_type != PinType::Any && dst_type != PinType::Any && src_type != dst_type)
                                compatible = false;
                        }
                    }

                    if (compatible) {
                        if (ed::AcceptNewItem(ImVec4(0.4f, 0.8f, 0.4f, 0.8f), 2.0f))
                            AddConnection(from_node, from_pin, to_node, to_pin);
                    } else {
                        ed::RejectNewItem(ImVec4(1.0f, 0.3f, 0.3f, 0.8f), 2.0f);
                    }
                }
            }
        }
    }
    ed::EndCreate();

    // ----------------------------------------------------------------
    //  Delete links and nodes
    // ----------------------------------------------------------------
    if (ed::BeginDelete()) {
        ed::LinkId linkId;
        while (ed::QueryDeletedLink(&linkId)) {
            int link_key = IdFromPointer(linkId.AsPointer());
            for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
                int conn_key = it->from_node_id * 10000 + it->from_pin_index * 100 + it->to_node_id * 10 + it->to_pin_index;
                if (conn_key == link_key) {
                    ed::AcceptDeletedItem();
                    m_connections.erase(it);
                    m_modified = true;
                    break;
                }
            }
        }

        ed::NodeId nodeId;
        while (ed::QueryDeletedNode(&nodeId)) {
            int node_id = IdFromPointer(nodeId.AsPointer());
            DeleteNode(node_id);
            ed::AcceptDeletedItem();
        }
    }
    ed::EndDelete();

    // ----------------------------------------------------------------
    //  Context menus
    // ----------------------------------------------------------------
    if (ed::ShowBackgroundContextMenu()) {
        ImGui::OpenPopup("NodePalette");
        m_palette_search[0] = '\0';
    }

    ed::NodeId contextNodeId;
    if (ed::ShowNodeContextMenu(&contextNodeId)) {
        int node_id = IdFromPointer(contextNodeId.AsPointer());
        ClearSelection();
        SelectNode(node_id);
    }

    ed::End();

    RenderPalette();

    // ----------------------------------------------------------------
    //  Drag-drop from palette panel (must be inside the child window)
    // ----------------------------------------------------------------
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_TYPE")) {
            int type_id = *(const int*)payload->Data;
            ImVec2 drop_pos = ImGui::GetMousePos();
            ImVec2 canvas_pos = ed::ScreenToCanvas(drop_pos);
            AddNode(type_id, canvas_pos);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();

    ed::SetCurrentEditor(nullptr);

    // ----------------------------------------------------------------
    //  Keyboard shortcuts
    // ----------------------------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (!m_selected_nodes.empty())
            DeleteSelectedNodes();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) CopySelectedNodes();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X)) CutSelectedNodes();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) PasteNodes();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (!m_current_file_path.empty())
            SaveToFile(m_current_file_path);
    }
}

// ============================================================================
// Palette (context menu popup)
// ============================================================================

void NodeEditor::RenderPalette() {
    if (!ImGui::IsPopupOpen("NodePalette")) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(28, 28, 34, 248));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(60, 60, 75, 180));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(22, 22, 28, 200));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(55, 55, 65, 255));

    if (ImGui::BeginPopup("NodePalette")) {
        ImGui::InputTextWithHint("##search", "Search...", m_palette_search, sizeof(m_palette_search));
        ImGui::Separator();

        const auto& defs = GetNodeTypeDefs();
        std::string search_lower = m_palette_search;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        for (auto cat : {
            NodeCategory::Flow, NodeCategory::Variables, NodeCategory::Math, NodeCategory::Comparison,
            NodeCategory::Sprites, NodeCategory::Animation, NodeCategory::Text, NodeCategory::Camera,
            NodeCategory::Physics, NodeCategory::Audio, NodeCategory::Window, NodeCategory::Scenes,
            NodeCategory::Noise, NodeCategory::MathTypes, NodeCategory::Constants
        }) {
            bool any_match = false;
            for (auto& d : defs) {
                if (d.category != cat) continue;
                std::string nl = d.name;
                std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
                if (search_lower.empty() || nl.find(search_lower) != std::string::npos) {
                    any_match = true;
                    break;
                }
            }
            if (!any_match) continue;

            if (ImGui::TreeNodeEx(GetCategoryName(cat), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& d : defs) {
                    if (d.category != cat) continue;
                    std::string nl = d.name;
                    std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
                    if (!search_lower.empty() && nl.find(search_lower) == std::string::npos) continue;

                    ImU32 cat_col = GetCategoryHeaderColor(cat);
                    ImVec2 tp = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(tp.x, tp.y),
                        ImVec2(tp.x + 3.0f, tp.y + ImGui::GetFrameHeight()),
                        cat_col);

                    if (ImGui::Selectable(d.name, false, ImGuiSelectableFlags_None, ImVec2(200, 18))) {
                        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                        AddNode(d.id, mouse_pos);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);
}

// ============================================================================
// Palette Panel (left side drag-drop list)
// ============================================================================

void NodeEditor::RenderPalettePanel() {
    const auto& defs = GetNodeTypeDefs();

    ImGui::InputTextWithHint("##pp_search", "Search...", m_palette_search_panel, sizeof(m_palette_search_panel));

    std::string search_lower = m_palette_search_panel;
    std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

    ImGui::BeginChild("##PalettePanelList", ImVec2(0, 0));

    for (auto cat : {
        NodeCategory::Flow, NodeCategory::Variables, NodeCategory::Math, NodeCategory::Comparison,
        NodeCategory::Sprites, NodeCategory::Animation, NodeCategory::Text, NodeCategory::Camera,
        NodeCategory::Physics, NodeCategory::Audio, NodeCategory::Window, NodeCategory::Scenes,
        NodeCategory::Noise, NodeCategory::MathTypes, NodeCategory::Constants
    }) {
        bool any_match = false;
        for (auto& d : defs) {
            if (d.category != cat) continue;
            std::string nl = d.name;
            std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
            if (search_lower.empty() || nl.find(search_lower) != std::string::npos) {
                any_match = true;
                break;
            }
        }
        if (!any_match) continue;

        if (ImGui::TreeNodeEx(GetCategoryName(cat), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& d : defs) {
                if (d.category != cat) continue;
                std::string nl = d.name;
                std::transform(nl.begin(), nl.end(), nl.begin(), ::tolower);
                if (!search_lower.empty() && nl.find(search_lower) == std::string::npos) continue;

                ImGui::PushID(d.id);

                ImU32 cat_col = GetCategoryHeaderColor(cat);
                ImVec2 p = ImGui::GetCursorScreenPos();
                float h = ImGui::GetFrameHeight();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(p.x, p.y), ImVec2(p.x + 3, p.y + h), cat_col);

                ImGui::Selectable(d.name, false, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x, 18));

                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("NODE_TYPE", &d.id, sizeof(int));
                    ImGui::Text("%s", d.name);
                    ImGui::EndDragDropSource();
                }

                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
}

// ============================================================================
// Node management
// ============================================================================

void NodeEditor::AddNode(int type_id, ImVec2 position) {
    const NodeTypeDef* def = FindNodeTypeDef(type_id);
    if (!def) return;

    NodeInstance node;
    node.id = GenerateNodeId();
    node.type_id = type_id;
    node.position = position;
    node.size = ImVec2(0, 0);

    ComputeNodeSize(node);
    node.position.x -= node.size.x * 0.5f;
    node.position.y -= NODE_HEADER_HEIGHT * 0.5f;

    for (size_t i = 0; i < def->inputs.size(); i++)
        node.pin_values[(int)i] = "";

    if (def->has_inline) {
        switch (def->inline_type) {
            case PinType::Number:  node.pin_values[-1] = "0.0";   break;
            case PinType::Integer: node.pin_values[-1] = "0";     break;
            case PinType::Bool:    node.pin_values[-1] = "false"; break;
            default:               node.pin_values[-1] = "";      break;
        }
    }

    m_nodes.push_back(node);
    m_modified = true;
}

void NodeEditor::DeleteSelectedNodes() {
    for (int id : m_selected_nodes)
        DeleteNode(id);
    m_selected_nodes.clear();
}

void NodeEditor::DeleteNode(int node_id) {
    RemoveAllConnectionsForNode(node_id);
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it->id == node_id) {
            m_nodes.erase(it);
            break;
        }
    }
    m_modified = true;
}

// ============================================================================
// Connection management
// ============================================================================

void NodeEditor::AddConnection(int from_node, int from_pin, int to_node, int to_pin) {
    if (from_node == to_node) return;

    for (auto it = m_connections.begin(); it != m_connections.end(); ) {
        if (it->to_node_id == to_node && it->to_pin_index == to_pin) {
            it = m_connections.erase(it);
            continue;
        }
        ++it;
    }

    NodeConnection conn;
    conn.from_node_id  = from_node;
    conn.from_pin_index = from_pin;
    conn.to_node_id    = to_node;
    conn.to_pin_index  = to_pin;
    m_connections.push_back(conn);
    m_modified = true;
}

void NodeEditor::RemoveAllConnectionsForNode(int node_id) {
    for (auto it = m_connections.begin(); it != m_connections.end(); ) {
        if (it->from_node_id == node_id || it->to_node_id == node_id)
            it = m_connections.erase(it);
        else
            ++it;
    }
}

// ============================================================================
// Selection
// ============================================================================

void NodeEditor::ClearSelection() {
    m_selected_nodes.clear();
    if (m_context) ed::ClearSelection();
}

void NodeEditor::SelectNode(int node_id, bool add_to_selection) {
    if (!add_to_selection) m_selected_nodes.clear();
    for (int id : m_selected_nodes)
        if (id == node_id) return;
    m_selected_nodes.push_back(node_id);
    if (m_context)
        ed::SelectNode(MakeNodeId(node_id), add_to_selection);
}

bool NodeEditor::IsNodeSelected(int node_id) {
    for (int id : m_selected_nodes)
        if (id == node_id) return true;
    return false;
}

// ============================================================================
// Copy / Cut / Paste
// ============================================================================

void NodeEditor::CopySelectedNodes() {
    m_clipboard_nodes.clear();
    m_clipboard_connections.clear();

    for (int id : m_selected_nodes) {
        NodeInstance* node = FindNode(id);
        if (node) m_clipboard_nodes.push_back(*node);
    }

    for (auto& conn : m_connections) {
        bool from_sel = false, to_sel = false;
        for (int id : m_selected_nodes) {
            if (conn.from_node_id == id) from_sel = true;
            if (conn.to_node_id == id)   to_sel   = true;
        }
        if (from_sel && to_sel) m_clipboard_connections.push_back(conn);
    }
}

void NodeEditor::CutSelectedNodes() {
    CopySelectedNodes();
    DeleteSelectedNodes();
}

void NodeEditor::PasteNodes() {
    if (m_clipboard_nodes.empty()) return;

    std::unordered_map<int, int> id_map;
    ImVec2 paste_center = ImVec2(0, 0);
    for (auto& node : m_clipboard_nodes) {
        paste_center.x += node.position.x;
        paste_center.y += node.position.y;
    }
    paste_center.x /= m_clipboard_nodes.size();
    paste_center.y /= m_clipboard_nodes.size();

    ImVec2 mouse_screen = ImGui::GetIO().MousePos;
    ImVec2 mouse_canvas = mouse_screen;
    if (m_context)
        mouse_canvas = ed::ScreenToCanvas(mouse_screen);

    ClearSelection();

    for (auto& node : m_clipboard_nodes) {
        int old_id = node.id;
        int new_id = GenerateNodeId();
        id_map[old_id] = new_id;

        NodeInstance new_node = node;
        new_node.id = new_id;
        new_node.position.x = mouse_canvas.x + (node.position.x - paste_center.x);
        new_node.position.y = mouse_canvas.y + (node.position.y - paste_center.y);

        m_nodes.push_back(new_node);
        SelectNode(new_id, true);
    }

    for (auto& conn : m_clipboard_connections) {
        NodeConnection new_conn = conn;
        auto it_from = id_map.find(conn.from_node_id);
        auto it_to   = id_map.find(conn.to_node_id);
        if (it_from != id_map.end() && it_to != id_map.end()) {
            new_conn.from_node_id = it_from->second;
            new_conn.to_node_id   = it_to->second;
            m_connections.push_back(new_conn);
        }
    }

    m_modified = true;
}

// ============================================================================
// Save / Load
// ============================================================================

bool NodeEditor::SaveToFile(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "[NODEMAP]\nVERSION=1\n";

    f << "[NODES]\n";
    for (auto& node : m_nodes) {
        f << "ID=" << node.id
          << ";TYPE=" << node.type_id
          << ";X=" << node.position.x
          << ";Y=" << node.position.y;
        for (auto& kv : node.pin_values)
            f << ";V" << kv.first << "=" << kv.second;
        f << "\n";
    }

    f << "[CONNECTIONS]\n";
    for (auto& conn : m_connections) {
        f << "FROM=" << conn.from_node_id << ":" << conn.from_pin_index
          << ";TO=" << conn.to_node_id << ":" << conn.to_pin_index << "\n";
    }

    f.close();
    m_current_file_path = path;
    m_modified = false;
    return true;
}

static std::string trim_str(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

bool NodeEditor::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    Clear();
    m_current_file_path = path;

    enum Section { None, Nodes, Connections };
    Section section = None;

    std::string line;
    while (std::getline(f, line)) {
        line = trim_str(line);
        if (line.empty()) continue;

        if (line == "[NODEMAP]")      continue;
        if (line == "[NODES]")        { section = Nodes; continue; }
        if (line == "[CONNECTIONS]")  { section = Connections; continue; }
        if (line.substr(0, 8) == "VERSION=") continue;

        if (section == Nodes) {
            NodeInstance node;
            node.size = ImVec2(0, 0);

            std::stringstream ss(line);
            std::string segment;
            while (std::getline(ss, segment, ';')) {
                size_t eq = segment.find('=');
                if (eq == std::string::npos) continue;
                std::string key = segment.substr(0, eq);
                std::string val = segment.substr(eq + 1);

                if      (key == "ID")   node.id      = std::atoi(val.c_str());
                else if (key == "TYPE") node.type_id  = std::atoi(val.c_str());
                else if (key == "X")    node.position.x = std::strtof(val.c_str(), nullptr);
                else if (key == "Y")    node.position.y = std::strtof(val.c_str(), nullptr);
                else if (key.size() > 1 && key[0] == 'V') {
                    int pin_idx = std::atoi(key.substr(1).c_str());
                    node.pin_values[pin_idx] = val;
                }
            }

            if (FindNodeTypeDef(node.type_id)) {
                ComputeNodeSize(node);
                m_nodes.push_back(node);
            }
        } else if (section == Connections) {
            NodeConnection conn;
            size_t from_colon = line.find(':', line.find("FROM=") + 5);
            size_t from_semi  = line.find(';');
            size_t to_colon   = line.find(':', line.find("TO=") + 3);

            if (from_colon != std::string::npos && from_semi != std::string::npos && to_colon != std::string::npos) {
                conn.from_node_id  = std::atoi(line.substr(line.find("FROM=") + 5, from_colon - line.find("FROM=") - 5).c_str());
                conn.from_pin_index = std::atoi(line.substr(from_colon + 1, from_semi - from_colon - 1).c_str());
                conn.to_node_id    = std::atoi(line.substr(line.find("TO=") + 3, to_colon - line.find("TO=") - 3).c_str());
                conn.to_pin_index  = std::atoi(line.substr(to_colon + 1).c_str());
                m_connections.push_back(conn);
            }
        }
    }

    f.close();
    m_modified = false;
    return true;
}

// ============================================================================
// Lua code generation
// ============================================================================

std::string NodeEditor::GetNodeOutputVar(int node_id, int pin_index,
                                          std::unordered_map<int, std::string>& output_vars, int& var_counter) {
    int key = node_id * 100 + pin_index;
    auto it = output_vars.find(key);
    if (it != output_vars.end()) return it->second;

    NodeInstance* node = FindNode(node_id);
    if (!node) return "nil";

    const NodeTypeDef* def = GetTypeDef(*node);
    if (!def) return "nil";

    std::string var_name = "v" + std::to_string(var_counter++);
    output_vars[key] = var_name;
    return var_name;
}

std::string NodeEditor::GetNodeInputValue(NodeInstance& node, int pin_index,
                                            std::unordered_map<int, std::string>& output_vars, int& var_counter) {
    int connected = FindConnectedNode(node.id, pin_index);
    if (connected != -1) {
        NodeInstance* src = FindNode(connected);
        if (src) {
            const NodeTypeDef* src_def = GetTypeDef(*src);
            if (src_def) {
                for (size_t i = 0; i < src_def->outputs.size(); i++) {
                    std::string var = GetNodeOutputVar(connected, (int)i, output_vars, var_counter);
                    int key = connected * 100 + (int)i;
                    auto it = output_vars.find(key);
                    if (it != output_vars.end()) return it->second;
                }
                return GetNodeOutputVar(connected, 0, output_vars, var_counter);
            }
        }
    }

    auto pit = node.pin_values.find(pin_index);
    if (pit != node.pin_values.end() && !pit->second.empty())
        return pit->second;

    const NodeTypeDef* def = GetTypeDef(node);
    if (def && pin_index < (int)def->inputs.size()) {
        switch (def->inputs[pin_index].type) {
            case PinType::Number:  return "0.0";
            case PinType::Integer: return "0";
            case PinType::Bool:    return "false";
            case PinType::String:  return "\"\"";
            default:               return "nil";
        }
    }

    return "nil";
}

std::string NodeEditor::GenerateNodeCode(NodeInstance& node,
                                           std::unordered_map<int, std::string>& output_vars, int& var_counter) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def) return "";

    std::string tmpl = def->lua_template;
    if (tmpl.empty()) return "";

    for (size_t i = 0; i < def->inputs.size(); i++) {
        std::string val = GetNodeInputValue(node, (int)i, output_vars, var_counter);
        std::string placeholder = "{" + std::to_string(i) + "}";
        size_t pos;
        while ((pos = tmpl.find(placeholder)) != std::string::npos)
            tmpl.replace(pos, placeholder.size(), val);
    }

    for (size_t i = 0; i < def->outputs.size(); i++) {
        std::string var_name = GetNodeOutputVar(node.id, (int)i, output_vars, var_counter);
        std::string placeholder = "${" + std::to_string(i) + "}";
        size_t pos;
        while ((pos = tmpl.find(placeholder)) != std::string::npos)
            tmpl.replace(pos, placeholder.size(), var_name);

        std::string named_placeholder = "${" + def->outputs[i].name + "}";
        while ((pos = tmpl.find(named_placeholder)) != std::string::npos)
            tmpl.replace(pos, named_placeholder.size(), var_name);
    }

    if (def->has_inline) {
        auto pit = node.pin_values.find(-1);
        std::string inline_val = (pit != node.pin_values.end()) ? pit->second : "";
        size_t pos;
        while ((pos = tmpl.find("{inline}")) != std::string::npos)
            tmpl.replace(pos, 8, inline_val);
    }

    return tmpl;
}

std::string NodeEditor::GenerateLua() {
    std::unordered_map<int, std::string> output_vars;
    int var_counter = 0;

    std::vector<int> start_nodes, update_nodes, draw_nodes, end_nodes;

    for (auto& node : m_nodes) {
        if      (node.type_id == 1000) start_nodes.push_back(node.id);
        else if (node.type_id == 1001) update_nodes.push_back(node.id);
        else if (node.type_id == 1002) draw_nodes.push_back(node.id);
        else if (node.type_id == 1003) end_nodes.push_back(node.id);
    }

    std::string lua_code;
    std::set<int> global_visited;

    auto generate_final_function = [&](const std::string&, int start_node_id, const std::string&) {
        std::string code;
        std::set<int> visited;
        std::vector<int> exec_order;

        if (start_node_id >= 0) {
            std::function<void(int)> follow_exec = [&](int node_id) {
                if (visited.count(node_id)) return;
                visited.insert(node_id);
                global_visited.insert(node_id);
                exec_order.push_back(node_id);

                NodeInstance* node = FindNode(node_id);
                if (!node) return;
                const NodeTypeDef* def = GetTypeDef(*node);
                if (!def) return;

                for (size_t out_idx = 0; out_idx < def->outputs.size(); out_idx++) {
                    if (def->outputs[out_idx].type == PinType::Flow) {
                        for (auto& conn : m_connections) {
                            if (conn.from_node_id == node_id && conn.from_pin_index == (int)out_idx)
                                follow_exec(conn.to_node_id);
                        }
                    }
                }
            };
            follow_exec(start_node_id);
        }

        for (int node_id : exec_order) {
            NodeInstance* node = FindNode(node_id);
            if (!node) continue;
            std::string node_code = GenerateNodeCode(*node, output_vars, var_counter);
            if (node_code.empty()) continue;

            std::istringstream stream(node_code);
            std::string l;
            while (std::getline(stream, l)) {
                if (l.size() >= 2 && l[0] == '>' && l[1] == '>')
                    code += "    " + l.substr(2) + "\n";
                else if (!l.empty())
                    code += "    " + l + "\n";
            }
        }
        return code;
    };

    lua_code += "-- Generated by BlazeBolt Node Editor\n\n";

    if (!start_nodes.empty()) {
        lua_code += "function Start()\n";
        lua_code += generate_final_function("Start", start_nodes[0], "");
        lua_code += "end\n\n";
    }
    if (!update_nodes.empty()) {
        lua_code += "function Update(dt)\n";
        lua_code += generate_final_function("Update", update_nodes[0], "dt");
        lua_code += "end\n\n";
    }
    if (!draw_nodes.empty()) {
        lua_code += "function Draw()\n";
        lua_code += generate_final_function("Draw", draw_nodes[0], "");
        lua_code += "end\n\n";
    }
    if (!end_nodes.empty()) {
        lua_code += "function End()\n";
        lua_code += generate_final_function("End", end_nodes[0], "");
        lua_code += "end\n\n";
    }

    for (auto& node : m_nodes) {
        if (global_visited.count(node.id)) continue;
        const NodeTypeDef* def = GetTypeDef(node);
        if (!def) continue;

        if (def->id == 1011) {
            std::string func_name = GetNodeInputValue(node, 0, output_vars, var_counter);
            lua_code += "function " + func_name + "()\n";

            for (auto& conn : m_connections) {
                if (conn.from_node_id == node.id && conn.from_pin_index == 1) {
                    std::function<void(int)> gen_body = [&](int nid) {
                        NodeInstance* n = FindNode(nid);
                        if (!n) return;
                        global_visited.insert(nid);

                        std::string c = GenerateNodeCode(*n, output_vars, var_counter);
                        std::istringstream stream(c);
                        std::string l;
                        while (std::getline(stream, l)) {
                            if (l.size() >= 2 && l[0] == '>' && l[1] == '>')
                                lua_code += "    " + l.substr(2) + "\n";
                            else if (!l.empty())
                                lua_code += "    " + l + "\n";
                        }

                        const NodeTypeDef* nd = GetTypeDef(*n);
                        if (nd) {
                            for (size_t oi = 0; oi < nd->outputs.size(); oi++) {
                                if (nd->outputs[oi].type == PinType::Flow) {
                                    for (auto& c2 : m_connections) {
                                        if (c2.from_node_id == n->id && c2.from_pin_index == (int)oi)
                                            gen_body(c2.to_node_id);
                                    }
                                }
                            }
                        }
                    };
                    gen_body(conn.to_node_id);
                    break;
                }
            }
            lua_code += "end\n\n";
        }
    }

    for (auto& node : m_nodes) {
        if (global_visited.count(node.id)) continue;
        const NodeTypeDef* def = GetTypeDef(node);
        if (!def) continue;

        std::string node_code = GenerateNodeCode(node, output_vars, var_counter);
        if (!node_code.empty()) {
            std::istringstream stream(node_code);
            std::string l;
            while (std::getline(stream, l)) {
                if (l.size() >= 2 && l[0] == '>' && l[1] == '>')
                    lua_code += l.substr(2) + "\n";
                else if (!l.empty())
                    lua_code += l + "\n";
            }
            lua_code += "\n";
        }
    }

    return lua_code;
}

void NodeEditor::RenderInlineEditor(NodeInstance&) {
}
