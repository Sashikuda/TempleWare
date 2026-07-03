#define NOMINMAX
#include "particles.h"

#include <algorithm>
#include <cmath>

#include "../hooks/hooks.h"
#include "../utils/memory/patternscan/patternscan.h"
#include "../utils/memory/gaa/gaa.h"

// Converts an ImVec4 color (0..1) + explicit alpha byte (0..255) to ImU32
static ImU32 col_a(const ImVec4& c, int a) {
    a = std::clamp(a, 0, 255);
    return IM_COL32((int)(c.x * 255.f), (int)(c.y * 255.f), (int)(c.z * 255.f), a);
}

static int alpha_byte(const ImVec4& c) {
    return (int)(c.w * 255.f);
}

void c_ash_particles::spawn_particle(ash_config_t& cfg, const Vector_t& origin) {
    ash_particle_3d_t p{};

    // Spawn in a disc around the local player
    float ang = rf(0.f, 6.28318f);
    float dist = rf(0.f, cfg.radius);

    p.pos.x = origin.x + std::cos(ang) * dist;
    p.pos.y = origin.y + std::sin(ang) * dist;
    p.pos.z = origin.z + rf(cfg.height_min, cfg.height_max);

    p.life = 1.0f;
    p.brightness = rf(0.6f, 1.0f);
    p.flicker_phase = rf(0.f, 6.28f);
    p.turbulence_phase = rf(0.f, 6.28f);
    p.rotation = rf(0.f, 6.28f);
    p.star_radius = 0.f;

    switch (cfg.particle_type)
    {
    case 0: // Ash: debris + embers
    {
        p.type = (rf(0.f, 1.f) < 0.75f) ? 0 : 1;
        if (p.type == 0) {
            p.vel = { rf(-6.f, 6.f), rf(-6.f, 6.f), rf(3.f, 12.f) };
            p.size = rf(1.5f, 4.f);
            p.max_life = rf(5.f, 12.f);
            p.stretch = rf(1.2f, 3.0f);
            p.rot_speed = rf(-3.f, 3.f);
        }
        else {
            p.vel = { rf(-5.f, 5.f), rf(-5.f, 5.f), rf(10.f, 25.f) };
            p.size = rf(1.8f, 4.0f);
            p.max_life = rf(3.f, 7.f);
            p.stretch = 1.f;
            p.rot_speed = 0.f;
        }
        break;
    }
    case 1: // Snow
    {
        p.type = 0;
        p.vel = { rf(-3.f, 3.f), rf(-3.f, 3.f), rf(-15.f, -5.f) };
        p.size = rf(1.5f, 5.f);
        p.max_life = rf(6.f, 14.f);
        p.stretch = 1.f;
        p.rot_speed = rf(-1.f, 1.f);
        break;
    }
    case 2: // Rain
    {
        p.type = 0;
        p.vel = { rf(-2.f, 2.f) + cfg.wind_x * 0.2f, rf(-2.f, 2.f) + cfg.wind_y * 0.2f, rf(-120.f, -80.f) };
        p.size = rf(0.8f, 1.6f);
        p.max_life = rf(0.8f, 2.0f);
        p.stretch = 1.f;
        p.rot_speed = 0.f;
        p.rotation = 0.f;
        break;
    }
    case 3: // Stars
    {
        p.type = 0;
        p.vel = { rf(-0.5f, 0.5f), rf(-0.5f, 0.5f), rf(-0.3f, 0.3f) };
        p.size = rf(1.f, 3.5f);
        p.max_life = rf(12.f, 25.f);
        p.stretch = 1.f;
        p.rot_speed = rf(-0.4f, 0.4f);
        p.star_radius = rf(2.5f, 5.5f);
        break;
    }
    case 4: // Leaves
    {
        p.type = 0;
        p.vel = { rf(-8.f, 8.f) + cfg.wind_x * 0.3f, rf(-8.f, 8.f) + cfg.wind_y * 0.3f, rf(-18.f, -6.f) };
        p.size = rf(4.f, 19.f);
        p.max_life = rf(5.f, 12.f);
        p.stretch = 1.f;
        p.rot_speed = rf(-3.f, 3.f);
        p.wind_angle = rf(0.f, 6.28f);
        p.tumble = rf(0.f, 6.28f);
        break;
    }
    }

    m_particles.push_back(p);
}

