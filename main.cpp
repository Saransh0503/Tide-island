// ============================================================================
// Tide Island application entry point
// ============================================================================
//
// This file wires the shared island state, Wayland platform backend, renderer,
// and graphics-backend diagnostics into the application lifecycle.
//
#define SOKOL_IMPL

#include "island.hpp"
#include "renderer.hpp"
#include "environment.hpp"
#include "log.hpp"

#include "sokol_gfx.h"
#include "sokol_log.h"
#include "wayland.hpp"

#include <print>

using namespace std;

// ============================================================================
// [Application Lifecycle]
// ============================================================================

int main() {
    println("");

#if defined(_DEBUG) || !defined(NDEBUG)

    frame_logger(Log::Warning,
        "This build was compiled in debug mode.",
        "Performance may be reduced and additional debug output may appear."
    );

#endif

    GraphicBackend::prepare_graphics_backend();

    // Island dimensions must be known before the Wayland layer surface exists.
    Island::set_window_size(140, 40);
    Island::set_island_size(140, 38);
    Island::set_anchor_top(2);
    Island::set_radius(19);
    Island::set_zone(40);

    Log::check(Wayland::init());

    logger(Log::Debug, "Initialize Wayland successfully");

    Log::check(Renderer::init());

    logger(Log::Debug, "Initialize render successfully");

    GraphicBackend::inspect_graphics_backend_after_context();

    Renderer::frame();

    Wayland::swap_buffer();

    while (Island::state().is_running) {
        Wayland::dispatch_events();
    }

    logger(Log::Info, "Quit because island.is_running is set as false");

    Wayland::shutdown();
    Renderer::shutdown();
}
