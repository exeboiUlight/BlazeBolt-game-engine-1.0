#include "node_editor.hpp"
#include "editor.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <queue>

static inline ImU32 ClampByte(int v) { return (ImU32)(v < 0 ? 0 : (v > 255 ? 255 : v)); }

static const float NODE_RADIUS = 6.0f;
static const float NODE_HEADER_HEIGHT = 24.0f;
static const float NODE_PIN_SPACING = 20.0f;
static const float NODE_PADDING = 8.0f;
static const float NODE_MIN_WIDTH = 140.0f;
static const float NODE_INLINE_HEIGHT = 22.0f;

NodeEditor::NodeEditor() {
    m_state.offset = ImVec2(400, 300);
    m_state.zoom = 1.0f;
}

NodeEditor::~NodeEditor() {}

void NodeEditor::Clear() {
    m_state.nodes.clear();
    m_state.connections.clear();
    m_state.selected_nodes.clear();
    m_state.selected_node_id = -1;
    m_state.hovered_node_id = -1;
    m_state.dragging_node_id = -1;
    m_state.is_connecting = false;
    m_state.is_panning = false;
    m_state.is_selecting = false;
    m_state.modified = false;
    m_state.current_file_path.clear();
}

// ============================================================================
// Coordinate conversion
// ============================================================================

ImVec2 NodeEditor::ScreenToCanvas(ImVec2 screen_pos) {
    ImVec2 origin = m_state.canvas_pos_cache;
    return ImVec2(
        (screen_pos.x - origin.x - m_state.offset.x) / m_state.zoom,
        (screen_pos.y - origin.y - m_state.offset.y) / m_state.zoom
    );
}

ImVec2 NodeEditor::CanvasToScreen(ImVec2 canvas_pos) {
    ImVec2 origin = m_state.canvas_pos_cache;
    return ImVec2(
        canvas_pos.x * m_state.zoom + m_state.offset.x + origin.x,
        canvas_pos.y * m_state.zoom + m_state.offset.y + origin.y
    );
}

// ============================================================================
// Lookup helpers
// ============================================================================

NodeInstance* NodeEditor::FindNode(int node_id) {
    for (auto& n : m_state.nodes) {
        if (n.id == node_id) return &n;
    }
    return nullptr;
}

const NodeTypeDef* NodeEditor::GetTypeDef(const NodeInstance& node) {
    return FindNodeTypeDef(node.type_id);
}

int NodeEditor::FindConnectedNode(int node_id, int pin_index) {
    for (auto& c : m_state.connections) {
        if (c.to_node_id == node_id && c.to_pin_index == pin_index) {
            return c.from_node_id;
        }
    }
    return -1;
}

bool NodeEditor::HasInputConnection(int node_id, int pin_index) {
    return FindConnectedNode(node_id, pin_index) != -1;
}

// ============================================================================
// Pin / category colors
// ============================================================================

ImU32 NodeEditor::GetPinColor(PinType type) {
    switch (type) {
        case PinType::Flow:    return IM_COL32(255, 255, 255, 255);
        case PinType::Number:  return IM_COL32(100, 200, 100, 255);
        case PinType::Integer: return IM_COL32(100, 180, 100, 255);
        case PinType::String:  return IM_COL32(200, 200, 100, 255);
        case PinType::Bool:    return IM_COL32(200, 80, 80, 255);
        case PinType::Entity:  return IM_COL32(80, 200, 200, 255);
        case PinType::Table:   return IM_COL32(180, 120, 200, 255);
        case PinType::Vector2: return IM_COL32(100, 180, 220, 255);
        case PinType::Vector3: return IM_COL32(100, 160, 220, 255);
        case PinType::Any:     return IM_COL32(150, 150, 150, 255);
        default:               return IM_COL32(128, 128, 128, 255);
    }
}

ImU32 NodeEditor::GetCategoryHeaderColor(NodeCategory cat) {
    ImColor c = GetCategoryColor(cat);
    return IM_COL32(c.Value.x * 255, c.Value.y * 255, c.Value.z * 255, 255);
}

// ============================================================================
// Node sizing & pin positions
// ============================================================================

void NodeEditor::ComputeNodeSize(NodeInstance& node) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def) return;

    float max_pins = (float)std::max(def->inputs.size(), def->outputs.size());
    float height = NODE_HEADER_HEIGHT + NODE_PADDING + max_pins * NODE_PIN_SPACING + NODE_PADDING;
    if (def->has_inline) {
        height += NODE_INLINE_HEIGHT + NODE_PADDING;
    }

    ImGui::PushFont(nullptr);
    float name_width = ImGui::CalcTextSize(def->name).x;
    ImGui::PopFont();

    float width = NODE_MIN_WIDTH;
    width = std::max(width, name_width + NODE_PADDING * 2 + 20.0f);

    node.size = ImVec2(width, height);
}

void NodeEditor::ComputePinPositions(NodeInstance& node) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def) return;

    node.input_pins.clear();
    node.output_pins.clear();

    float y_start = NODE_HEADER_HEIGHT + NODE_PADDING;

    for (size_t i = 0; i < def->inputs.size(); i++) {
        NodePin pin;
        pin.id = node.id * 100 + (int)i;
        pin.name = def->inputs[i].name;
        pin.type = def->inputs[i].type;
        pin.is_input = true;
        pin.position = ImVec2(0.0f, y_start + i * NODE_PIN_SPACING);
        node.input_pins.push_back(pin);
    }

    for (size_t i = 0; i < def->outputs.size(); i++) {
        NodePin pin;
        pin.id = node.id * 100 + 50 + (int)i;
        pin.name = def->outputs[i].name;
        pin.type = def->outputs[i].type;
        pin.is_input = false;
        pin.position = ImVec2(node.size.x, y_start + i * NODE_PIN_SPACING);
        node.output_pins.push_back(pin);
    }
}

ImVec2 NodeEditor::GetPinPosition(const NodeInstance& node, int pin_index, bool is_input) {
    ImVec2 abs_pos = ImVec2(node.position.x + (is_input ? 0.0f : node.size.x),
                             node.position.y + NODE_HEADER_HEIGHT + NODE_PADDING + pin_index * NODE_PIN_SPACING);
    return abs_pos;
}

// ============================================================================
// Canvas rendering
// ============================================================================

void NodeEditor::Render(EditorTab& tab) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);

    ImGui::BeginChild("##NodeCanvas", ImVec2(0, 0), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    m_state.canvas_pos_cache = canvas_pos;

    draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
                             IM_COL32(22, 22, 28, 255));

    RenderGrid(draw_list, canvas_pos, canvas_size);
    RenderConnections(draw_list);
    RenderNodes(draw_list);

    if (m_state.is_connecting) {
        RenderConnectionBeingCreated(draw_list);
    }
    if (m_state.is_selecting) {
        RenderSelectionRect(draw_list);
    }

    HandleCanvasInteraction();

    RenderPalette();

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}

