#pragma once

#include <imgui.h>
#include <GLFW/glfw3.h>

struct CustomTitleBar {
    GLFWwindow* window = nullptr;
    float height = 32.0f;

    bool is_dragging = false;
    double drag_screen_x = 0, drag_screen_y = 0;
    int drag_win_x = 0, drag_win_y = 0;
    int last_set_x = 0, last_set_y = 0;

    bool mouse_was_pressed = false;

    void Init(GLFWwindow* win) { window = win; }

    float GetHeight() const { return height; }

    void Render(const char* title) {
        if (!window) return;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        float bar_w = vp->Size.x;

        ImVec2 bar_min = vp->Pos;
        ImVec2 bar_max = ImVec2(vp->Pos.x + bar_w, vp->Pos.y + height);

        ImDrawList* dl = ImGui::GetForegroundDrawList(vp);
        dl->AddRectFilled(bar_min, bar_max, IM_COL32(18, 18, 22, 255));

        float btn_w = 46.0f;
        float close_x = bar_max.x - btn_w;
        float max_x = close_x - btn_w;
        float min_x = max_x - btn_w;

        ImVec2 mouse = ImGui::GetIO().MousePos;
        bool in_bar = mouse.y >= bar_min.y && mouse.y < bar_max.y;
        bool h_min  = in_bar && mouse.x >= min_x && mouse.x < min_x + btn_w;
        bool h_max  = in_bar && mouse.x >= max_x && mouse.x < max_x + btn_w;
        bool h_cls  = in_bar && mouse.x >= close_x && mouse.x < close_x + btn_w;
        bool h_drag = in_bar && mouse.x < min_x;

        auto drawBtn = [&](float x, float y, float w, float h, ImU32 idle, const char* label, bool hov) {
            dl->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), hov ? IM_COL32(60, 60, 70, 255) : idle);
            ImVec2 ts = ImGui::CalcTextSize(label);
            dl->AddText(ImVec2(x + (w - ts.x) * 0.5f, y + (h - ts.y) * 0.5f), IM_COL32(200, 200, 210, 255), label);
        };

        drawBtn(min_x, bar_min.y, btn_w, height, IM_COL32(30, 30, 36, 255), "--", h_min);
        bool maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);
        drawBtn(max_x, bar_min.y, btn_w, height, IM_COL32(30, 30, 36, 255), maximized ? "[]#" : "[]", h_max);
        drawBtn(close_x, bar_min.y, btn_w, height, IM_COL32(180, 40, 40, 255), "X", h_cls);

        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(bar_min.x + 14.0f, bar_min.y + (height - ts.y) * 0.5f),
                     IM_COL32(170, 170, 180, 255), title);

        bool mouse_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (mouse_pressed && !mouse_was_pressed) {
            if (h_cls) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            } else if (h_min) {
                glfwIconifyWindow(window);
            } else if (h_max) {
                if (maximized) glfwRestoreWindow(window);
                else glfwMaximizeWindow(window);
            } else if (h_drag) {
                is_dragging = true;
                glfwGetWindowPos(window, &drag_win_x, &drag_win_y);
                double cx, cy;
                glfwGetCursorPos(window, &cx, &cy);
                drag_screen_x = drag_win_x + cx;
                drag_screen_y = drag_win_y + cy;
            }
        }

        if (!mouse_pressed) {
            is_dragging = false;
        }

        if (is_dragging && mouse_pressed) {
            int cur_x, cur_y;
            glfwGetWindowPos(window, &cur_x, &cur_y);
            double cx, cy;
            glfwGetCursorPos(window, &cx, &cy);
            int new_x = drag_win_x + (int)((cur_x + cx) - drag_screen_x);
            int new_y = drag_win_y + (int)((cur_y + cy) - drag_screen_y);
            if (new_x != last_set_x || new_y != last_set_y) {
                glfwSetWindowPos(window, new_x, new_y);
                last_set_x = new_x;
                last_set_y = new_y;
            }
        }

        if (!mouse_pressed && in_bar && ImGui::IsMouseDoubleClicked(0)) {
            if (maximized) glfwRestoreWindow(window);
            else glfwMaximizeWindow(window);
        }

        mouse_was_pressed = mouse_pressed;
    }
};

inline CustomTitleBar& GetTitleBar() {
    static CustomTitleBar s_title_bar;
    return s_title_bar;
}
