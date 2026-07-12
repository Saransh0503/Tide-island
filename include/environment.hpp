#pragma once

// ============================================================================
// Tide Island graphics backend API
// ============================================================================
//
// This module helps avoid expensive software OpenGL backends where possible and
// reports the renderer selected after context creation.
//
namespace GraphicBackend {

enum GraphicBackendDecision {
    KeepCurrentContext,
    RecreateContextWithSoftpipe
};

void prepare_graphics_backend();
GraphicBackendDecision inspect_graphics_backend_after_context();

} // namespace GraphicBackend