void NodeEditor::RenderGrid(ImDrawList* draw_list, ImVec2 canvas_pos, ImVec2 canvas_size) {
    float grid_step = 20.0f * m_state.zoom;
    if (grid_step < 5.0f) grid_step = 5.0f;

    ImU32 grid_color = IM_COL32(35, 35, 45, 255);
    ImU32 grid_color_bold = IM_COL32(45, 45, 60, 255);

    float ox = fmodf(m_state.offset.x, grid_step);
    float oy = fmodf(m_state.offset.y, grid_step);

    for (float x = ox; x < canvas_size.x; x += grid_step) {
        float abs_x = canvas_pos.x + x;
        bool bold = (fmodf(x, grid_step * 5) < 1.0f || grid_step * 5.0f - fmodf(x, grid_step * 5.0f) < 1.0f);
        draw_list->AddLine(ImVec2(abs_x, canvas_pos.y), ImVec2(abs_x, canvas_pos.y + canvas_size.y),
                           bold ? grid_color_bold : grid_color, bold ? 1.0f : 0.5f);
    }
    for (float y = oy; y < canvas_size.y; y += grid_step) {
        float abs_y = canvas_pos.y + y;
        bool bold = (fmodf(y, grid_step * 5) < 1.0f || grid_step * 5.0f - fmodf(y, grid_step * 5.0f) < 1.0f);
        draw_list->AddLine(ImVec2(canvas_pos.x, abs_y), ImVec2(canvas_pos.x + canvas_size.x, abs_y),
                           bold ? grid_color_bold : grid_color, bold ? 1.0f : 0.5f);
    }
}

// ============================================================================
// Node rendering
// ============================================================================

void NodeEditor::RenderNodes(ImDrawList* draw_list) {
    for (auto& node : m_state.nodes) {
        ComputeNodeSize(node);
        ComputePinPositions(node);
        RenderNode(draw_list, node);
    }
}

void NodeEditor::RenderNode(ImDrawList* draw_list, NodeInstance& node) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def) return;

    ImVec2 screen_pos = CanvasToScreen(node.position);
    ImVec2 screen_size = ImVec2(node.size.x * m_state.zoom, node.size.y * m_state.zoom);

    bool is_selected = IsNodeSelected(node.id);
    bool is_hovered = (m_state.hovered_node_id == node.id);

    ImU32 shadow_color = IM_COL32(0, 0, 0, 60);
    ImVec2 shadow_offset = ImVec2(3.0f * m_state.zoom, 3.0f * m_state.zoom);
    draw_list->AddRectFilled(
        ImVec2(screen_pos.x + shadow_offset.x, screen_pos.y + shadow_offset.y),
        ImVec2(screen_pos.x + screen_size.x + shadow_offset.x, screen_pos.y + screen_size.y + shadow_offset.y),
        shadow_color, 6.0f);

    ImU32 body_color = IM_COL32(35, 35, 42, 245);
    ImU32 border_color = IM_COL32(65, 65, 75, 255);
    if (is_selected) border_color = IM_COL32(80, 160, 255, 255);
    else if (is_hovered) border_color = IM_COL32(90, 90, 110, 255);

    draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + screen_size.x, screen_pos.y + screen_size.y),
                             body_color, 6.0f);

    ImU32 header_color = GetCategoryHeaderColor(def->category);
    ImVec2 header_end = ImVec2(screen_pos.x + screen_size.x, screen_pos.y + NODE_HEADER_HEIGHT * m_state.zoom);

    draw_list->AddRectFilled(screen_pos, header_end, header_color, 6.0f, ImDrawFlags_RoundCornersTop);
    draw_list->AddRectFilled(ImVec2(screen_pos.x, header_end.y - 6.0f), header_end, header_color, 0.0f);

    ImU32 header_line_color = IM_COL32(
        ((header_color >> 0) & 0xFF) * 60 / 255,
        ((header_color >> 8) & 0xFF) * 60 / 255,
        ((header_color >> 16) & 0xFF) * 60 / 255, 180);
    draw_list->AddLine(
        ImVec2(screen_pos.x, header_end.y),
        ImVec2(screen_pos.x + screen_size.x, header_end.y),
        header_line_color, 1.0f);

    float border_thickness = is_selected ? 2.0f : 1.0f;
    if (is_selected) {
        ImU32 glow_color = IM_COL32(80, 160, 255, 60);
        for (int g = 0; g < 2; g++) {
            float expand = (float)(g + 1) * 1.5f;
            draw_list->AddRect(
                ImVec2(screen_pos.x - expand, screen_pos.y - expand),
                ImVec2(screen_pos.x + screen_size.x + expand, screen_pos.y + screen_size.y + expand),
                glow_color, 6.0f, 0, 1.0f);
        }
    }
    draw_list->AddRect(screen_pos, ImVec2(screen_pos.x + screen_size.x, screen_pos.y + screen_size.y),
                       border_color, 6.0f, 0, border_thickness);

    ImVec2 text_pos = ImVec2(screen_pos.x + NODE_PADDING * m_state.zoom, screen_pos.y + 5.0f * m_state.zoom);
    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), def->name);

    ImVec2 mouse = ImGui::GetIO().MousePos;
    float pin_base_radius = NODE_RADIUS * m_state.zoom;
    float pin_hover_radius = pin_base_radius + 3.0f * m_state.zoom;

    for (size_t i = 0; i < node.input_pins.size(); i++) {
        ImVec2 pin_canvas = ImVec2(node.position.x, node.position.y + NODE_HEADER_HEIGHT + NODE_PADDING + i * NODE_PIN_SPACING);
        ImVec2 pin_screen = CanvasToScreen(pin_canvas);
        ImU32 pin_color = GetPinColor(node.input_pins[i].type);

        float dist_to_mouse = std::sqrtf(
            (mouse.x - pin_screen.x) * (mouse.x - pin_screen.x) +
            (mouse.y - pin_screen.y) * (mouse.y - pin_screen.y));
        bool pin_hovered = dist_to_mouse < pin_hover_radius + 4.0f;

        if (pin_hovered) {
            ImU32 glow = IM_COL32(
                (pin_color >> 0) & 0xFF,
                (pin_color >> 8) & 0xFF,
                (pin_color >> 16) & 0xFF, 80);
            draw_list->AddCircleFilled(pin_screen, pin_hover_radius + 4.0f, glow);
            draw_list->AddCircleFilled(pin_screen, pin_hover_radius + 2.0f, IM_COL32(
                (pin_color >> 0) & 0xFF,
                (pin_color >> 8) & 0xFF,
                (pin_color >> 16) & 0xFF, 160));
        }

        draw_list->AddCircleFilled(pin_screen, pin_base_radius, pin_color);
        draw_list->AddCircle(pin_screen, pin_base_radius, IM_COL32(0, 0, 0, 180), 0, 1.5f);

        ImU32 highlight = IM_COL32(
            ClampByte(((pin_color >> 0) & 0xFF) + 80),
            ClampByte(((pin_color >> 8) & 0xFF) + 80),
            ClampByte(((pin_color >> 16) & 0xFF) + 80), 120);
        draw_list->AddCircleFilled(
            ImVec2(pin_screen.x - pin_base_radius * 0.3f, pin_screen.y - pin_base_radius * 0.3f),
            pin_base_radius * 0.35f, highlight);

        ImVec2 label_pos = ImVec2(pin_screen.x + pin_base_radius + 5.0f, pin_screen.y - 6.0f * m_state.zoom);
        draw_list->AddText(label_pos, IM_COL32(210, 210, 215, 255), node.input_pins[i].name.c_str());
    }

    for (size_t i = 0; i < node.output_pins.size(); i++) {
        ImVec2 pin_canvas = ImVec2(node.position.x + node.size.x,
                                   node.position.y + NODE_HEADER_HEIGHT + NODE_PADDING + i * NODE_PIN_SPACING);
        ImVec2 pin_screen = CanvasToScreen(pin_canvas);
        ImU32 pin_color = GetPinColor(node.output_pins[i].type);

        float dist_to_mouse = std::sqrtf(
            (mouse.x - pin_screen.x) * (mouse.x - pin_screen.x) +
            (mouse.y - pin_screen.y) * (mouse.y - pin_screen.y));
        bool pin_hovered = dist_to_mouse < pin_hover_radius + 4.0f;

        if (pin_hovered) {
            ImU32 glow = IM_COL32(
                (pin_color >> 0) & 0xFF,
                (pin_color >> 8) & 0xFF,
                (pin_color >> 16) & 0xFF, 80);
            draw_list->AddCircleFilled(pin_screen, pin_hover_radius + 4.0f, glow);
            draw_list->AddCircleFilled(pin_screen, pin_hover_radius + 2.0f, IM_COL32(
                (pin_color >> 0) & 0xFF,
                (pin_color >> 8) & 0xFF,
                (pin_color >> 16) & 0xFF, 160));
        }

        draw_list->AddCircleFilled(pin_screen, pin_base_radius, pin_color);
        draw_list->AddCircle(pin_screen, pin_base_radius, IM_COL32(0, 0, 0, 180), 0, 1.5f);

        ImU32 highlight = IM_COL32(
            ClampByte(((pin_color >> 0) & 0xFF) + 80),
            ClampByte(((pin_color >> 8) & 0xFF) + 80),
            ClampByte(((pin_color >> 16) & 0xFF) + 80), 120);
        draw_list->AddCircleFilled(
            ImVec2(pin_screen.x - pin_base_radius * 0.3f, pin_screen.y - pin_base_radius * 0.3f),
            pin_base_radius * 0.35f, highlight);

        ImVec2 label_pos = ImVec2(pin_screen.x - pin_base_radius - 5.0f, pin_screen.y - 6.0f * m_state.zoom);
        ImVec2 text_size = ImGui::CalcTextSize(node.output_pins[i].name.c_str());
        draw_list->AddText(ImVec2(label_pos.x - text_size.x, label_pos.y), IM_COL32(210, 210, 215, 255),
                           node.output_pins[i].name.c_str());
    }

    if (def->has_inline) {
        RenderInlineEditor(node);
    }
}