void c_ash_particles::update_and_draw(ash_config_t& cfg) {
    if (!cfg.enabled) {
        if (!m_particles.empty()) reset();
        return;
    }

    if (cfg.particle_type != m_last_particle_type) {
        reset();
        m_last_particle_type = cfg.particle_type;
    }

    // Lazily resolve the view matrix (same pattern as Visuals::init)
    if (!m_matrix_initialized) {
        m_viewMatrix.viewMatrix = (viewmatrix_t*)M::getAbsoluteAddress(M::patternScan("client", "48 8D 0D ? ? ? ? 48 C1 E0 06"), 3, 0);
        m_matrix_initialized = true;
    }
    if (!m_viewMatrix.viewMatrix)
        return;

    C_CSPlayerPawn* localPawn = H::oGetLocalPlayer(0);
    if (!localPawn || localPawn->m_iHealth() <= 0)
        return;

    Vector_t player_origin = localPawn->m_vOldOrigin();

    auto now = std::chrono::steady_clock::now();
    if (m_last_time.time_since_epoch().count() == 0) {
        m_last_time = now;
        return;
    }

    float dt = std::chrono::duration<float>(now - m_last_time).count();
    m_last_time = now;
    m_elapsed += dt;
    if (dt > 0.1f) dt = 0.1f;

    while (static_cast<int>(m_particles.size()) < cfg.count)
        spawn_particle(cfg, player_origin);

    auto* dl = ImGui::GetBackgroundDrawList();

    for (int i = static_cast<int>(m_particles.size()) - 1; i >= 0; i--) {
        auto& p = m_particles[i];
        p.life -= dt / p.max_life;

        Vector_t delta = p.pos - player_origin;
        float dist_sq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

        if (p.life <= 0.f || dist_sq > cfg.radius * cfg.radius * 9.f) {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        float tx = std::sin(m_elapsed * 1.3f + p.turbulence_phase) * cfg.turbulence * 10.f;
        float ty = std::cos(m_elapsed * 1.1f + p.turbulence_phase * 0.7f) * cfg.turbulence * 10.f;
        float tz = std::sin(m_elapsed * 0.8f + p.turbulence_phase * 1.3f) * cfg.turbulence * 5.f;
        float gust = std::sin(m_elapsed * 0.25f + p.turbulence_phase * 0.4f) * 0.4f + 1.0f;

        p.pos.x += (p.vel.x + cfg.wind_x * gust + tx) * cfg.speed * dt;
        p.pos.y += (p.vel.y + cfg.wind_y * gust + ty) * cfg.speed * dt;
        p.pos.z += (p.vel.z + cfg.wind_z + tz) * cfg.speed * dt;

        p.rotation += p.rot_speed * dt;

        Vector_t screen;
        if (!m_viewMatrix.WorldToScreen(p.pos, screen))
            continue;

        float dist = std::sqrt(dist_sq);
        float perspective = 250.f / (dist + 60.f);

        float alpha_life = (p.life > 0.9f) ? (1.0f - p.life) * 10.f : (p.life < 0.2f ? p.life / 0.2f : 1.0f);
        float dist_fade = 1.0f - std::clamp(dist / (cfg.radius * 2.5f), 0.f, 1.f);
        dist_fade *= dist_fade;

        float alpha = alpha_life * p.brightness * dist_fade;
        if (alpha < 0.01f) continue;

        ImVec2 sp(screen.x, screen.y);

        switch (cfg.particle_type)
        {
        case 0:
            if (p.type == 0) {
                float sz = std::clamp(p.size * perspective, 0.4f, 8.f);
                float sx = sz * p.stretch, sy = sz;
                float c = std::cos(p.rotation), s = std::sin(p.rotation);
                ImVec2 pts[4];
                float hx = sx * 0.5f, hy = sy * 0.5f;
                pts[0] = { sp.x + (-hx * c - -hy * s), sp.y + (-hx * s + -hy * c) };
                pts[1] = { sp.x + (hx * c - -hy * s), sp.y + (hx * s + -hy * c) };
                pts[2] = { sp.x + (hx * c - hy * s), sp.y + (hx * s + hy * c) };
                pts[3] = { sp.x + (-hx * c - hy * s), sp.y + (-hx * s + hy * c) };
                dl->AddConvexPolyFilled(pts, 4, col_a(cfg.debris_color, (int)(alpha * alpha_byte(cfg.debris_color))));
            }
            else {
                float sz = std::clamp(p.size * perspective * 0.7f, 0.3f, 6.f);
                float flicker = 0.6f + 0.4f * std::sin(m_elapsed * 10.f + p.flicker_phase);
                float ea = alpha * flicker;
                if (ea * 0.3f * cfg.glow_intensity > 0.01f)
                    dl->AddCircleFilled(sp, sz * 4.f, col_a(cfg.ember_glow, (int)(ea * 0.3f * cfg.glow_intensity * 100.f)), 8);
                dl->AddCircleFilled(sp, sz * 2.f, col_a(cfg.ember_glow, (int)(ea * 120.f)), 8);
                dl->AddCircleFilled(sp, sz, col_a(cfg.ember_core, (int)(std::min(ea * cfg.glow_intensity, 1.f) * 255.f)), 6);
            }
            break;
        case 1:
        {
            float sz = std::clamp(p.size * perspective, 0.4f, 9.f);
            int base_a = (int)(alpha * alpha_byte(cfg.snow_color));
            dl->AddCircleFilled(sp, sz * 2.2f, col_a(cfg.snow_color, base_a / 6), 12);
            dl->AddCircleFilled(sp, sz * 1.4f, col_a(cfg.snow_color, base_a / 3), 12);
            dl->AddCircleFilled(sp, sz, col_a(cfg.snow_color, base_a), 10);
            break;
        }
        case 2:
        {
            float sz = std::clamp(p.size * perspective, 0.4f, 4.f);
            Vector_t tail_world = p.pos - p.vel * (cfg.speed * 0.035f);
            Vector_t tail_screen;
            if (!m_viewMatrix.WorldToScreen(tail_world, tail_screen)) continue;
            ImVec2 tail(tail_screen.x, tail_screen.y);

            float dx = sp.x - tail.x, dy = sp.y - tail.y;
            float slen = std::sqrt(dx * dx + dy * dy);
            if (slen > 40.f) { float scale = 40.f / slen; tail = { sp.x - dx * scale, sp.y - dy * scale }; }
            if (slen < 1.0f) continue;

            int ra = (int)(alpha * alpha_byte(cfg.rain_color));

            dl->AddLine(tail, sp, col_a(cfg.rain_color, ra / 8), sz * 3.0f);
            dl->AddLine(tail, sp, col_a(cfg.rain_color, ra / 2), sz * 1.5f);
            dl->AddLine(tail, sp, IM_COL32(255, 255, 255, (int)(alpha * 200)), std::max(sz * 0.4f, 0.5f));

            // Splash ring near the ground
            if (p.pos.z < player_origin.z - 20.f && rf(0.f, 1.f) < 0.1f) {
                dl->AddCircle(sp, sz * 3.f * (1.0f - p.life), IM_COL32(200, 220, 255, ra / 4), 8, 1.0f);
            }
            break;
        }
        case 3:
        {
            float sz = std::clamp(p.size * perspective, 0.5f, 7.f);
            float twinkle = 0.55f + 0.45f * std::sin(m_elapsed * 2.5f + p.flicker_phase);
            float ea = alpha * twinkle * cfg.glow_intensity;
            if (ea < 0.02f) continue;
            int sa = (int)(std::min(ea, 1.f) * 255.f);
            float gr = std::min(sz * 5.5f * p.star_radius * 0.35f, 28.f);
            if (gr > 1.f) {
                dl->AddCircleFilled(sp, gr, col_a(cfg.star_glow, (int)(ea * 0.10f * alpha_byte(cfg.star_glow))), 16);
                dl->AddCircleFilled(sp, gr * 0.5f, col_a(cfg.star_glow, (int)(ea * 0.22f * alpha_byte(cfg.star_glow))), 12);
                dl->AddCircleFilled(sp, gr * 0.2f, col_a(cfg.star_color, (int)(ea * 0.42f * alpha_byte(cfg.star_glow))), 8);
            }
            constexpr int PTS = 10; constexpr float TWO_PI = 6.28318530f;
            float outer_r = sz * 2.0f, inner_r = sz * 0.80f, base_a = p.rotation - (TWO_PI / 4.f);
            ImVec2 v[PTS];
            for (int k = 0; k < PTS; ++k) {
                float r = (k % 2 == 0) ? outer_r : inner_r;
                float a = base_a + (TWO_PI * k) / PTS;
                v[k] = { sp.x + std::cos(a) * r, sp.y + std::sin(a) * r };
            }
            ImU32 col = col_a(cfg.star_color, sa);
            if (sz > 1.5f) {
                ImVec2 gv[PTS];
                for (int k = 0; k < PTS; ++k) {
                    float r = ((k % 2 == 0) ? outer_r : inner_r) * 1.4f;
                    float a = base_a + (TWO_PI * k) / PTS;
                    gv[k] = { sp.x + std::cos(a) * r, sp.y + std::sin(a) * r };
                }
                for (int k = 0; k < PTS; ++k)
                    dl->AddTriangleFilled(sp, gv[k], gv[(k + 1) % PTS], col_a(cfg.star_color, sa / 5));
            }
            for (int k = 0; k < PTS; ++k)
                dl->AddTriangleFilled(sp, v[k], v[(k + 1) % PTS], col);
            dl->AddCircleFilled(sp, sz * 0.4f, IM_COL32(255, 255, 255, sa), 5);
            break;
        }
        case 4:
        {
            float sz = 15.f * perspective;
            p.tumble += p.rot_speed * dt;
            float tumble_scale = std::abs(std::cos(p.tumble));
            float sx = sz, sy = sz * (0.15f + 0.85f * tumble_scale);
            const ImVec4& lc = (p.flicker_phase / 6.28f < 0.33f) ? cfg.leaf_color_a : (p.flicker_phase / 6.28f < 0.66f ? cfg.leaf_color_b : cfg.leaf_color_c);
            float c = std::cos(p.rotation), s = std::sin(p.rotation);
            float hx = sx * 0.5f, hy = sy * 0.5f;
            ImVec2 pts[4];
            pts[0] = { sp.x + (-hx * c - -hy * s), sp.y + (-hx * s + -hy * c) };
            pts[1] = { sp.x + (hx * c - -hy * s), sp.y + (hx * s + -hy * c) };
            pts[2] = { sp.x + (hx * c - hy * s), sp.y + (hx * s + hy * c) };
            pts[3] = { sp.x + (-hx * c - hy * s), sp.y + (-hx * s + hy * c) };
            dl->AddConvexPolyFilled(pts, 4, col_a(lc, (int)(alpha * alpha_byte(lc))));
            break;
        }
        }
    }
}
