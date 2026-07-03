#include "menu.h"
#include "../config/config.h"
#include "particles.h"
#include "../features/world/skybox/skybox.h"

#include <iostream>
#include <vector>
#include "../config/configmanager.h"

#include "../keybinds/keybinds.h"

#include "../utils/logging/log.h"

void ApplyImGuiTheme() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // REDLINE theme - white / grey / black
    ImVec4 primaryColor = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);   // light grey
    ImVec4 outlineColor = ImVec4(0.45f, 0.45f, 0.45f, 0.7f);   // mid grey outline

    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.0f);

    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);

    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
    colors[ImGuiCol_SliderGrab] = primaryColor;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);

    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.0f);

    colors[ImGuiCol_Separator] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = primaryColor;
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

    colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.8f);
    colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);

    colors[ImGuiCol_Border] = outlineColor;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupRounding = 0.0f;

    style.ItemSpacing = ImVec2(8, 4);
    style.FramePadding = ImVec2(4, 3);
    style.WindowPadding = ImVec2(8, 8);

    style.FrameBorderSize = 1.0f;
    style.TabBorderSize = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;

    style.GrabMinSize = 7.0f;
}

Menu::Menu() {
    activeTab = 0;
    showMenu = true;
}

void Menu::init(HWND& window, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11RenderTargetView* mainRenderTargetView) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);

    ApplyImGuiTheme();

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f);

    std::cout << "initialized menu\n";
}