void NodeEditor::RenderInlineEditor(NodeInstance& node) {
    const NodeTypeDef* def = GetTypeDef(node);
    if (!def || !def->has_inline) return;

    ImVec2 screen_pos = CanvasToScreen(node.position);
    ImVec2 screen_size = ImVec2(node.size.x * m_state.zoom, node.size.y * m_state.zoom);

    float inline_y = screen_pos.y + (NODE_HEADER_HEIGHT + NODE_PADDING + node.input_pins.size() * NODE_PIN_SPACING + NODE_PADDING * 0.5f) * m_state.zoom;
    float inline_x = screen_pos.x + NODE_PADDING * m_state.zoom;
    float inline_w = screen_size.x - NODE_PADDING * 2.0f * m_state.zoom;
    float inline_h = NODE_INLINE_HEIGHT * m_state.zoom;

    std::string label = "##inline_" + std::to_string(node.id);
    std::string& val = m_state.nodes[0].pin_values[0];

    auto it = m_state.nodes.begin();
    while (it != m_state.nodes.end()) {
        if (it->id == node.id) break;
        ++it;
    }
    if (it == m_state.nodes.end()) return;

    std::string& value_ref = it->pin_values[-1];
    if (it->pin_values.find(-1) == it->pin_values.end()) {
        it->pin_values[-1] = "";
    }

    ImGui::SetCursorScreenPos(ImVec2(inline_x, inline_y));

    ImGui::PushItemWidth(inline_w);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * m_state.zoom, 2.0f * m_state.zoom));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

    ImGui::PushID(node.id);

    switch (def->inline_type) {
        case PinType::String: {
            char buf[512];
            auto pit = it->pin_values.find(-1);
            if (pit != it->pin_values.end()) {
                strncpy(buf, pit->second.c_str(), sizeof(buf) - 1);
            } else {
                buf[0] = '\0';
            }
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText("##val", buf, sizeof(buf))) {
                it->pin_values[-1] = buf;
                m_state.modified = true;
            }
            break;
        }
        case PinType::Number: {
            float fv = 0.0f;
            auto pit = it->pin_values.find(-1);
            if (pit != it->pin_values.end()) {
                fv = std::strtof(pit->second.c_str(), nullptr);
            }
            if (ImGui::InputFloat("##val", &fv, 0.0f, 0.0f, "%.3f")) {
                it->pin_values[-1] = std::to_string(fv);
                m_state.modified = true;
            }
            break;
        }
        case PinType::Integer: {
            int iv = 0;
            auto pit = it->pin_values.find(-1);
            if (pit != it->pin_values.end()) {
                iv = std::atoi(pit->second.c_str());
            }
            if (ImGui::InputInt("##val", &iv, 0, 0)) {
                it->pin_values[-1] = std::to_string(iv);
                m_state.modified = true;
            }
            break;
        }
        case PinType::Bool: {
            bool bv = false;
            auto pit = it->pin_values.find(-1);
            if (pit != it->pin_values.end()) {
                bv = (pit->second == "true" || pit->second == "1");
            }
            if (ImGui::Checkbox("##val", &bv)) {
                it->pin_values[-1] = bv ? "true" : "false";
                m_state.modified = true;
            }
            break;
        }
        default: {
            char buf[512];
            auto pit = it->pin_values.find(-1);
            if (pit != it->pin_values.end()) {
                strncpy(buf, pit->second.c_str(), sizeof(buf) - 1);
            } else {
                buf[0] = '\0';
            }
            buf[sizeof(buf) - 1] = '\0';
            if (ImGui::InputText("##val", buf, sizeof(buf))) {
                it->pin_values[-1] = buf;
                m_state.modified = true;
            }
            break;
        }
    }

    ImGui::PopID();
    ImGui::PopStyleVar(2);
    ImGui::PopItemWidth();
}

// ============================================================================
// Connection rendering
// ============================================================================

void NodeEditor::RenderConnections(ImDrawList* draw_list) {
    for (auto& conn : m_state.connections) {
        NodeInstance* from_node = FindNode(conn.from_node_id);
        NodeInstance* to_node = FindNode(conn.to_node_id);
        if (!from_node || !to_node) continue;

        ImVec2 start_pin = GetPinPosition(*from_node, conn.from_pin_index, false);
        ImVec2 end_pin = GetPinPosition(*to_node, conn.to_pin_index, true);

        ImVec2 start_screen = CanvasToScreen(start_pin);
        ImVec2 end_screen = CanvasToScreen(end_pin);

        float dist = std::sqrtf((end_screen.x - start_screen.x) * (end_screen.x - start_screen.x) +
                                (end_screen.y - start_screen.y) * (end_screen.y - start_screen.y));
        float bezier_offset = std::max(50.0f, dist * 0.4f) * m_state.zoom;

        ImVec2 cp1 = ImVec2(start_screen.x + bezier_offset, start_screen.y);
        ImVec2 cp2 = ImVec2(end_screen.x - bezier_offset, end_screen.y);

        PinType pin_type = PinType::Any;
        for (auto& p : from_node->output_pins) {
            if (p.is_input == false) {
                if (conn.from_pin_index >= 0 && conn.from_pin_index < (int)from_node->output_pins.size()) {
                    pin_type = from_node->output_pins[conn.from_pin_index].type;
                    break;
                }
            }
        }

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool hovered = false;
        for (float t = 0.0f; t <= 1.0f; t += 0.02f) {
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;
            float uuu = uu * u;
            float ttt = tt * t;
            ImVec2 p = ImVec2(
                uuu * start_screen.x + 3 * uu * t * cp1.x + 3 * u * tt * cp2.x + ttt * end_screen.x,
                uuu * start_screen.y + 3 * uu * t * cp1.y + 3 * u * tt * cp2.y + ttt * end_screen.y
            );
            float d = std::sqrtf((mouse.x - p.x) * (mouse.x - p.x) + (mouse.y - p.y) * (mouse.y - p.y));
            if (d < 8.0f) { hovered = true; break; }
        }

        float thickness = hovered ? 3.5f : 2.5f;
        ImU32 conn_color = GetPinColor(pin_type);
        if (hovered) {
            conn_color = IM_COL32(255, 120, 120, 255);
        }

        if (!hovered) {
            ImU32 shadow_conn = IM_COL32(0, 0, 0, 40);
            ImVec2 so = ImVec2(1.0f, 1.5f);
            draw_list->AddBezierCubic(
                ImVec2(start_screen.x + so.x, start_screen.y + so.y),
                ImVec2(cp1.x + so.x, cp1.y + so.y),
                ImVec2(cp2.x + so.x, cp2.y + so.y),
                ImVec2(end_screen.x + so.x, end_screen.y + so.y),
                shadow_conn, thickness + 1.0f);
        }

        draw_list->AddBezierCubic(start_screen, cp1, cp2, end_screen, conn_color, thickness);

        if (hovered) {
            draw_list->AddBezierCubic(start_screen, cp1, cp2, end_screen, IM_COL32(255, 200, 200, 60), thickness + 3.0f);
        }
    }
}

