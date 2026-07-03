#pragma once
#pragma once

#include <vector>

#include <chrono>

#include <random>

#include <algorithm>

#include "../sdk/structs.h"

#include "../sdk/globals.h"

#include "../imgui/imgui.h"



struct ash_particle_3d_t {

    Vector3 pos;

    Vector3 vel;

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



    float rf(float a, float b) {

        std::uniform_real_distribution<float> d(a, b);

        return d(m_rng);

    }



    void spawn_particle(ash_config_t& cfg, const Vector3& origin, const Vector3& forward);

};



namespace features {

    inline c_ash_particles particles;

}



[/ ICODE]



particles.cpp

[ICODE]

#define NOMINMAX

#include "particles.h"

#include "../sdk/offsets.h"

#include <algorithm>



void c_ash_particles::spawn_particle(ash_config_t& cfg, const Vector3& origin, const Vector3& forward) {

    ash_particle_3d_t p{};





    Vector3 side = { -forward.y, forward.x, 0.f };





    float fwdDist = rf(-cfg.radius * 0.2f, cfg.radius * 1.8f);

    float sideDist = rf(-cfg.radius, cfg.radius) * (fwdDist / (cfg.radius * 1.8f) + 0.2f);



    p.pos.x = origin.x + forward.x * fwdDist + side.x * sideDist;

    p.pos.y = origin.y + forward.y * fwdDist + side.y * sideDist;

    p.pos.z = origin.z + rf(cfg.height_min, cfg.height_max);



    p.life = 1.0f;

    p.brightness = rf(0.6f, 1.0f);

    p.flicker_phase = rf(0.f, 6.28f);

    p.turbulence_phase = rf(0.f, 6.28f);

    p.rotation = rf(0.f, 6.28f);

    p.star_radius = 0.f;



    switch (cfg.particle_type)

    {

    case 0:

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

    case 1:

    {

        p.type = 0;

        p.vel = { rf(-3.f, 3.f), rf(-3.f, 3.f), rf(-15.f, -5.f) };

        p.size = rf(1.5f, 5.f);

        p.max_life = rf(6.f, 14.f);

        p.stretch = 1.f;

        p.rot_speed = rf(-1.f, 1.f);

        break;

    }

    case 2:

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

    case 3:

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

    case 4:

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



    uintptr_t clientBase = (uintptr_t)GetModuleHandleA("client.dll");

    if (!clientBase) return;



    uintptr_t localPawn = *(uintptr_t*)(clientBase + offsets::dwLocalPlayerPawn);

    if (!localPawn) return;



    int health = *(int*)(localPawn + offsets::m_iHealth);

    if (health <= 0) return;



    uintptr_t sceneNode = *(uintptr_t*)(localPawn + offsets::m_pGameSceneNode);

    if (!sceneNode) return;



    Vector3 player_origin = *(Vector3*)(sceneNode + offsets::m_vecAbsOrigin);

    Vector3 viewAngles = *(Vector3*)(localPawn + offsets::m_angEyeAngles);



    view_matrix_t viewMatrix;

    memcpy(&viewMatrix, (void*)(clientBase + offsets::dwViewMatrix), sizeof(viewMatrix));



    auto now = std::chrono::steady_clock::now();

    if (m_last_time.time_since_epoch().count() == 0) {

        m_last_time = now;

        return;

    }



    float dt = std::chrono::duration<float>(now - m_last_time).count();

    m_last_time = now;

    m_elapsed += dt;

    if (dt > 0.1f) dt = 0.1f;





    float yaw = viewAngles.y * (3.14159265f / 180.f);

    Vector3 forward = { std::cos(yaw), std::sin(yaw), 0.f };



    while (static_cast<int>(m_particles.size()) < cfg.count)

        spawn_particle(cfg, player_origin, forward);



    auto* dl = ImGui::GetBackgroundDrawList();

    ImVec2 ds = ImGui::GetIO().DisplaySize;



    for (int i = static_cast<int>(m_particles.size()) - 1; i >= 0; i--) {

        auto& p = m_particles[i];

        p.life -= dt / p.max_life;



        Vector3 delta = p.pos - player_origin;

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



        Vector2 screen;

        if (!WorldToScreen(p.pos, screen, viewMatrix, (int)ds.x, (int)ds.y))

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

                float sz = (std::clamp)(p.size * perspective, 0.4f, 8.f);

                float sx = sz * p.stretch, sy = sz;

                float c = std::cos(p.rotation), s = std::sin(p.rotation);

                ImVec2 pts[4];

                float hx = sx * 0.5f, hy = sy * 0.5f;

                pts[0] = { sp.x + (-hx * c - -hy * s), sp.y + (-hx * s + -hy * c) };

                pts[1] = { sp.x + (hx * c - -hy * s), sp.y + (hx * s + -hy * c) };

                pts[2] = { sp.x + (hx * c - hy * s), sp.y + (hx * s + hy * c) };

                pts[3] = { sp.x + (-hx * c - hy * s), sp.y + (-hx * s + hy * c) };

                dl->AddConvexPolyFilled(pts, 4, IM_COL32(cfg.debris_color.r, cfg.debris_color.g, cfg.debris_color.b, (int)(alpha * cfg.debris_color.a)));

            }
            else {

                float sz = (std::clamp)(p.size * perspective * 0.7f, 0.3f, 6.f);

                float flicker = 0.6f + 0.4f * std::sin(m_elapsed * 10.f + p.flicker_phase);

                float ea = alpha * flicker;

                if (ea * 0.3f * cfg.glow_intensity > 0.01f)

                    dl->AddCircleFilled(sp, sz * 4.f, IM_COL32(cfg.ember_glow.r, cfg.ember_glow.g, cfg.ember_glow.b, (int)(ea * 0.3f * cfg.glow_intensity * 100.f)), 8);

                dl->AddCircleFilled(sp, sz * 2.f, IM_COL32(cfg.ember_glow.r, cfg.ember_glow.g, cfg.ember_glow.b, (int)(ea * 120.f)), 8);

                dl->AddCircleFilled(sp, sz, IM_COL32(cfg.ember_core.r, cfg.ember_core.g, cfg.ember_core.b, (int)((std::min)(ea * cfg.glow_intensity, 1.f) * 255.f)), 6);

            }

            break;

        case 1:

        {

            float sz = (std::clamp)(p.size * perspective, 0.4f, 9.f);

            int base_a = (int)(alpha * cfg.snow_color.a);

            dl->AddCircleFilled(sp, sz * 2.2f, IM_COL32(cfg.snow_color.r, cfg.snow_color.g, cfg.snow_color.b, base_a / 6), 12);

            dl->AddCircleFilled(sp, sz * 1.4f, IM_COL32(cfg.snow_color.r, cfg.snow_color.g, cfg.snow_color.b, base_a / 3), 12);

            dl->AddCircleFilled(sp, sz, IM_COL32(cfg.snow_color.r, cfg.snow_color.g, cfg.snow_color.b, base_a), 10);

            break;

        }

        case 2:

        {

            float sz = (std::clamp)(p.size * perspective, 0.4f, 4.f);

            Vector3 tail_world = p.pos - p.vel * (cfg.speed * 0.035f);

            Vector2 tail_screen;

            if (!WorldToScreen(tail_world, tail_screen, viewMatrix, (int)ds.x, (int)ds.y)) continue;

            ImVec2 tail(tail_screen.x, tail_screen.y);



            float dx = sp.x - tail.x, dy = sp.y - tail.y;

            float slen = std::sqrt(dx * dx + dy * dy);

            if (slen > 40.f) { float scale = 40.f / slen; tail = { sp.x - dx * scale, sp.y - dy * scale }; }

            if (slen < 1.0f) continue;



            int ra = (int)(alpha * cfg.rain_color.a);



            dl->AddLine(tail, sp, IM_COL32(cfg.rain_color.r, cfg.rain_color.g, cfg.rain_color.b, ra / 8), sz * 3.0f);



            dl->AddLine(tail, sp, IM_COL32(cfg.rain_color.r, cfg.rain_color.g, cfg.rain_color.b, ra / 2), sz * 1.5f);



            dl->AddLine(tail, sp, IM_COL32(255, 255, 255, (int)(alpha * 200)), (std::max)(sz * 0.4f, 0.5f));



            if (p.pos.z < player_origin.z - 20.f && rf(0.f, 1.f) < 0.1f) {

                dl->AddCircle(sp, sz * 3.f * (1.0f - p.life), IM_COL32(200, 220, 255, ra / 4), 8, 1.0f);

            }

            break;

        }

        case 3:

        {

            float sz = (std::clamp)(p.size * perspective, 0.5f, 7.f);

            float twinkle = 0.55f + 0.45f * std::sin(m_elapsed * 2.5f + p.flicker_phase);

            float ea = alpha * twinkle * cfg.glow_intensity;

            if (ea < 0.02f) continue;

            int sa = (int)((std::min)(ea, 1.f) * 255.f);

            float gr = (std::min)(sz * 5.5f * p.star_radius * 0.35f, 28.f);

            if (gr > 1.f) {

                dl->AddCircleFilled(sp, gr, IM_COL32(cfg.star_glow.r, cfg.star_glow.g, cfg.star_glow.b, (int)(ea * 0.10f * cfg.star_glow.a)), 16);

                dl->AddCircleFilled(sp, gr * 0.5f, IM_COL32(cfg.star_glow.r, cfg.star_glow.g, cfg.star_glow.b, (int)(ea * 0.22f * cfg.star_glow.a)), 12);

                dl->AddCircleFilled(sp, gr * 0.2f, IM_COL32(cfg.star_color.r, cfg.star_color.g, cfg.star_color.b, (int)(ea * 0.42f * cfg.star_glow.a)), 8);

            }

            constexpr int PTS = 10; constexpr float TWO_PI = 6.28318530f;

            float outer_r = sz * 2.0f, inner_r = sz * 0.80f, base_a = p.rotation - (TWO_PI / 4.f);

            ImVec2 v[PTS]; for (int k = 0; k < PTS; ++k) { float r = (k % 2 == 0) ? outer_r : inner_r; float ang = base_a + (TWO_PI * k) / PTS; v[k] = { sp.x + std::cos(ang) * r, sp.y + std::sin(ang) * r }; }

            ImU32 col = IM_COL32(cfg.star_color.r, cfg.star_color.g, cfg.star_color.b, sa);

            if (sz > 1.5f) {

                ImVec2 gv[PTS]; for (int k = 0; k < PTS; ++k) { float r = ((k % 2 == 0) ? outer_r : inner_r) * 1.4f; float ang = base_a + (TWO_PI * k) / PTS; gv[k] = { sp.x + std::cos(ang) * r, sp.y + std::sin(ang) * r }; }

                for (int k = 0; k < PTS; ++k) dl->AddTriangleFilled(sp, gv[k], gv[(k + 1) % PTS], IM_COL32(cfg.star_color.r, cfg.star_color.g, cfg.star_color.b, sa / 5));

            }

            for (int k = 0; k < PTS; ++k) dl->AddTriangleFilled(sp, v[k], v[(k + 1) % PTS], col);

            dl->AddCircleFilled(sp, sz * 0.4f, IM_COL32(255, 255, 255, sa), 5);

            break;

        }

        case 4:

        {

            float sz = 15.f * perspective;

            p.tumble += p.rot_speed * dt;

            float tumble_scale = std::abs(std::cos(p.tumble));

            float sx = sz, sy = sz * (0.15f + 0.85f * tumble_scale);

            ash_color_t lc = (p.flicker_phase / 6.28f < 0.33f) ? cfg.leaf_color_a : (p.flicker_phase / 6.28f < 0.66f ? cfg.leaf_color_b : cfg.leaf_color_c);

            float c = std::cos(p.rotation), s = std::sin(p.rotation);

            float hx = sx * 0.5f, hy = sy * 0.5f;

            ImVec2 pts[4];

            pts[0] = { sp.x + (-hx * c - -hy * s), sp.y + (-hx * s + -hy * c) };

            pts[1] = { sp.x + (hx * c - -hy * s), sp.y + (hx * s + -hy * c) };

            pts[2] = { sp.x + (hx * c - hy * s), sp.y + (hx * s + hy * c) };

            pts[3] = { sp.x + (-hx * c - hy * s), sp.y + (-hx * s + hy * c) };

            dl->AddConvexPolyFilled(pts, 4, IM_COL32(lc.r, lc.g, lc.b, (int)(alpha * lc.a)));

            break;

        }

        }

    }

}