void Menu::render() {
    keybind.pollInputs();
    if (showMenu) {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar;

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once);

        ImGui::Begin("REDLINE | Internal", nullptr, window_flags);

        {
            float windowWidth = ImGui::GetWindowWidth();
            float rightTextWidth = ImGui::CalcTextSize("Internal").x;

            ImGui::Text("REDLINE");

            ImGui::SameLine(windowWidth - rightTextWidth - 10);
            ImGui::Text("Internal");
        }

        ImGui::Separator();

        const char* tabNames[] = { "Aim", "Visuals", "Misc", "Config" };

        if (ImGui::BeginTabBar("MainTabBar", ImGuiTabBarFlags_NoTooltip)) {
            for (int i = 0; i < 4; i++) {
                if (ImGui::BeginTabItem(tabNames[i])) {
                    activeTab = i;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::BeginChild("ContentRegion", ImVec2(0, 0), false);

        switch (activeTab) {
        case 0:
        {
            ImGui::BeginChild("AimLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 0), true);
            ImGui::Text("General");
            ImGui::Separator();

            ImGui::Checkbox("Enable##AimBot", &Config::aimbot);
            ImGui::SameLine();
            ImGui::Text("Key:");
            ImGui::SameLine();
            keybind.menuButton(Config::aimbot);

            ImGui::Checkbox("Team Check", &Config::team_check);
            ImGui::SliderFloat("FOV", &Config::aimbot_fov, 0.f, 90.f);
            ImGui::Checkbox("Draw FOV Circle", &Config::fov_circle);
            if (Config::fov_circle) {
                ImGui::ColorEdit4("Circle Color##FovColor", (float*)&Config::fovCircleColor);
            }
            ImGui::Checkbox("Recoil Control", &Config::rcs);
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("AimRight", ImVec2(0, 0), true);
            ImGui::Text("TriggerBot");
            ImGui::Separator();
            ImGui::Text("No additional settings");

            ImGui::EndChild();
        }
        break;

        case 1:
        {
            ImGui::BeginChild("VisualsLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 0), true);
            ImGui::Text("Player ESP");
            ImGui::Separator();

            ImGui::Checkbox("Box", &Config::esp);
            ImGui::SliderFloat("Thickness", &Config::espThickness, 1.0f, 5.0f);
            ImGui::Checkbox("Box Fill", &Config::espFill);
            if (Config::espFill) {
                ImGui::SliderFloat("Fill Opacity", &Config::espFillOpacity, 0.0f, 1.0f);
            }
            ImGui::ColorEdit4("ESP Color##BoxColor", (float*)&Config::espColor);
            ImGui::Checkbox("Team Check", &Config::teamCheck);
            ImGui::Checkbox("Health Bar", &Config::showHealth);
            ImGui::Checkbox("Name Tags", &Config::showNameTags);

            ImGui::Spacing();
            ImGui::Text("World");
            ImGui::Separator();

            ImGui::Checkbox("Night Mode", &Config::Night);
            if (Config::Night) {
                ImGui::ColorEdit4("Night Color", (float*)&Config::NightColor);
            }

            ImGui::Checkbox("World Color", &Config::modulateWorld);
            if (Config::modulateWorld) {
                ImGui::ColorEdit4("World Color##WorldColor", (float*)&Config::worldColor);
            }

            ImGui::Checkbox("Smoke Color", &Config::modulateSmoke);
            if (Config::modulateSmoke) {
                ImGui::ColorEdit4("Smoke Color##SmokeColor", (float*)&Config::smokeColor);
            }

            ImGui::Checkbox("Custom FOV", &Config::fovEnabled);
            if (Config::fovEnabled) {
                ImGui::SliderFloat("FOV Value##FovSlider", &Config::fov, 20.0f, 160.0f, "%1.0f");
            }

            ImGui::Spacing();
            ImGui::Text("Particles");
            ImGui::Separator();

            ash_config_t& pcfg = features::particles_cfg;
            ImGui::Checkbox("Enable##Particles", &pcfg.enabled);
            if (pcfg.enabled) {
                const char* particleTypes[] = { "Ash", "Snow", "Rain", "Stars", "Leaves" };
                ImGui::Combo("Type##Particles", &pcfg.particle_type, particleTypes, IM_ARRAYSIZE(particleTypes));
                ImGui::SliderInt("Count##Particles", &pcfg.count, 10, 500);
                ImGui::SliderFloat("Speed##Particles", &pcfg.speed, 0.1f, 3.0f, "%.1f");
                ImGui::SliderFloat("Radius##Particles", &pcfg.radius, 100.0f, 2000.0f, "%1.0f");
                ImGui::SliderFloat("Turbulence##Particles", &pcfg.turbulence, 0.0f, 2.0f, "%.1f");
                ImGui::SliderFloat("Wind X##Particles", &pcfg.wind_x, -30.0f, 30.0f, "%1.0f");
                ImGui::SliderFloat("Wind Y##Particles", &pcfg.wind_y, -30.0f, 30.0f, "%1.0f");

                switch (pcfg.particle_type) {
                case 0:
                    ImGui::SliderFloat("Glow##Particles", &pcfg.glow_intensity, 0.0f, 2.0f, "%.1f");
                    ImGui::ColorEdit4("Debris##Particles", (float*)&pcfg.debris_color);
                    ImGui::ColorEdit4("Ember Core##Particles", (float*)&pcfg.ember_core);
                    ImGui::ColorEdit4("Ember Glow##Particles", (float*)&pcfg.ember_glow);
                    break;
                case 1:
                    ImGui::ColorEdit4("Snow Color##Particles", (float*)&pcfg.snow_color);
                    break;
                case 2:
                    ImGui::ColorEdit4("Rain Color##Particles", (float*)&pcfg.rain_color);
                    break;
                case 3:
                    ImGui::SliderFloat("Glow##Particles", &pcfg.glow_intensity, 0.0f, 2.0f, "%.1f");
                    ImGui::ColorEdit4("Star Color##Particles", (float*)&pcfg.star_color);
                    ImGui::ColorEdit4("Star Glow##Particles", (float*)&pcfg.star_glow);
                    break;
                case 4:
                    ImGui::ColorEdit4("Leaf A##Particles", (float*)&pcfg.leaf_color_a);
                    ImGui::ColorEdit4("Leaf B##Particles", (float*)&pcfg.leaf_color_b);
                    ImGui::ColorEdit4("Leaf C##Particles", (float*)&pcfg.leaf_color_c);
                    break;
                }
            }

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("VisualsRight", ImVec2(0, 0), true);
            ImGui::Text("Chams");
            ImGui::Separator();

            ImGui::Checkbox("Chams##ChamsCheckbox", &Config::enemyChams);
            const char* chamsMaterials[] = { "Flat", "Illuminate", "Glow" };
            ImGui::Combo("Material", &Config::chamsMaterial, chamsMaterials, IM_ARRAYSIZE(chamsMaterials));
            if (Config::enemyChams) {
                ImGui::ColorEdit4("Chams Color##ChamsColor", (float*)&Config::colVisualChams);
            }
            ImGui::Checkbox("Chams-XQZ", &Config::enemyChamsInvisible);
            if (Config::enemyChamsInvisible) {
                ImGui::ColorEdit4("XQZ Color##ChamsXQZColor", (float*)&Config::colVisualChamsIgnoreZ);
            }

            ImGui::Spacing();
            ImGui::Text("Hand Chams");
            ImGui::Separator();

            ImGui::Checkbox("Hand Chams", &Config::armChams);
            if (Config::armChams) {
                ImGui::ColorEdit4("Hand Color##HandChamsColor", (float*)&Config::colArmChams);
            }
            ImGui::Checkbox("Viewmodel Chams", &Config::viewmodelChams);
            if (Config::viewmodelChams) {
                ImGui::ColorEdit4("Viewmodel Color##ViewModelChamsColor", (float*)&Config::colViewmodelChams);
            }

            ImGui::Spacing();
            ImGui::Text("Removals");
            ImGui::Separator();

            ImGui::Checkbox("Anti Flash", &Config::antiflash);

            ImGui::EndChild();
        }
        break;

        case 2:
        {
            ImGui::BeginChild("MiscLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 0), true);
            ImGui::Text("Movement");
            ImGui::Separator();

            ImGui::Checkbox("Bhop", &Config::bhop);

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("MiscRight", ImVec2(0, 0), true);
            ImGui::Text("Skybox Changer");
            ImGui::Separator();

            {
                sky_config_t& scfg = features::skybox_cfg;

                ImGui::Combo("Map##Skybox", &scfg.selected_map, skybox::map_names(), skybox::map_count());

                ImGui::Checkbox("Override Tint##Skybox", &scfg.override_tint);
                if (scfg.override_tint) {
                    ImGui::ColorEdit4("Tint##Skybox", (float*)&scfg.tint);
                }

                ImGui::Checkbox("Override Brightness##Skybox", &scfg.override_brightness);
                if (scfg.override_brightness) {
                    ImGui::SliderFloat("Brightness##Skybox", &scfg.brightness, 0.0f, 5.0f, "%.2f");
                }

                ImGui::Spacing();

                if (ImGui::Button("Apply##Skybox")) {
                    skybox::request_apply();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset##Skybox")) {
                    skybox::request_reset();
                }

                ImGui::Spacing();
                ImGui::TextWrapped("Status: %s", skybox::status());
            }

            ImGui::EndChild();
        }
        break;

        case 3:
        {
            ImGui::BeginChild("ConfigLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 0), true);
            ImGui::Text("General");
            ImGui::Separator();

            static char configName[128] = "";
            static std::vector<std::string> configList = internal_config::ConfigManager::ListConfigs();
            static int selectedConfigIndex = -1;

            ImGui::InputText("Config Name", configName, IM_ARRAYSIZE(configName));

            if (ImGui::Button("Refresh")) {
                configList = internal_config::ConfigManager::ListConfigs();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                internal_config::ConfigManager::Load(configName);
            }
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                internal_config::ConfigManager::Save(configName);
                configList = internal_config::ConfigManager::ListConfigs();
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                internal_config::ConfigManager::Remove(configName);
                configList = internal_config::ConfigManager::ListConfigs();
            }

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("ConfigRight", ImVec2(0, 0), true);
            ImGui::Text("Saved Configs");
            ImGui::Separator();

            for (int i = 0; i < static_cast<int>(configList.size()); i++) {
                if (ImGui::Selectable(configList[i].c_str(), selectedConfigIndex == i)) {
                    selectedConfigIndex = i;
                    strncpy_s(configName, sizeof(configName), configList[i].c_str(), _TRUNCATE);
                }
            }

            ImGui::EndChild();
        }
        break;
        }

        ImGui::EndChild();
        ImGui::End();
    }
}

void Menu::toggleMenu() {
    showMenu = !showMenu;
}
