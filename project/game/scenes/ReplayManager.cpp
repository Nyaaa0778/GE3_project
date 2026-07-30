#include "ReplayManager.h"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

ReplayManager::ReplayManager() = default;
ReplayManager::~ReplayManager() = default;

void ReplayManager::Clear() {
    snapshot_.staticObjects.clear();
    frames_.clear();
}

void ReplayManager::SetSnapshot(const SnapshotData& snapshot) {
    snapshot_ = snapshot;
}

void ReplayManager::RecordFrame(const FrameState& state) {
    frames_.push_back(state);
}

void ReplayManager::Truncate(int frameCount) {
    if (frameCount >= 0 && frameCount < static_cast<int>(frames_.size())) {
        frames_.resize(frameCount);
    }
}

bool ReplayManager::GetFrameState(int index, FrameState& outState) const {
    if (index < 0 || index >= static_cast<int>(frames_.size())) {
        return false;
    }
    outState = frames_[index];
    return true;
}

bool ReplayManager::SaveLog(const std::string& filepath) {
    json root = json::object();
    
    // Snapshot
    json jSnapshot = json::object();
    json jStaticObjs = json::array();
    for (const auto& obj : snapshot_.staticObjects) {
        json jObj = json::object();
        jObj["name"] = obj.name;
        jObj["file_name"] = obj.filename;
        jObj["translation"] = { obj.translation.x, obj.translation.y, obj.translation.z };
        jObj["rotation"] = { obj.rotation.x, obj.rotation.y, obj.rotation.z };
        jObj["scaling"] = { obj.scaling.x, obj.scaling.y, obj.scaling.z };
        jStaticObjs.push_back(jObj);
    }
    jSnapshot["static_objects"] = jStaticObjs;
    root["snapshot"] = jSnapshot;

    // Frames
    json jFrames = json::array();
    for (const auto& f : frames_) {
        json jFrame = json::object();
        jFrame["frame"] = f.frameIndex;
        
        json jPlayer = json::object();
        jPlayer["translation"] = { f.playerTranslation.x, f.playerTranslation.y, f.playerTranslation.z };
        jPlayer["rotation"] = { f.playerRotation.x, f.playerRotation.y, f.playerRotation.z };
        jPlayer["color"] = { f.playerColor.x, f.playerColor.y, f.playerColor.z, f.playerColor.w };
        jFrame["player"] = jPlayer;

        json jThread = json::array();
        for (const auto& node : f.threadNodes) {
            jThread.push_back({ node.x, node.y, node.z });
        }
        jFrame["thread_nodes"] = jThread;

        json jEnemies = json::array();
        for (const auto& e : f.enemies) {
            json jEnemy = json::object();
            jEnemy["index"] = e.index;
            jEnemy["translation"] = { e.translation.x, e.translation.y, e.translation.z };
            jEnemy["rotation"] = { e.rotation.x, e.rotation.y, e.rotation.z };
            jEnemy["hp"] = e.hp;
            jEnemy["anim_state"] = e.animState;
            jEnemies.push_back(jEnemy);
        }
        jFrame["enemies"] = jEnemies;

        json jObjects = json::array();
        for (const auto& obj : f.objects) {
            json jObj = json::object();
            jObj["name"] = obj.name;
            jObj["type"] = obj.typeName;
            jObj["translation"] = { obj.pos[0], obj.pos[1], obj.pos[2] };
            jObj["rotation"] = { obj.rot[0], obj.rot[1], obj.rot[2] };
            jObj["scaling"] = { obj.scale[0], obj.scale[1], obj.scale[2] };
            jObj["is_alive"] = obj.isAlive;
            
            if (obj.customFloatCount > 0) {
                json jCustom = json::array();
                for (int p = 0; p < obj.customFloatCount; p++) {
                    json jParam = json::object();
                    jParam["name"] = obj.customFloatNames[p];
                    jParam["value"] = obj.customFloats[p];
                    jCustom.push_back(jParam);
                }
                jObj["custom_floats"] = jCustom;
            }
            jObjects.push_back(jObj);
        }
        jFrame["objects"] = jObjects;

        json jEvents = json::object();
        jEvents["bug_trigger"] = f.bugTrigger;
        jEvents["msg"] = f.bugMsg;
        jFrame["events"] = jEvents;

        jFrames.push_back(jFrame);
    }
    root["frames"] = jFrames;

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    file << root.dump(4);
    file.close();
    return true;
}

