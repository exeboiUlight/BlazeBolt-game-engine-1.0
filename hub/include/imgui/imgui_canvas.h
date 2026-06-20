// Canvas widget - view over infinite virtual space.
//
// VERSION 0.1
//
// LICENSE
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.
//
// CREDITS
//   Written by Michal Cichon
# ifndef __IMGUI_EX_CANVAS_H__
# define __IMGUI_EX_CANVAS_H__
# pragma once

# include <imgui.h>
# include <imgui_internal.h>

#ifndef IMGUIEX_CANVAS_API
#define IMGUIEX_CANVAS_API
#endif

namespace ImGuiEx {

struct CanvasView
{
    ImVec2 Origin;
    float  Scale  = 1.0f;
    float  InvScale = 1.0f;

    CanvasView() = default;
    CanvasView(const ImVec2& origin, float scale)
        : Origin(origin)
        , Scale(scale)
        , InvScale(scale ? 1.0f / scale : 0.0f)
    {
    }

    void Set(const ImVec2& origin, float scale)
    {
        *this = CanvasView(origin, scale);
    }
};

struct Canvas
{
    IMGUIEX_CANVAS_API bool Begin(const char* id, const ImVec2& size);
    IMGUIEX_CANVAS_API bool Begin(ImGuiID id, const ImVec2& size);
    IMGUIEX_CANVAS_API void End();

    IMGUIEX_CANVAS_API void SetView(const ImVec2& origin, float scale);
    IMGUIEX_CANVAS_API void SetView(const CanvasView& view);

    IMGUIEX_CANVAS_API void CenterView(const ImVec2& canvasPoint);
    IMGUIEX_CANVAS_API CanvasView CalcCenterView(const ImVec2& canvasPoint) const;
    IMGUIEX_CANVAS_API void CenterView(const ImRect& canvasRect);
    IMGUIEX_CANVAS_API CanvasView CalcCenterView(const ImRect& canvasRect) const;

    IMGUIEX_CANVAS_API void Suspend();
    IMGUIEX_CANVAS_API void Resume();

    IMGUIEX_CANVAS_API ImVec2 FromLocal(const ImVec2& point) const;
    IMGUIEX_CANVAS_API ImVec2 FromLocal(const ImVec2& point, const CanvasView& view) const;
    IMGUIEX_CANVAS_API ImVec2 FromLocalV(const ImVec2& vector) const;
    IMGUIEX_CANVAS_API ImVec2 FromLocalV(const ImVec2& vector, const CanvasView& view) const;
    IMGUIEX_CANVAS_API ImVec2 ToLocal(const ImVec2& point) const;
    IMGUIEX_CANVAS_API ImVec2 ToLocal(const ImVec2& point, const CanvasView& view) const;
    IMGUIEX_CANVAS_API ImVec2 ToLocalV(const ImVec2& vector) const;
    IMGUIEX_CANVAS_API ImVec2 ToLocalV(const ImVec2& vector, const CanvasView& view) const;

    const ImRect& Rect() const { return m_WidgetRect; }
    const ImRect& ViewRect() const { return m_ViewRect; }
    IMGUIEX_CANVAS_API ImRect CalcViewRect(const CanvasView& view) const;

    const CanvasView& View() const { return m_View; }
    const ImVec2& ViewOrigin()  const { return m_View.Origin; }
    float ViewScale() const { return m_View.Scale; }

    bool IsSuspended() const { return m_SuspendCounter > 0; }

private:
    void UpdateViewTransformPosition();
    void SaveInputState();
    void RestoreInputState();
    void SaveViewportState();
    void RestoreViewportState();
    void EnterLocalSpace();
    void LeaveLocalSpace();

    bool m_InBeginEnd = false;

    ImVec2 m_WidgetPosition;
    ImVec2 m_WidgetSize;
    ImRect m_WidgetRect;

    ImDrawList* m_DrawList = nullptr;
    int m_ExpectedChannel = 0;

    int m_DrawListFirstCommandIndex = 0;
    int m_DrawListCommadBufferSize = 0;
    int m_DrawListStartVertexIndex = 0;

    CanvasView  m_View;
    ImRect      m_ViewRect;

    ImVec2 m_ViewTransformPosition;

    int m_SuspendCounter = 0;

    float m_LastFringeScale = 1.0f;

    ImVec2 m_MousePosBackup;
    ImVec2 m_MousePosPrevBackup;
    ImVec2 m_MouseClickedPosBackup[IM_ARRAYSIZE(ImGuiIO::MouseClickedPos)];
    ImVec2 m_WindowCursorMaxBackup;

# if defined(IMGUI_HAS_VIEWPORT)
    ImVec2 m_WindowPosBackup;
    ImVec2 m_ViewportPosBackup;
    ImVec2 m_ViewportSizeBackup;
# if IMGUI_VERSION_NUM > 18002
    ImVec2 m_ViewportWorkPosBackup;
    ImVec2 m_ViewportWorkSizeBackup;
# else
    ImVec2 m_ViewportWorkOffsetMinBackup;
    ImVec2 m_ViewportWorkOffsetMaxBackup;
# endif
# endif
};

} // namespace ImGuiEx

#endif // __IMGUI_EX_CANVAS_H__