void NodeEditor::RenderConnectionBeingCreated(ImDrawList* draw_list) {
    ImVec2 mouse_screen = ImGui::GetIO().MousePos;

    ImVec2 start_screen;
    NodeInstance* from_node = FindNode(m_state.connecting_from_node);
    if (from_node && m_state.connecting_from_pin >= 0) {
        ImVec2 pin_pos;
        if (m_state.connecting_from_output) {
            pin_pos = GetPinPosition(*from_node, m_state.connecting_from_pin, false);
        } else {
            pin_pos = GetPinPosition(*from_node, m_state.connecting_from_pin, true);
        }
        start_screen = CanvasToScreen(pin_pos);
    } else {
        start_screen = mouse_screen;
    }

    ImVec2 end_screen;
    if (m_state.is_connecting) {
        end_screen = mouse_screen;
    } else {
        end_screen = mouse_screen;
    }

    if (!m_state.connecting_from_output) {
        std::swap(start_screen, end_screen);
    }

    float dist = std::sqrtf((end_screen.x - start_screen.x) * (end_screen.x - start_screen.x) +
                            (end_screen.y - start_screen.y) * (end_screen.y - start_screen.y));
    float bezier_offset = std::max(50.0f, dist * 0.4f);

    ImVec2 cp1 = ImVec2(start_screen.x + bezier_offset, start_screen.y);
    ImVec2 cp2 = ImVec2(end_screen.x - bezier_offset, end_screen.y);

    draw_list->AddBezierCubic(start_screen, cp1, cp2, end_screen, IM_COL32(200, 200, 210, 140), 2.0f);
    draw_list->AddBezierCubic(start_screen, cp1, cp2, end_screen, IM_COL32(255, 255, 255, 50), 5.0f);
}

// ============================================================================
// Selection rectangle
// ============================================================================

void NodeEditor::RenderSelectionRect(ImDrawList* draw_list) {
    ImVec2 canvas_origin = ImGui::GetCursorScreenPos();

    ImVec2 s = ImVec2(
        canvas_origin.x + m_state.offset.x + m_state.selection_start.x * m_state.zoom,
        canvas_origin.y + m_state.offset.y + m_state.selection_start.y * m_state.zoom
    );
    ImVec2 e = ImVec2(
        canvas_origin.x + m_state.offset.x + m_state.selection_end.x * m_state.zoom,
        canvas_origin.y + m_state.offset.y + m_state.selection_end.y * m_state.zoom
    );

    ImVec2 r_min = ImVec2(std::min(s.x, e.x), std::min(s.y, e.y));
    ImVec2 r_max = ImVec2(std::max(s.x, e.x), std::max(s.y, e.y));

    draw_list->AddRectFilled(r_min, r_max, IM_COL32(80, 150, 255, 30));
    draw_list->AddRect(r_min, r_max, IM_COL32(80, 150, 255, 180), 0.0f, 0, 1.5f);

    for (float x = r_min.x; x < r_max.x; x += 6.0f) {
        draw_list->AddLine(ImVec2(x, r_min.y), ImVec2(x + 3.0f, r_min.y), IM_COL32(80, 150, 255, 100), 1.0f);
        draw_list->AddLine(ImVec2(x, r_max.y), ImVec2(x + 3.0f, r_max.y), IM_COL32(80, 150, 255, 100), 1.0f);
    }
    for (float y = r_min.y; y < r_max.y; y += 6.0f) {
        draw_list->AddLine(ImVec2(r_min.x, y), ImVec2(r_min.x, y + 3.0f), IM_COL32(80, 150, 255, 100), 1.0f);
        draw_list->AddLine(ImVec2(r_max.x, y), ImVec2(r_max.x, y + 3.0f), IM_COL32(80, 150, 255, 100), 1.0f);
    }
}

// ============================================================================
// Palette
// ============================================================================

void NodeEditor::RenderPalette() {
    if (!ImGui::IsPopupOpen("NodePalette")) return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(28, 28, 34, 248));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(60, 60, 75, 180));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, IM_COL32(22, 22, 28, 200));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, IM_COL32(55, 55, 65, 255));

    if (ImGui::BeginPopup("NodePalette")) {
        ImGui::InputTextWithHint("##search", "Search...", m_state.palette_search, sizeof(m_state.palette_search));
        ImGui::Separator();

        const auto& defs = GetNodeTypeDefs();
        std::set<int> shown_categories;
        std::string search_lower = m_state.palette_search;
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
                std::string name_lower = d.name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                if (search_lower.empty() || name_lower.find(search_lower) != std::string::npos) {
                    any_match = true;
                    break;
                }
            }
            if (!any_match) continue;

            if (ImGui::TreeNodeEx(GetCategoryName(cat), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& d : defs) {
                    if (d.category != cat) continue;
                    std::string name_lower = d.name;
                    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                    if (!search_lower.empty() && name_lower.find(search_lower) == std::string::npos) continue;

                    ImU32 cat_col = GetCategoryHeaderColor(cat);
                    ImVec2 text_pos = ImGui::GetCursorScreenPos();
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(text_pos.x, text_pos.y),
                        ImVec2(text_pos.x + 3.0f, text_pos.y + ImGui::GetFrameHeight()),
                        cat_col);

                    if (ImGui::Selectable(d.name, false, ImGuiSelectableFlags_None, ImVec2(200, 18))) {
                        ImVec2 canvas_mouse = ScreenToCanvas(ImGui::GetIO().MousePos);
                        AddNode(d.id, canvas_mouse);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);
}

// ============================================================================
// Interaction
// ============================================================================

