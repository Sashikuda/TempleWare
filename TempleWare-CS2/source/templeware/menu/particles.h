#pragma once

#include <vector>
#include <chrono>
#include <random>

#include "../../../external/imgui/imgui.h"
#include "../utils/math/vector/vector.h"
#include "../utils/math/viewmatrix/viewmatrix.h"

// Configuration for the ambient particle system.
// particle_type: 0 = Ash, 1 = Snow, 2 = Rain, 3 = Stars, 4 = Leaves
struct ash_config_t {
    bool  enabled = false;
    int   particle_type = 0;
    int   count = 150;
    float speed = 1.0f;
    float radius = 500.0f;
    float height_min = 0.0f;
    float height_max = 200.0f;
    float wind_x = 0.0f;
    float wind_y = 0.0f;
    float wind_z = 0.0f;
    float turbulence = 0.5f;
    float glow_intensity = 1.0f;

    ImVec4 debris_color = ImVec4(0.60f, 0.60f, 0.60f, 0.70f);
    ImVec4 ember_core   = ImVec4(1.00f, 0.85f, 0.50f, 1.00f);
    ImVec4 ember_glow   = ImVec4(1.00f, 0.45f, 0.10f, 1.00f);
    ImVec4 snow_color   = ImVec4(0.95f, 0.97f, 1.00f, 0.90f);
    ImVec4 rain_color   = ImVec4(0.55f, 0.65f, 0.80f, 0.70f);
    ImVec4 star_color   = ImVec4(1.00f, 1.00f, 0.90f, 1.00f);
    ImVec4 star_glow    = ImVec4(0.80f, 0.85f, 1.00f, 1.00f);
    ImVec4 leaf_color_a = ImVec4(0.80f, 0.45f, 0.15f, 0.90f);
    ImVec4 leaf_color_b = ImVec4(0.70f, 0.25f, 0.10f, 0.90f);
    ImVec4 leaf_color_c = ImVec4(0.85f, 0.65f, 0.20f, 0.90f);
};

struct ash_particle_3d_t {
    Vector_t pos;
    Vector_t vel;
    float size;
    float life;
    float max_life;
    float brightness;
    float stretch;
    float rotation;
    float rot_speed;
    float flicker_phase;
    float turbulence_phase;
    float star_radius;
    float wind_angle;
    float tumble;
    int   type;
};

class c_ash_particles {
public:
    void update_and_draw(ash_config_t& cfg);

    void reset() {
        m_particles.clear();
        m_last_time = {};
        m_elapsed = 0.f;
    }

private:
    std::vector<ash_particle_3d_t> m_particles;
    std::chrono::steady_clock::time_point m_last_time{};
    float m_elapsed{ 0.f };
    int   m_last_particle_type{ -1 };
    std::mt19937 m_rng{ std::random_device{}() };

    ViewMatrix m_viewMatrix{};
    bool m_matrix_initialized{ false };

    float rf(float a, float b) {
        std::uniform_real_distribution<float> d(a, b);
        return d(m_rng);
    }

    void spawn_particle(ash_config_t& cfg, const Vector_t& origin);
};

namespace features {
    inline c_ash_particles particles;
    inline ash_config_t particles_cfg;
}
