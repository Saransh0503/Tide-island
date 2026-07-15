#pragma once

#include <array>
#include <cstddef>
#include <expected>

// ============================================================================
// Tide Island renderer API
// ============================================================================
//
// The renderer owns Sokol graphics resources and draws the current island state
// into the active Wayland/EGL swapchain.
//
namespace Renderer {

struct Vertex {
    float x, y;
    float r, g, b, a;
};

enum tex_pos : char {
    begin,
    middle,
    end
};

struct ObjFrame {
    float x, y;
    float width, height;
};

std::expected<void, const char*> init();

std::expected<void, const char*> draw_rectangle(
    ObjFrame obj_frame,
    float radius,
    std::array<float, 4> color);

std::expected<void, const char*> draw_text(
    ObjFrame obj_frame,
    const char* text,
    std::size_t font_size,
    std::array<float, 4> color,
    tex_pos xpos = tex_pos::middle,
    tex_pos ypos = tex_pos::middle);

std::expected<void, const char*> frame();
void shutdown();

} // namespace Renderer