void NodeEditor::HandleCanvasInteraction() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mouse_screen = io.MousePos;

    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (!m_state.selected_nodes.empty()) {
            DeleteSelectedNodes();
        }
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
        CopySelectedNodes();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X)) {
        CutSelectedNodes();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
        PasteNodes();
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (!m_state.current_file_path.empty()) {
            SaveToFile(m_state.current_file_path);
        }
    }

    if (m_state.is_connecting && io.MouseReleased[0]) {
        ImVec2 mouse_canvas = ScreenToCanvas(mouse_screen);

        for (auto& node : m_state.nodes) {
            for (size_t i = 0; i < node.input_pins.size(); i++) {
                ImVec2 pin_pos = GetPinPosition(node, (int)i, true);
                float d = std::sqrtf((mouse_canvas.x - pin_pos.x) * (mouse_canvas.x - pin_pos.x) +
                                     (mouse_canvas.y - pin_pos.y) * (mouse_canvas.y - pin_pos.y));
                if (d < 12.0f / m_state.zoom) {
                    if (m_state.connecting_from_output) {
                        AddConnection(m_state.connecting_from_node, m_state.connecting_from_pin, node.id, (int)i);
                    } else {
                        AddConnection(node.id, (int)i, m_state.connecting_from_node, m_state.connecting_from_pin);
                    }
                    break;
                }
            }
            for (size_t i = 0; i < node.output_pins.size(); i++) {
                ImVec2 pin_pos = GetPinPosition(node, (int)i, false);
                float d = std::sqrtf((mouse_canvas.x - pin_pos.x) * (mouse_canvas.x - pin_pos.x) +
                                     (mouse_canvas.y - pin_pos.y) * (mouse_canvas.y - pin_pos.y));
                if (d < 12.0f / m_state.zoom) {
                    if (m_state.connecting_from_output) {
                        AddConnection(node.id, (int)i, m_state.connecting_from_node, m_state.connecting_from_pin);
                    } else {
                        AddConnection(m_state.connecting_from_node, m_state.connecting_from_pin, node.id, (int)i);
                    }
                    break;
                }
            }
        }
        m_state.is_connecting = false;
    }

    if (m_state.is_panning) {
        m_state.offset.x += io.MouseDelta.x;
        m_state.offset.y += io.MouseDelta.y;
    }

    if (m_state.is_selecting) {
        ImVec2 canvas_mouse = ScreenToCanvas(mouse_screen);
        m_state.selection_end = canvas_mouse;

        m_state.selected_nodes.clear();
        ImVec2 sel_min = ImVec2(std::min(m_state.selection_start.x, m_state.selection_end.x),
                                std::min(m_state.selection_start.y, m_state.selection_end.y));
        ImVec2 sel_max = ImVec2(std::max(m_state.selection_start.x, m_state.selection_end.x),
                                std::max(m_state.selection_start.y, m_state.selection_end.y));

        for (auto& node : m_state.nodes) {
            bool overlap = node.position.x < sel_max.x && node.position.x + node.size.x > sel_min.x &&
                           node.position.y < sel_max.y && node.position.y + node.size.y > sel_min.y;
            if (overlap) {
                m_state.selected_nodes.push_back(node.id);
            }
        }

        if (io.MouseReleased[0]) {
            m_state.is_selecting = false;
            if (!m_state.selected_nodes.empty()) {
                m_state.selected_node_id = m_state.selected_nodes[0];
            }
        }
    }

    if (m_state.dragging_node_id >= 0) {
        if (io.MouseReleased[0]) {
            m_state.dragging_node_id = -1;
        } else {
            ImVec2 mouse_canvas = ScreenToCanvas(mouse_screen);
            NodeInstance* node = FindNode(m_state.dragging_node_id);
            if (node) {
                node->position.x = mouse_canvas.x - m_state.drag_offset.x;
                node->position.y = mouse_canvas.y - m_state.drag_offset.y;
                m_state.modified = true;
            }
        }
    }

    bool hovering_node = false;
    for (auto& node : m_state.nodes) {
        ImVec2 screen_pos = CanvasToScreen(node.position);
        ImVec2 screen_size = ImVec2(node.size.x * m_state.zoom, node.size.y * m_state.zoom);

        if (mouse_screen.x >= screen_pos.x && mouse_screen.x <= screen_pos.x + screen_size.x &&
            mouse_screen.y >= screen_pos.y && mouse_screen.y <= screen_pos.y + screen_size.y) {
            m_state.hovered_node_id = node.id;
            hovering_node = true;

            for (size_t i = 0; i < node.input_pins.size(); i++) {
                ImVec2 pin_pos = GetPinPosition(node, (int)i, true);
                ImVec2 pin_screen = CanvasToScreen(pin_pos);
                float d = std::sqrtf((mouse_screen.x - pin_screen.x) * (mouse_screen.x - pin_screen.x) +
                                     (mouse_screen.y - pin_screen.y) * (mouse_screen.y - pin_screen.y));
                if (d < NODE_RADIUS * m_state.zoom + 4.0f) {
                    if (io.MouseClicked[0]) {
                        m_state.is_connecting = true;
                        m_state.connecting_from_node = node.id;
                        m_state.connecting_from_pin = (int)i;
                        m_state.connecting_from_output = false;
                        m_state.connecting_mouse_pos = mouse_screen;
                    }
                }
            }

            for (size_t i = 0; i < node.output_pins.size(); i++) {
                ImVec2 pin_pos = GetPinPosition(node, (int)i, false);
                ImVec2 pin_screen = CanvasToScreen(pin_pos);
                float d = std::sqrtf((mouse_screen.x - pin_screen.x) * (mouse_screen.x - pin_screen.x) +
                                     (mouse_screen.y - pin_screen.y) * (mouse_screen.y - pin_screen.y));
                if (d < NODE_RADIUS * m_state.zoom + 4.0f) {
                    if (io.MouseClicked[0]) {
                        m_state.is_connecting = true;
                        m_state.connecting_from_node = node.id;
                        m_state.connecting_from_pin = (int)i;
                        m_state.connecting_from_output = true;
                        m_state.connecting_mouse_pos = mouse_screen;
                    }
                }
            }

            break;
        }
    }

    if (!hovering_node) {
        m_state.hovered_node_id = -1;
    }

    ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos());

    ImGui::InvisibleButton("##canvas_bg", ImGui::GetContentRegionAvail());
    bool canvas_active = ImGui::IsItemActive();
    bool canvas_hovered = ImGui::IsItemHovered();

    if (canvas_active || canvas_hovered) {
        if (io.MouseWheel != 0.0f) {
            float old_zoom = m_state.zoom;
            m_state.zoom *= (1.0f + io.MouseWheel * 0.1f);
            m_state.zoom = std::max(0.1f, std::min(m_state.zoom, 5.0f));

            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            float mx = mouse_screen.x - canvas_pos.x;
            float my = mouse_screen.y - canvas_pos.y;

            m_state.offset.x = mx - (mx - m_state.offset.x) * (m_state.zoom / old_zoom);
            m_state.offset.y = my - (my - m_state.offset.y) * (m_state.zoom / old_zoom);
        }

        if (io.MouseClicked[0] && !hovering_node) {
            bool over_connection = false;
            for (auto& conn : m_state.connections) {
                NodeInstance* from_node = FindNode(conn.from_node_id);
                NodeInstance* to_node = FindNode(conn.to_node_id);
                if (!from_node || !to_node) continue;

                ImVec2 start_screen = CanvasToScreen(GetPinPosition(*from_node, conn.from_pin_index, false));
                ImVec2 end_screen = CanvasToScreen(GetPinPosition(*to_node, conn.to_pin_index, true));

                float bez_offset = std::max(50.0f,
                    std::sqrtf((end_screen.x - start_screen.x) * (end_screen.x - start_screen.x) +
                               (end_screen.y - start_screen.y) * (end_screen.y - start_screen.y)) * 0.4f);
                ImVec2 cp1 = ImVec2(start_screen.x + bez_offset, start_screen.y);
                ImVec2 cp2 = ImVec2(end_screen.x - bez_offset, end_screen.y);

                for (float t = 0.0f; t <= 1.0f; t += 0.02f) {
                    float u = 1.0f - t;
                    ImVec2 p = ImVec2(
                        u * u * u * start_screen.x + 3 * u * u * t * cp1.x + 3 * u * t * t * cp2.x + t * t * t * end_screen.x,
                        u * u * u * start_screen.y + 3 * u * u * t * cp1.y + 3 * u * t * t * cp2.y + t * t * t * end_screen.y
                    );
                    float d = std::sqrtf((mouse_screen.x - p.x) * (mouse_screen.x - p.x) +
                                         (mouse_screen.y - p.y) * (mouse_screen.y - p.y));
                    if (d < 8.0f) {
                        over_connection = true;
                        break;
                    }
                }
                if (over_connection) break;
            }

            if (!over_connection) {
                if (!io.KeyCtrl) {
                    ClearSelection();
                }
                ImVec2 canvas_mouse = ScreenToCanvas(mouse_screen);
                m_state.is_selecting = true;
                m_state.selection_start = canvas_mouse;
                m_state.selection_end = canvas_mouse;
            }
        }

        if (io.MouseClicked[0] && hovering_node) {
            NodeInstance* clicked_node = FindNode(m_state.hovered_node_id);
            if (clicked_node) {
                if (!io.KeyCtrl) {
                    ClearSelection();
                }
                SelectNode(clicked_node->id, io.KeyCtrl);

                m_state.dragging_node_id = clicked_node->id;
                ImVec2 canvas_mouse = ScreenToCanvas(mouse_screen);
                m_state.drag_offset = ImVec2(canvas_mouse.x - clicked_node->position.x,
                                             canvas_mouse.y - clicked_node->position.y);
                m_state.modified = true;
            }
        }

        if (io.MouseClicked[0] && !hovering_node && !m_state.is_connecting) {
            bool over_connection = false;
            for (auto& conn : m_state.connections) {
                NodeInstance* from_node = FindNode(conn.from_node_id);
                NodeInstance* to_node = FindNode(conn.to_node_id);
                if (!from_node || !to_node) continue;

                ImVec2 start_screen = CanvasToScreen(GetPinPosition(*from_node, conn.from_pin_index, false));
                ImVec2 end_screen = CanvasToScreen(GetPinPosition(*to_node, conn.to_pin_index, true));

                float bez_offset = std::max(50.0f,
                    std::sqrtf((end_screen.x - start_screen.x) * (end_screen.x - start_screen.x) +
                               (end_screen.y - start_screen.y) * (end_screen.y - start_screen.y)) * 0.4f);
                ImVec2 cp1 = ImVec2(start_screen.x + bez_offset, start_screen.y);
                ImVec2 cp2 = ImVec2(end_screen.x - bez_offset, end_screen.y);

                for (float t = 0.0f; t <= 1.0f; t += 0.02f) {
                    float u = 1.0f - t;
                    ImVec2 p = ImVec2(
                        u * u * u * start_screen.x + 3 * u * u * t * cp1.x + 3 * u * t * t * cp2.x + t * t * t * end_screen.x,
                        u * u * u * start_screen.y + 3 * u * u * t * cp1.y + 3 * u * t * t * cp2.y + t * t * t * end_screen.y
                    );
                    float d = std::sqrtf((mouse_screen.x - p.x) * (mouse_screen.x - p.x) +
                                         (mouse_screen.y - p.y) * (mouse_screen.y - p.y));
                    if (d < 8.0f) {
                        over_connection = true;
                        break;
                    }
                }
                if (over_connection) break;
            }

            if (over_connection) {
                for (auto it = m_state.connections.begin(); it != m_state.connections.end(); ) {
                    NodeInstance* from_node = FindNode(it->from_node_id);
                    NodeInstance* to_node = FindNode(it->to_node_id);
                    if (!from_node || !to_node) { ++it; continue; }

                    ImVec2 start_screen = CanvasToScreen(GetPinPosition(*from_node, it->from_pin_index, false));
                    ImVec2 end_screen = CanvasToScreen(GetPinPosition(*to_node, it->to_pin_index, true));

                    float bez_offset = std::max(50.0f,
                        std::sqrtf((end_screen.x - start_screen.x) * (end_screen.x - start_screen.x) +
                                   (end_screen.y - start_screen.y) * (end_screen.y - start_screen.y)) * 0.4f);
                    ImVec2 cp1 = ImVec2(start_screen.x + bez_offset, start_screen.y);
                    ImVec2 cp2 = ImVec2(end_screen.x - bez_offset, end_screen.y);

                    bool hit = false;
                    for (float t = 0.0f; t <= 1.0f; t += 0.02f) {
                        float u = 1.0f - t;
                        ImVec2 p = ImVec2(
                            u * u * u * start_screen.x + 3 * u * u * t * cp1.x + 3 * u * t * t * cp2.x + t * t * t * end_screen.x,
                            u * u * u * start_screen.y + 3 * u * u * t * cp1.y + 3 * u * t * t * cp2.y + t * t * t * end_screen.y
                        );
                        float d = std::sqrtf((mouse_screen.x - p.x) * (mouse_screen.x - p.x) +
                                             (mouse_screen.y - p.y) * (mouse_screen.y - p.y));
                        if (d < 8.0f) { hit = true; break; }
                    }

                    if (hit) {
                        it = m_state.connections.erase(it);
                        m_state.modified = true;
                        break;
                    } else {
                        ++it;
                    }
                }
            }
        }

        if (io.MouseClicked[1] && canvas_hovered) {
            ImGui::OpenPopup("NodePalette");
            m_state.palette_search[0] = '\0';
        }

        if (io.MouseDown[2] || (io.KeyAlt && io.MouseDown[0])) {
            if (!m_state.is_panning) {
                m_state.is_panning = true;
            }
        } else {
            m_state.is_panning = false;
        }
    }
}

