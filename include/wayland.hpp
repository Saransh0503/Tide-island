#pragma once

#include <expected>

// ============================================================================
// Tide Island Wayland API
// ============================================================================
//
// The Wayland backend owns the layer-shell surface and EGL context used by the
// renderer.
//
namespace Wayland {

std::expected<void, const char*> init();
void request_resize(int width, int height);
void swap_buffer();
void dispatch_events();
void shutdown();

} // namespace Wayland
