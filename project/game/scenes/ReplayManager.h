#pragma once
#include <string>
#include <vector>
#include <json.hpp>
#include "Vector3.h"
#include "Vector4.h"

struct SnapshotObject {
    std::string name;
    std::string filename;
    Vector3 translation;
    Vector3 rotation;
    Vector3 scaling;
};

struct SnapshotData {
    std::vector<SnapshotObject> staticObjects;
};

struct EnemyFrameState {
    int index;
    Vector3 translation;
    Vector3 rotation;
    float hp;
    std::string animState;
};

struct FrameState {
    int frameIndex;
    Vector3 playerTranslation;
    Vector3 playerRotation;
    Vector4 playerColor;
    std::vector<Vector3> threadNodes;
    std::vector<EnemyFrameState> enemies;
    bool bugTrigger;
    std::string bugMsg;
};

class ReplayManager {
public:
    ReplayManager();
    ~ReplayManager();

    void Clear();
    void SetSnapshot(const SnapshotData& snapshot);
    void RecordFrame(const FrameState& state);
    void Truncate(int frameCount);
    
    bool SaveLog(const std::string& filepath);
    bool LoadLog(const std::string& filepath);

    const SnapshotData& GetSnapshot() const { return snapshot_; }
    const std::vector<FrameState>& GetFrames() const { return frames_; }
    
    int GetTotalFrames() const { return static_cast<int>(frames_.size()); }
    
    // 指定フレームの状態を取得する（インデックス範囲外ならfalse）
    bool GetFrameState(int index, FrameState& outState) const;

private:
    SnapshotData snapshot_;
    std::vector<FrameState> frames_;
};