void NodeEditor::HandleNodeInteraction(NodeInstance& node) {}

void NodeEditor::HandlePinInteraction(NodeInstance& node, int pin_index, bool is_input) {}

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

    for (size_t i = 0; i < def->inputs.size(); i++) {
        node.pin_values[(int)i] = "";
    }
    if (def->has_inline) {
        switch (def->inline_type) {
            case PinType::Number: node.pin_values[-1] = "0.0"; break;
            case PinType::Integer: node.pin_values[-1] = "0"; break;
            case PinType::Bool: node.pin_values[-1] = "false"; break;
            case PinType::String: node.pin_values[-1] = ""; break;
            default: node.pin_values[-1] = ""; break;
        }
    }

    m_state.nodes.push_back(node);
    m_state.modified = true;
}

void NodeEditor::DeleteSelectedNodes() {
    for (int id : m_state.selected_nodes) {
        DeleteNode(id);
    }
    m_state.selected_nodes.clear();
    m_state.selected_node_id = -1;
}

void NodeEditor::DeleteNode(int node_id) {
    RemoveAllConnectionsForNode(node_id);
    for (auto it = m_state.nodes.begin(); it != m_state.nodes.end(); ++it) {
        if (it->id == node_id) {
            m_state.nodes.erase(it);
            break;
        }
    }
    m_state.modified = true;
}

// ============================================================================
// Connection management
// ============================================================================

void NodeEditor::AddConnection(int from_node, int from_pin, int to_node, int to_pin) {
    if (from_node == to_node) return;

    for (auto& c : m_state.connections) {
        if (c.to_node_id == to_node && c.to_pin_index == to_pin) {
            c.from_node_id = from_node;
            c.from_pin_index = from_pin;
            m_state.modified = true;
            return;
        }
    }

    NodeConnection conn;
    conn.from_node_id = from_node;
    conn.from_pin_index = from_pin;
    conn.to_node_id = to_node;
    conn.to_pin_index = to_pin;
    m_state.connections.push_back(conn);
    m_state.modified = true;
}

