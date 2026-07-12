// ============================================================================
// Tide Island text shaping backend
// ============================================================================
//
// This translation unit owns the FreeType and HarfBuzz objects used to prepare
// text for display.
//
#include "text_engine.hpp"
#include "log.hpp"

#include <expected>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <cstring>
#include <memory>
#include <vector>
#include <string_view>
#include <unordered_map>

using namespace std;

// ============================================================================
// [Internal Details]
// ============================================================================

namespace {

struct ShapedGlyph {
    uint32_t glyph_id{};

    float x_advance{};
    float y_advance{};

    float x_offset{};
    float y_offset{};
};

struct CachedGlyph {
    uint32_t glyph_id{};

    int width{};
    int height{};

    int bearing_x{};
    int bearing_y{};

    vector<unsigned char> pixels;
};

struct RenderedChar {
    int width{};
    int rows{};
    int pitch{};
    int bitmap_left{};
    int bitmap_top{};
    vector<unsigned char> buffer;
    unsigned char pixel_mode{};
};

template <auto delete_func>
struct DeleteTextEngine {
    void operator()(auto* ptr) const noexcept {
        if (ptr) delete_func(ptr);
    }
};

unique_ptr<FT_LibraryRec_, DeleteTextEngine<FT_Done_FreeType>> ft_library{};
unique_ptr<FT_FaceRec_,    DeleteTextEngine<FT_Done_Face>>     ft_face{};
unique_ptr<hb_font_t,      DeleteTextEngine<hb_font_destroy>>  hb_font{};
unique_ptr<hb_buffer_t,    DeleteTextEngine<hb_buffer_destroy>> hb_buffer{ hb_buffer_create()};
size_t font_size{};
const char* font_path{};

unordered_map<uint32_t, RenderedChar> glyph_cache{};

expected<int, const char*> compact_bitmap_pitch(const FT_Bitmap& bitmap) {
    switch (bitmap.pixel_mode) {
        case FT_PIXEL_MODE_GRAY:
            return static_cast<int>(bitmap.width);

        case FT_PIXEL_MODE_BGRA:
            return static_cast<int>(bitmap.width) * 4;

        case FT_PIXEL_MODE_MONO:
            return static_cast<int>((bitmap.width + 7) / 8);

        default:
            return unexpected("Unsupported glyph bitmap pixel mode");
    }
}

vector<ShapedGlyph> shape_text(string_view text) {
    hb_buffer_clear_contents(hb_buffer.get());

    hb_buffer_add_utf8(
        hb_buffer.get(),
        text.data(),
        static_cast<int>(text.size()),
        0,
        static_cast<int>(text.size())
    );

    hb_buffer_guess_segment_properties(hb_buffer.get());

    hb_shape(hb_font.get(), hb_buffer.get(), nullptr, 0);

    unsigned int glyph_count = 0;

    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(hb_buffer.get(), &glyph_count);

    hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(hb_buffer.get(), &glyph_count);

    vector<ShapedGlyph> result;
    result.reserve(glyph_count);

    for (unsigned int i = 0; i < glyph_count; i++) {
        result.push_back({
            .glyph_id = infos[i].codepoint,

            .x_advance = positions[i].x_advance / 64.0f,
            .y_advance = positions[i].y_advance / 64.0f,

            .x_offset = positions[i].x_offset / 64.0f,
            .y_offset = positions[i].y_offset / 64.0f,
        });
    }

    return result;
}

expected<RenderedChar, const char*> render_char(uint32_t glyph_id, bool cache) {
    if (!ft_face) {
        return unexpected("Failed to initialize FT_Face");
    }

    if (FT_Load_Glyph(ft_face.get(), static_cast<FT_UInt>(glyph_id), FT_LOAD_DEFAULT)) {
        return unexpected("Failed to load glyph");
    }

    if (FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
        return unexpected("Failed to render glyph");
    }

    if (glyph_cache.contains(glyph_id)) return glyph_cache[glyph_id];

    const FT_Bitmap& bitmap = ft_face->glyph->bitmap;
    int compact_pitch = Log::check(compact_bitmap_pitch(bitmap));

    vector<unsigned char> buffer(static_cast<size_t>(bitmap.rows) * compact_pitch);
    if (bitmap.buffer && compact_pitch > 0) {
        for (unsigned int row = 0; row < bitmap.rows; row++) {
            const unsigned char* source_row = bitmap.buffer + (bitmap.pitch >= 0
                ? row * bitmap.pitch : (bitmap.rows - 1 - row) * abs(bitmap.pitch));

            unsigned char* target_row = buffer.data() + row * compact_pitch;
            memcpy(target_row, source_row, compact_pitch);
        }
    }

    RenderedChar rendered_char = {
        .width = static_cast<int>(bitmap.width),
        .rows = static_cast<int>(bitmap.rows),
        .pitch = compact_pitch,
        .bitmap_left = ft_face->glyph->bitmap_left,
        .bitmap_top = ft_face->glyph->bitmap_top,
        .buffer = move(buffer),
        .pixel_mode = bitmap.pixel_mode,
    };

    if (cache) glyph_cache[glyph_id] = rendered_char;

    return rendered_char;
}

} // namespace

// ============================================================================
// [Public API Implementation]
// ============================================================================

expected<void, const char*> Display_word::init(size_t arg_font_size, const char* arg_font_path) {
    font_size = arg_font_size;
    font_path = arg_font_path;

    FT_Library tmp_ft_library{};
    if (FT_Init_FreeType(&tmp_ft_library)) {
        return unexpected("Failed to initialize FT_Library");
    }
    ft_library.reset(tmp_ft_library);

    FT_Face tmp_ft_face{};
    if (FT_New_Face(tmp_ft_library, font_path, 0, &tmp_ft_face)) {
        return unexpected("Failed to initalize FT_Face");
    }
    ft_face.reset(tmp_ft_face);

    FT_Set_Pixel_Sizes(tmp_ft_face, 0, font_size);

    hb_font_t* tmp_hb_font = hb_ft_font_create_referenced(tmp_ft_face);
    if (!tmp_hb_font) {
        return unexpected("Failed to create hb_font");
    }

    hb_font.reset(tmp_hb_font);

    for (auto shaped_glyph : shape_text("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.，。")) {
        Log::check(render_char(shaped_glyph.glyph_id, true));
    }

    return {};
}
