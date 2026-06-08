#include <imgui.h>
#include "DebugState.h"

extern "C" __declspec(dllexport) void DrawDebugUI(ImGuiContext* ctx, DebugState* state) {
    if (!ctx || !state) return;
    ImGui::SetCurrentContext(ctx);

    // Unify allocator functions to prevent heap corruption assertion
    if (state->allocFunc && state->freeFunc) {
        ImGui::SetAllocatorFunctions(
            (ImGuiMemAllocFunc)state->allocFunc,
            (ImGuiMemFreeFunc)state->freeFunc,
            state->allocUserData
        );
    }

    ImGui::Begin("Game Debug Dynamic Panel (Hot-Reloadable)");

    // Show compiled time to verify hot-reload works
    ImGui::Text("DLL Compiled Time: " __DATE__ " " __TIME__);
    ImGui::Separator();

    // Player Controls
    if (ImGui::CollapsingHeader("Player Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Player Pos", state->playerPos, 0.1f);
        ImGui::DragFloat3("Player Rot", state->playerRot, 0.05f);
        ImGui::ColorEdit4("Player Color", state->playerColor);

        ImGui::Text("Thread Nodes: %d", *(state->threadNodeCount));
        if (ImGui::TreeNode("Thread Node Coordinates")) {
            int nodeCount = *(state->threadNodeCount);
            for (int i = 0; i < nodeCount; i++) {
                ImGui::Text("[%d] X: %.2f, Y: %.2f, Z: %.2f", i, 
                    state->threadNodes[i * 3 + 0],
                    state->threadNodes[i * 3 + 1],
                    state->threadNodes[i * 3 + 2]);
            }
            ImGui::TreePop();
        }
    }

    // Enemy Controls
    if (ImGui::CollapsingHeader("Enemy Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        int enemyCount = *(state->enemyCount);
        ImGui::Text("Enemies Count: %d", enemyCount);
        for (int i = 0; i < enemyCount; i++) {
            ImGui::PushID(i);
            ImGui::Text("Enemy %d Details", i);
            ImGui::DragFloat3("Pos", &state->enemyPositions[i * 3], 0.1f);
            ImGui::DragFloat3("Rot", &state->enemyRotations[i * 3], 0.05f);
            ImGui::SliderFloat("HP", &state->enemyHPs[i], 0.0f, 100.0f);
            ImGui::PopID();
        }
    }

    // Replay / Timeline Controls
    if (ImGui::CollapsingHeader("Replay / Timeline Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Replay Playback Mode", state->isPlayback);
        if (*(state->isPlayback)) {
            if (*(state->totalFrames) > 0) {
                ImGui::SliderInt("Playback Frame", state->playbackFrame, 0, *(state->totalFrames) - 1);
            } else {
                ImGui::Text("No frames recorded yet.");
            }
        } else {
            ImGui::Text("Recording... Current Frame Count: %d", *(state->totalFrames));
        }

        if (ImGui::Button("Save Replay Log JSON")) {
            *(state->saveReplayNow) = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Replay Log JSON")) {
            *(state->loadReplayNow) = true;
        }
    }

    // Socket status
    if (ImGui::CollapsingHeader("Blender Sync (TCP)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Socket Sync", state->isSocketSyncEnabled);
        ImGui::Text("Server Status: %s", *(state->isSocketConnected) ? "CONNECTED" : "LISTENING/IDLE");
        ImGui::Text("Server Port: %d", *(state->socketPort));
    }

    // Bug triggers
    if (ImGui::CollapsingHeader("Bug Event Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Bug Triggered Flag", state->isBugTriggered);
        ImGui::InputText("Bug Message", state->bugMessage, 256);
        if (ImGui::Button("Trigger Bug Now")) {
            *(state->triggerBugNow) = true;
        }
    }

    ImGui::End();
}