void NodeEditor::RemoveConnectionsForPin(int node_id, int pin_index, bool is_input) {
    for (auto it = m_state.connections.begin(); it != m_state.connections.end(); ) {
        if (is_input) {
            if (it->to_node_id == node_id && it->to_pin_index == pin_index) {
                it = m_state.connections.erase(it);
                m_state.modified = true;
                continue;
            }
        } else {
            if (it->from_node_id == node_id && it->from_pin_index == pin_index) {
                it = m_state.connections.erase(it);
                m_state.modified = true;
                continue;
            }
        }
        ++it;
    }
}

void NodeEditor::RemoveAllConnectionsForNode(int node_id) {
    for (auto it = m_state.connections.begin(); it != m_state.connections.end(); ) {
        if (it->from_node_id == node_id || it->to_node_id == node_id) {
            it = m_state.connections.erase(it);
            m_state.modified = true;
        } else {
            ++it;
        }
    }
}

// ============================================================================
// Selection
// ============================================================================

void NodeEditor::ClearSelection() {
    m_state.selected_nodes.clear();
    m_state.selected_node_id = -1;
}

void NodeEditor::SelectNode(int node_id, bool add_to_selection) {
    if (!add_to_selection) {
        m_state.selected_nodes.clear();
    }
    for (int id : m_state.selected_nodes) {
        if (id == node_id) return;
    }
    m_state.selected_nodes.push_back(node_id);
    m_state.selected_node_id = node_id;
}

bool NodeEditor::IsNodeSelected(int node_id) {
    for (int id : m_state.selected_nodes) {
        if (id == node_id) return true;
    }
    return false;
}

// ============================================================================
// Copy / Cut / Paste
// ============================================================================

void NodeEditor::CopySelectedNodes() {
    m_state.clipboard_nodes.clear();
    m_state.clipboard_connections.clear();

    for (int id : m_state.selected_nodes) {
        NodeInstance* node = FindNode(id);
        if (node) {
            m_state.clipboard_nodes.push_back(*node);
        }
    }

    for (auto& conn : m_state.connections) {
        bool from_selected = false, to_selected = false;
        for (int id : m_state.selected_nodes) {
            if (conn.from_node_id == id) from_selected = true;
            if (conn.to_node_id == id) to_selected = true;
        }
        if (from_selected && to_selected) {
            m_state.clipboard_connections.push_back(conn);
        }
    }
}

void NodeEditor::CutSelectedNodes() {
    CopySelectedNodes();
    DeleteSelectedNodes();
}

void NodeEditor::PasteNodes() {
    if (m_state.clipboard_nodes.empty()) return;

    std::unordered_map<int, int> id_map;
    ImVec2 paste_center = ImVec2(0, 0);

    for (auto& node : m_state.clipboard_nodes) {
        paste_center.x += node.position.x;
        paste_center.y += node.position.y;
    }
    paste_center.x /= m_state.clipboard_nodes.size();
    paste_center.y /= m_state.clipboard_nodes.size();

    ImVec2 mouse_canvas = ScreenToCanvas(ImGui::GetIO().MousePos);

    ClearSelection();

    for (auto& node : m_state.clipboard_nodes) {
        int old_id = node.id;
        int new_id = GenerateNodeId();
        id_map[old_id] = new_id;

        NodeInstance new_node = node;
        new_node.id = new_id;
        new_node.position.x = mouse_canvas.x + (node.position.x - paste_center.x);
        new_node.position.y = mouse_canvas.y + (node.position.y - paste_center.y);

        m_state.nodes.push_back(new_node);
        SelectNode(new_id, true);
    }

    for (auto& conn : m_state.clipboard_connections) {
        NodeConnection new_conn = conn;
        auto it_from = id_map.find(conn.from_node_id);
        auto it_to = id_map.find(conn.to_node_id);
        if (it_from != id_map.end() && it_to != id_map.end()) {
            new_conn.from_node_id = it_from->second;
            new_conn.to_node_id = it_to->second;
            m_state.connections.push_back(new_conn);
        }
    }

    m_state.modified = true;
}

// ============================================================================
// Save / Load
// ============================================================================

bool NodeEditor::SaveToFile(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "[NODEMAP]\n";
    f << "VERSION=1\n";

    f << "[NODES]\n";
    for (auto& node : m_state.nodes) {
        f << "ID=" << node.id
          << ";TYPE=" << node.type_id
          << ";X=" << node.position.x
          << ";Y=" << node.position.y;

        for (auto& kv : node.pin_values) {
            f << ";V" << kv.first << "=" << kv.second;
        }
        f << "\n";
    }

    f << "[CONNECTIONS]\n";
    for (auto& conn : m_state.connections) {
        f << "FROM=" << conn.from_node_id << ":" << conn.from_pin_index
          << ";TO=" << conn.to_node_id << ":" << conn.to_pin_index << "\n";
    }

    f.close();
    m_state.current_file_path = path;
    m_state.modified = false;
    return true;
}

static std::string trim_str(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

bool NodeEditor::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    Clear();
    m_state.current_file_path = path;

    enum Section { None, Nodes, Connections };
    Section section = None;

    std::string line;
    while (std::getline(f, line)) {
        line = trim_str(line);
        if (line.empty()) continue;

        if (line == "[NODEMAP]") continue;
        if (line == "[NODES]") { section = Nodes; continue; }
        if (line == "[CONNECTIONS]") { section = Connections; continue; }

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

                if (key == "ID") node.id = std::atoi(val.c_str());
                else if (key == "TYPE") node.type_id = std::atoi(val.c_str());
                else if (key == "X") node.position.x = std::strtof(val.c_str(), nullptr);
                else if (key == "Y") node.position.y = std::strtof(val.c_str(), nullptr);
                else if (key.size() > 1 && key[0] == 'V') {
                    int pin_idx = std::atoi(key.substr(1).c_str());
                    node.pin_values[pin_idx] = val;
                }
            }

            if (FindNodeTypeDef(node.type_id)) {
                ComputeNodeSize(node);
                m_state.nodes.push_back(node);
            }
        } else if (section == Connections) {
            NodeConnection conn;
            size_t from_colon = line.find(':', line.find("FROM=") + 5);
            size_t from_semi = line.find(';');
            size_t to_colon = line.find(':', line.find("TO=") + 3);

            if (from_colon != std::string::npos && from_semi != std::string::npos && to_colon != std::string::npos) {
                conn.from_node_id = std::atoi(line.substr(line.find("FROM=") + 5, from_colon - line.find("FROM=") - 5).c_str());
                conn.from_pin_index = std::atoi(line.substr(from_colon + 1, from_semi - from_colon - 1).c_str());
                conn.to_node_id = std::atoi(line.substr(line.find("TO=") + 3, to_colon - line.find("TO=") - 3).c_str());
                conn.to_pin_index = std::atoi(line.substr(to_colon + 1).c_str());
                m_state.connections.push_back(conn);
            }
        }
    }

    f.close();
    m_state.modified = false;
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
    if (pit != node.pin_values.end() && !pit->second.empty()) {
        return pit->second;
    }

    const NodeTypeDef* def = GetTypeDef(node);
    if (def && pin_index < (int)def->inputs.size()) {
        switch (def->inputs[pin_index].type) {
            case PinType::Number: return "0.0";
            case PinType::Integer: return "0";
            case PinType::Bool: return "false";
            case PinType::String: return "\"\"";
            default: return "nil";
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
        while ((pos = tmpl.find(placeholder)) != std::string::npos) {
            tmpl.replace(pos, placeholder.size(), val);
        }
    }

    for (size_t i = 0; i < def->outputs.size(); i++) {
        std::string var_name = GetNodeOutputVar(node.id, (int)i, output_vars, var_counter);
        std::string placeholder = "${" + std::to_string(i) + "}";
        size_t pos;
        while ((pos = tmpl.find(placeholder)) != std::string::npos) {
            tmpl.replace(pos, placeholder.size(), var_name);
        }
        std::string named_placeholder = "${" + def->outputs[i].name + "}";
        while ((pos = tmpl.find(named_placeholder)) != std::string::npos) {
            tmpl.replace(pos, named_placeholder.size(), var_name);
        }
    }

    if (def->has_inline) {
        auto pit = node.pin_values.find(-1);
        std::string inline_val = (pit != node.pin_values.end()) ? pit->second : "";
        size_t pos;
        while ((pos = tmpl.find("{inline}")) != std::string::npos) {
            tmpl.replace(pos, 8, inline_val);
        }
    }

    return tmpl;
}

