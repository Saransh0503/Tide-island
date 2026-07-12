#pragma once

#include <array>
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

std::expected<void, const char*> init();
std::expected<void, const char*> draw_rectangle(float width, float height, float x, float y, float radius, std::array<float, 4> color);
void frame();
void shutdown();

} // namespace Renderer
