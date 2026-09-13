#include "level_settings_panel.hpp"

#include "engine/core/engine.hpp"
#include "engine/levels/level.hpp"
#include "engine/levels/level_manager.hpp"
#include "engine/rendering/renderer/renderer.hpp"

#include "editor/gui/main_window.hpp"

#include "imgui/imgui.h"

#include <cstring>

using namespace Pulse::Engine;
using Pulse::Engine::Core::GetEngine;

namespace Pulse::Editor::GUI{

    void LevelSettingsPanel::SetParentWindow(Core::EditorMainWindow* parent)
    {
        this->parent = parent;
    }

    void LevelSettingsPanel::Draw()
    {
        if (!ImGui::Begin("Level Settings"))
        {
            ImGui::End();
            return;
        }

        Levels::Level* level = GetEngine().GetLevelManager()->GetLevelAt(0);
        if (!level)
        {
            ImGui::TextDisabled("No level loaded");
            ImGui::End();
            return;
        }

        DrawGeneralCategory();
        DrawRenderingCategory();

        ImGui::End();
    }

    void LevelSettingsPanel::DrawGeneralCategory()
    {
        Levels::Level* level = GetEngine().GetLevelManager()->GetLevelAt(0);

        if (!ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::BeginDisabled();

        char nameBuffer[256];
        strncpy(nameBuffer, level->GetName().c_str(), sizeof(nameBuffer) - 1);
        nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));

        char pathBuffer[512];
        strncpy(pathBuffer, level->GetPath().full.c_str(), sizeof(pathBuffer) - 1);
        pathBuffer[sizeof(pathBuffer) - 1] = '\0';
        ImGui::InputText("Path", pathBuffer, sizeof(pathBuffer));

        ImGui::EndDisabled();
    }

    void LevelSettingsPanel::DrawRenderingCategory()
    {
        Levels::Level* level = GetEngine().GetLevelManager()->GetLevelAt(0);

        if (!ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::Indent();

        // Ambient - see the comment on Level::ambientIntensity/lit.frag's SampleSSAO for the exact
        // semantics : an additive light floor always present, even with zero real lights/DDGI/IBL.
        if (ImGui::TreeNodeEx("Ambient", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::DragFloat("Intensity", &level->ambientIntensity, 0.01f, 0.0f, 10.0f);
            ImGui::TreePop();
        }

        // Screen-space ambient occlusion (see SSAOManager) - the 3 render passes always run regardless
        // of ssaoEnabled (see the field's own comment in level.hpp), so this checkbox is a cheap,
        // instant toggle rather than something that (de)registers passes.
        if (ImGui::TreeNodeEx("Screen-Space Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Checkbox("Enabled", &level->ssaoEnabled);

            if (!level->ssaoEnabled)
                ImGui::BeginDisabled();

            ImGui::DragFloat("Radius", &level->ssaoRadius, 0.01f, 0.01f, 5.0f);
            ImGui::DragFloat("Bias", &level->ssaoBias, 0.001f, 0.0f, 0.5f);
            // Power > 1 darkens occluded areas more aggressively (see ssao.frag) - this is what
            // makes the effect actually read as contact shadowing rather than a faint gray wash.
            ImGui::DragFloat("Power", &level->ssaoPower, 0.05f, 0.5f, 6.0f);
            // Allowed past 1.0 as an amplification knob - SampleSSAO's mix() extrapolates beyond
            // the raw (already power-curved) AO value and clamps the result, so this can push the
            // effect further without re-running the SSAO passes themselves.
            ImGui::DragFloat("Intensity", &level->ssaoIntensity, 0.01f, 0.0f, 3.0f);

            if (!level->ssaoEnabled)
                ImGui::EndDisabled();

            ImGui::TreePop();
        }

        // Global illumination (DDGI) - probeVolume itself keeps driving GI regardless of this checkbox
        // (see ProbeGizmoPass in Renderer::Init()) ; it only hides the per-probe marker gizmos, a pure
        // viewing preference, so it lives on EditorSettings rather than on Level.
        if (parent && ImGui::TreeNodeEx("Global Illumination (DDGI)", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::Checkbox("Show Probe Gizmos", &parent->settings.showProbeGizmos);

            auto probeGizmoPass = GetEngine().GetRenderer()->GetRenderPass("ProbeGizmoPass");
            if (probeGizmoPass)
                probeGizmoPass->enabled = parent->settings.showProbeGizmos;

            ImGui::TreePop();
        }

        ImGui::Unindent();
    }
}