std::string NodeEditor::GenerateLua() {
    std::unordered_map<int, std::string> output_vars;
    int var_counter = 0;

    std::vector<int> start_nodes;
    std::vector<int> update_nodes;
    std::vector<int> draw_nodes;
    std::vector<int> end_nodes;

    for (auto& node : m_state.nodes) {
        if (node.type_id == 1000) start_nodes.push_back(node.id);
        else if (node.type_id == 1001) update_nodes.push_back(node.id);
        else if (node.type_id == 1002) draw_nodes.push_back(node.id);
        else if (node.type_id == 1003) end_nodes.push_back(node.id);
    }

    std::string lua_code;

    auto generate_function = [&](const std::string& func_name, int start_node_id, const std::string& extra_params) {
        std::string code;
        std::set<int> visited;
        std::vector<int> order;

        std::function<void(int)> dfs = [&](int node_id) {
            if (visited.count(node_id)) return;
            visited.insert(node_id);

            NodeInstance* node = FindNode(node_id);
            if (!node) return;

            for (auto& conn : m_state.connections) {
                if (conn.from_node_id == node_id && conn.from_pin_index >= 0) {
                    for (auto& conn2 : m_state.connections) {
                        if (conn2.to_node_id == conn.to_node_id && conn2.to_pin_index == conn.to_pin_index && &conn2 != &conn) {
                        }
                    }
                }
            }

            order.push_back(node_id);

            for (auto& conn : m_state.connections) {
                if (conn.from_node_id == node_id) {
                    dfs(conn.to_node_id);
                }
            }
        };

        if (start_node_id >= 0) {
            dfs(start_node_id);
        }

        for (auto& node : m_state.nodes) {
            if (!visited.count(node.id)) {
                const NodeTypeDef* def = GetTypeDef(node);
                if (def && def->category != NodeCategory::Flow) {
                    order.push_back(node.id);
                }
            }
        }

        for (int node_id : order) {
            NodeInstance* node = FindNode(node_id);
            if (!node) continue;

            std::string node_code = GenerateNodeCode(*node, output_vars, var_counter);
            if (node_code.empty()) continue;

            std::istringstream stream(node_code);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.size() >= 2 && line[0] == '>' && line[1] == '>') {
                    code += line.substr(2) + "\n";
                } else {
                    code += "    " + line + "\n";
                }
            }
        }

        if (start_node_id >= 0) {
            NodeInstance* start_node = FindNode(start_node_id);
            if (start_node) {
                std::string start_code = GenerateNodeCode(*start_node, output_vars, var_counter);
                std::string combined = "";
                std::istringstream stream(start_code);
                std::string line;
                while (std::getline(stream, line)) {
                    if (line.size() >= 2 && line[0] == '>' && line[1] == '>') {
                        combined += line.substr(2) + "\n";
                    }
                }

                std::string existing = code;
                code = combined;
                std::istringstream existing_stream(existing);
                while (std::getline(existing_stream, line)) {
                    if (!line.empty()) {
                        code += line + "\n";
                    }
                }
            }
        }

        return code;
    };

    std::string start_code = generate_function("Start", start_nodes.empty() ? -1 : start_nodes[0], "");
    std::string update_code = generate_function("Update", update_nodes.empty() ? -1 : update_nodes[0], "dt");
    std::string draw_code = generate_function("Draw", draw_nodes.empty() ? -1 : draw_nodes[0], "");
    std::string end_code = generate_function("End", end_nodes.empty() ? -1 : end_nodes[0], "");

    output_vars.clear();
    var_counter = 0;

    auto generate_clean = [&](const std::string& func_name, int start_node_id, const std::string& extra_params,
                              std::set<int>& global_visited) {
        std::string code;

        std::vector<int> exec_order;
        std::set<int> visited;

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
                        for (auto& conn : m_state.connections) {
                            if (conn.from_node_id == node_id && conn.from_pin_index == (int)out_idx) {
                                follow_exec(conn.to_node_id);
                            }
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
            std::string line;
            while (std::getline(stream, line)) {
                if (line.size() >= 2 && line[0] == '>' && line[1] == '>') {
                    code += line.substr(2) + "\n";
                } else {
                    code += "    " + line + "\n";
                }
            }
        }

        return code;
    };

    std::set<int> global_visited;

    lua_code.clear();
    output_vars.clear();
    var_counter = 0;

    auto generate_final_function = [&](const std::string& func_name, int start_node_id, const std::string& extra_params) {
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
                        for (auto& conn : m_state.connections) {
                            if (conn.from_node_id == node_id && conn.from_pin_index == (int)out_idx) {
                                follow_exec(conn.to_node_id);
                            }
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
            std::string line;
            while (std::getline(stream, line)) {
                if (line.size() >= 2 && line[0] == '>' && line[1] == '>') {
                    code += "    " + line.substr(2) + "\n";
                } else if (!line.empty()) {
                    code += "    " + line + "\n";
                }
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

    for (auto& node : m_state.nodes) {
        if (global_visited.count(node.id)) continue;

        const NodeTypeDef* def = GetTypeDef(node);
        if (!def) continue;

        if (def->id == 1011) {
            std::string func_name = GetNodeInputValue(node, 0, output_vars, var_counter);
            lua_code += "function " + func_name + "()\n";

            for (auto& conn : m_state.connections) {
                if (conn.from_node_id == node.id && conn.from_pin_index == 1) {
                    std::function<void(int)> gen_body = [&](int nid) {
                        NodeInstance* n = FindNode(nid);
                        if (!n) return;
                        global_visited.insert(nid);

                        std::string code = GenerateNodeCode(*n, output_vars, var_counter);
                        std::istringstream stream(code);
                        std::string line;
                        while (std::getline(stream, line)) {
                            if (line.size() >= 2 && line[0] == '>' && line[1] == '>') {
                                lua_code += "    " + line.substr(2) + "\n";
                            } else if (!line.empty()) {
                                lua_code += "    " + line + "\n";
                            }
                        }

                        const NodeTypeDef* nd = GetTypeDef(*n);
                        if (nd) {
                            for (size_t oi = 0; oi < nd->outputs.size(); oi++) {
                                if (nd->outputs[oi].type == PinType::Flow) {
                                    for (auto& c2 : m_state.connections) {
                                        if (c2.from_node_id == n->id && c2.from_pin_index == (int)oi) {
                                            gen_body(c2.to_node_id);
                                        }
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

    for (auto& node : m_state.nodes) {
        if (global_visited.count(node.id)) continue;

        const NodeTypeDef* def = GetTypeDef(node);
        if (!def) continue;

        std::string node_code = GenerateNodeCode(node, output_vars, var_counter);
        if (!node_code.empty()) {
            std::istringstream stream(node_code);
            std::string line;
            while (std::getline(stream, line)) {
                if (line.size() >= 2 && line[0] == '>' && line[1] == '>') {
                    lua_code += line.substr(2) + "\n";
                } else if (!line.empty()) {
                    lua_code += line + "\n";
                }
            }
            lua_code += "\n";
        }
    }

    return lua_code;
}