bool ReplayManager::LoadLog(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (...) {
        return false;
    }
    file.close();

    Clear();

    // Parse Snapshot
    if (root.contains("snapshot") && root["snapshot"].contains("static_objects")) {
        for (const auto& jObj : root["snapshot"]["static_objects"]) {
            SnapshotObject obj;
            obj.name = jObj.value("name", "");
            obj.filename = jObj.value("file_name", "");
            
            auto t = jObj["translation"];
            obj.translation = { t[0].get<float>(), t[1].get<float>(), t[2].get<float>() };
            
            auto r = jObj["rotation"];
            obj.rotation = { r[0].get<float>(), r[1].get<float>(), r[2].get<float>() };
            
            auto s = jObj["scaling"];
            obj.scaling = { s[0].get<float>(), s[1].get<float>(), s[2].get<float>() };

            snapshot_.staticObjects.push_back(obj);
        }
    }

    // Parse Frames
    if (root.contains("frames")) {
        for (const auto& jFrame : root["frames"]) {
            FrameState f;
            f.frameIndex = jFrame.value("frame", 0);

            if (jFrame.contains("player")) {
                auto jp = jFrame["player"];
                auto t = jp["translation"];
                f.playerTranslation = { t[0].get<float>(), t[1].get<float>(), t[2].get<float>() };
                auto r = jp["rotation"];
                f.playerRotation = { r[0].get<float>(), r[1].get<float>(), r[2].get<float>() };
                auto c = jp["color"];
                f.playerColor = { c[0].get<float>(), c[1].get<float>(), c[2].get<float>(), c[3].get<float>() };
            }

            if (jFrame.contains("thread_nodes")) {
                for (const auto& jNode : jFrame["thread_nodes"]) {
                    f.threadNodes.push_back({ jNode[0].get<float>(), jNode[1].get<float>(), jNode[2].get<float>() });
                }
            }

            if (jFrame.contains("enemies")) {
                for (const auto& jEnemy : jFrame["enemies"]) {
                    EnemyFrameState e;
                    e.index = jEnemy.value("index", 0);
                    auto t = jEnemy["translation"];
                    e.translation = { t[0].get<float>(), t[1].get<float>(), t[2].get<float>() };
                    auto r = jEnemy["rotation"];
                    e.rotation = { r[0].get<float>(), r[1].get<float>(), r[2].get<float>() };
                    e.hp = jEnemy.value("hp", 0.0f);
                    e.animState = jEnemy.value("anim_state", "");
                    f.enemies.push_back(e);
                }
            }

            if (jFrame.contains("objects")) {
                for (const auto& jObj : jFrame["objects"]) {
                    DebugObjectState obj = {};
                    strcpy_s(obj.name, jObj.value("name", "").c_str());
                    strcpy_s(obj.typeName, jObj.value("type", "").c_str());
                    
                    auto t = jObj["translation"];
                    obj.pos[0] = t[0].get<float>();
                    obj.pos[1] = t[1].get<float>();
                    obj.pos[2] = t[2].get<float>();
                    
                    auto r = jObj["rotation"];
                    obj.rot[0] = r[0].get<float>();
                    obj.rot[1] = r[1].get<float>();
                    obj.rot[2] = r[2].get<float>();
                    
                    auto s = jObj["scaling"];
                    obj.scale[0] = s[0].get<float>();
                    obj.scale[1] = s[1].get<float>();
                    obj.scale[2] = s[2].get<float>();
                    
                    obj.isAlive = jObj.value("is_alive", true);
                    
                    if (jObj.contains("custom_floats")) {
                        int pCount = 0;
                        for (const auto& jParam : jObj["custom_floats"]) {
                            if (pCount >= 4) break;
                            strcpy_s(obj.customFloatNames[pCount], jParam.value("name", "").c_str());
                            obj.customFloats[pCount] = jParam.value("value", 0.0f);
                            pCount++;
                        }
                        obj.customFloatCount = pCount;
                    } else {
                        obj.customFloatCount = 0;
                    }
                    f.objects.push_back(obj);
                }
            }

            if (jFrame.contains("events")) {
                auto je = jFrame["events"];
                f.bugTrigger = je.value("bug_trigger", false);
                f.bugMsg = je.value("msg", "");
            } else {
                f.bugTrigger = false;
                f.bugMsg = "";
            }

            frames_.push_back(f);
        }
    }

    return true;
}
