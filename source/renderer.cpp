// ============================================================================
// Tide Island renderer backend
// ============================================================================
//
// This translation unit owns the Sokol graphics objects, per-frame draw setup,
// and shader uniform updates for the island surface.
//
#include "renderer.hpp"
#include "island.hpp"
#include "log.hpp"

#include "sokol_gfx.h"
#include "sokol_log.h"
#include "basic.glsl.h"

using namespace std;

// ============================================================================
// [Internal Details]
// ============================================================================

namespace {

// --- Renderer State ---

const Island::Island& island_state = Island::state();
sg_shader shader{};
sg_pipeline pipeline{};
sg_buffer buffer{};

// --- Sokol Frame Descriptors ---

sg_swapchain make_swapchain() {
    sg_swapchain sc{};
    sc.width          = island_state.window_width;
    sc.height         = island_state.window_height;
    sc.sample_count   = 1;
    sc.color_format   = SG_PIXELFORMAT_RGBA8;
    sc.depth_format   = SG_PIXELFORMAT_NONE;
    sc.gl.framebuffer = 0;
    return sc;
}

// --- Uniform Data ---

project_uniform_t update_project_uniform() {
    if (island_state.island_width == 0 || island_state.island_height == 0) {
        logger(Log::Error, "Size of island window should not be zero");
        return{};
    }

    project_uniform_t project_uniform{};
    project_uniform.proj[0]  =  2.0f / island_state.window_width;
    project_uniform.proj[5]  = -2.0f / island_state.window_height;
    project_uniform.proj[10] =  1.0f;
    project_uniform.proj[12] = -1.0f;
    project_uniform.proj[13] =  1.0f;
    project_uniform.proj[15] =  1.0f;
    return project_uniform;
}

radius_uniform_t update_radius_uniform(float x, float y, float width, float height, float radius) {
    if (width == 0 || height == 0) {
        logger(Log::Error, "Size of this component should not be zero");
        return{};
    }

    radius_uniform_t radius_uniform{};

    radius_uniform.center[0] = width / 2 + x;
    radius_uniform.center[1] = height / 2 + y;
    radius_uniform.half_size[0] = width / 2;
    radius_uniform.half_size[1] = height / 2;
    radius_uniform.radius    = radius;
    return radius_uniform;
}

// --- Geometry Generation ---

array<float, 24> generate_vertexes(
    float x, 
    float y, 
    float width, 
    float height, 
    array<float, 4> color
) {
    if (width == 0 || height == 0) {
        logger(Log::Error, "Size of this component should not be zero");
        return{};
    }
    
return {{
        x, y, color[0], color[1], color[2], color[3],
        x + width, y, color[0], color[1], color[2], color[3],
        x, y + height, color[0], color[1], color[2], color[3],
        x + width, y + height, color[0], color[1], color[2], color[3]
    }};
}

} // namespace

// ============================================================================
// [Public API Implementation]
// ============================================================================

expected<void, const char*> Renderer::init() {
    sg_desc sgdesc{};
    sgdesc.logger.func = slog_func;
    sgdesc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    sgdesc.environment.defaults.depth_format = SG_PIXELFORMAT_NONE;
    sgdesc.environment.defaults.sample_count = 1;

    sg_setup(&sgdesc);
    if (!sg_isvalid()) {
        return unexpected("sg_setup failed");
    }

    shader = sg_make_shader(rectangle_shader_desc(sg_query_backend()));
    if (sg_query_shader_state(shader) != SG_RESOURCESTATE_VALID) {
        sg_shutdown();
        return unexpected("sg_make_shader failed");
    }

    sg_pipeline_desc pipeline_desc{};
    pipeline_desc.shader = shader;
    pipeline_desc.layout.attrs[ATTR_rectangle_position].buffer_index = 0;
    pipeline_desc.layout.attrs[ATTR_rectangle_position].format = SG_VERTEXFORMAT_FLOAT2;
    pipeline_desc.layout.attrs[ATTR_rectangle_color].buffer_index = 0;
    pipeline_desc.layout.attrs[ATTR_rectangle_color].format = SG_VERTEXFORMAT_FLOAT4;
    pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
    pipeline_desc.colors[0].blend.enabled = true;
    pipeline_desc.colors[0].blend.src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA;
    pipeline_desc.colors[0].blend.dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.colors[0].blend.op_rgb           = SG_BLENDOP_ADD;
    pipeline_desc.colors[0].blend.src_factor_alpha = SG_BLENDFACTOR_ONE;
    pipeline_desc.colors[0].blend.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    pipeline_desc.colors[0].blend.op_alpha         = SG_BLENDOP_ADD;

    pipeline = sg_make_pipeline(&pipeline_desc);
    if (sg_query_pipeline_state(pipeline) != SG_RESOURCESTATE_VALID) {
        sg_destroy_shader(shader);
        sg_shutdown();
        return unexpected("sg_make_pipeline failed");
    }

    sg_buffer_desc buffer_desc{};
    buffer_desc.size = sizeof(float) * 24 * 10;
    buffer_desc.usage.dynamic_update = true;
    
    buffer = sg_make_buffer(&buffer_desc);
    if (sg_query_buffer_state(buffer) != SG_RESOURCESTATE_VALID) {
        sg_destroy_pipeline(pipeline);
        sg_destroy_shader(shader);
        sg_shutdown();
        return unexpected("sg_make_buffer failed");
    }

    return {};
}

expected<void, const char*> Renderer::draw_rectangle(
    float width, 
    float height, 
    float x, 
    float y, 
    float radius, 
    array<float, 4> color
) {
    array<float, 24> vertexes = generate_vertexes(x, y, width, height, color);

    int vertex_offset =sg_append_buffer(buffer, SG_RANGE(vertexes));

    if (sg_query_buffer_overflow(buffer)) {
        return unexpected("Vertex buffer overflow");
    }
    sg_apply_pipeline(pipeline);

    sg_bindings bindings{};
    bindings.vertex_buffers[0] = buffer;
    bindings.vertex_buffer_offsets[0] = vertex_offset;

    sg_apply_bindings(&bindings);

    project_uniform_t project_uniform = update_project_uniform();
    radius_uniform_t radius_uniform = update_radius_uniform(x, y, width, height, radius);
    sg_apply_uniforms(UB_project_uniform, SG_RANGE(project_uniform));
    sg_apply_uniforms(UB_radius_uniform,  SG_RANGE(radius_uniform));
    sg_draw(0, 4, 1);

    return {};
}

void Renderer::frame() {
    sg_pass_action pass_action{};
    pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action.colors[0].clear_value = {0.0f, 0.0f, 0.0f, 0.0f};
    sg_pass pass{};
    pass.action = pass_action;
    pass.swapchain = make_swapchain();
    sg_begin_pass(&pass);

    Log::check(draw_rectangle(
        island_state.island_width,
        island_state.island_height,
        0.0f,
        island_state.anchor_top,
        island_state.radius,
        island_state.color
    ));

    sg_end_pass();
    sg_commit();

}

void Renderer::shutdown() {
    sg_destroy_buffer(buffer);
    sg_destroy_pipeline(pipeline);
    sg_destroy_shader(shader);
    sg_shutdown();
    logger(Log::Debug, "Sokol graphics context destroyed.");
}
