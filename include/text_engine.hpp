#pragma once

#include <expected>
#include <cstddef>

// ============================================================================
// Tide Island text engine API
// ============================================================================
//
// The text engine initializes FreeType and HarfBuzz state for later shaping and
// glyph rasterization work.
//
namespace Display_word {

std::expected<void, const char*> init(std::size_t font_size, const char* font_path);

} // namespace Display_word
