#include <imgui.h>
#include "DebugState.h"

void DrawDataFlowSimulator(DebugState* state);

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

    // Dynamic Objects (New Sync System)
    if (state->objectCount && state->objects) {
        if (ImGui::CollapsingHeader("Active Scene Objects (Dynamic)", ImGuiTreeNodeFlags_DefaultOpen)) {
            int objectCount = *(state->objectCount);
            ImGui::Text("Registered Objects: %d", objectCount);
            for (int i = 0; i < objectCount; i++) {
                DebugObjectState& obj = state->objects[i];
                if (!obj.isAlive) continue;

                ImGui::PushID(i);
                if (ImGui::TreeNode(obj.name, "%s [%s]", obj.name, obj.typeName)) {
                    ImGui::DragFloat3("Position", obj.pos, 0.1f);
                    ImGui::DragFloat3("Rotation", obj.rot, 0.05f);
                    ImGui::DragFloat3("Scale", obj.scale, 0.05f);

                    for (int p = 0; p < obj.customFloatCount; p++) {
                        ImGui::SliderFloat(obj.customFloatNames[p], &obj.customFloats[p], 0.0f, 100.0f);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
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

    DrawDataFlowSimulator(state);

    ImGui::End();
}

void DrawDataFlowSimulator(DebugState* state) {
    ImGui::Begin("Blender Data Flow Simulator");
    ImGui::Text("Interactive Flow of Replay & Takeover Sync");
    ImGui::Separator();

    // Node 1: Game Engine Logger
    ImGui::BeginChild("EngineNode", ImVec2(220, 160), true);
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "[1] Game Engine (C++)");
    ImGui::Text("State: %s", *(state->isPlayback) ? "ROLLBACK PLAYBACK" : "LIVE RECORDING");
    ImGui::Text("Total Frames: %d", *(state->totalFrames));
    ImGui::Text("Current Frame: %d", *(state->isPlayback) ? *(state->playbackFrame) : *(state->totalFrames));
    
    if (ImGui::Button("Explain Logger")) {
        ImGui::OpenPopup("ExplainLoggerPopup");
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::Text(" ----> ");
    ImGui::SameLine();

    // Node 2: JSON Log File
    ImGui::BeginChild("JsonNode", ImVec2(220, 160), true);
    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "[2] replay_log.json");
    ImGui::Text("Snapshot Objects: %d", *(state->staticObjectCount));
    ImGui::Text("Enemies Tracked: %d", *(state->enemyCount));
    ImGui::Text("Max Thread Nodes: 100");
    if (ImGui::Button("Explain Log JSON")) {
        ImGui::OpenPopup("ExplainJSONPopup");
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::Text(" ----> ");
    ImGui::SameLine();

    // Node 3: Blender Python Importer
    ImGui::BeginChild("BlenderNode", ImVec2(220, 160), true);
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.1f, 1.0f), "[3] Blender 3D (Python)");
    ImGui::Text("Sync TCP Socket: %s", *(state->isSocketConnected) ? "CONNECTED" : "DISCONNECTED");
    ImGui::Text("Curve Object: PlayerThread");
    if (ImGui::Button("Explain Addon")) {
        ImGui::OpenPopup("ExplainBlenderPopup");
    }
    ImGui::EndChild();

    ImGui::Separator();
    
    // Bidirectional Socket connection visual flow
    if (*(state->isSocketConnected)) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "LIVE SYNC ACTIVE: Blender Timeline <==== (TCP) ====> Game Replay Engine");
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sync Status: Waiting for Blender socket connection on port 12345...");
    }

    // Explanation Popups
    if (ImGui::BeginPopup("ExplainLoggerPopup")) {
        ImGui::Text("Phase 1 (C++ side):");
        ImGui::BulletText("At stage start, captures coordinates of fixed objects (snapshot).");
        ImGui::BulletText("Every frame, records player transform, thread coordinates, enemy details, and flags.");
        ImGui::BulletText("Stores list in memory and writes to logs/play_log.json on save.");
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("ExplainJSONPopup")) {
        ImGui::Text("Replay Data Format:");
        ImGui::BulletText("`snapshot`: Fixed meshes (eg. eggs, traps, terrain) coordinate structures.");
        ImGui::BulletText("`frames`: Array of frame-by-frame updates (player, thread curve nodes, enemy HP/animations).");
        ImGui::BulletText("`events`: Event triggers (eg. bugs, collisions) flagged to jump to instantly.");
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("ExplainBlenderPopup")) {
        ImGui::Text("Phase 2 & 3 (Blender side):");
        ImGui::BulletText("Reconstructs the stage using bpy primitives based on the snapshot.");
        ImGui::BulletText("Inserts location/rotation keyframes on timeline for animation.");
        ImGui::BulletText("Updates curve control points on frame change pre-handler.");
        ImGui::BulletText("Sends socket message `FRAME <index>` to sync back to the C++ engine.");
        ImGui::EndPopup();
    }

    ImGui::End();
}
