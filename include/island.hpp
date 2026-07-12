#pragma once

#include <array>

// ============================================================================
// Tide Island shared state API
// ============================================================================
//
// The island state is intentionally small and process-wide. Platform and render
// backends read this state while public setters validate updates.
//
namespace Island {

struct Island {
    std::array<float, 4> color = {0, 0, 0, 1};
    int window_width{};
    int window_height{};
    int zone{-1};
    float island_width{};
    float island_height{};
    float anchor_top{};
    float radius{};
    bool is_running{true};
    bool geometry_dirty{};
};

const Island& state();
void set_anchor_top(float distance);
void set_island_size(float width, float height);
void set_window_size(int width, int height);
void set_is_running(bool state);
void set_radius(float radius);
void set_zone(int zone);

} // namespace Island
