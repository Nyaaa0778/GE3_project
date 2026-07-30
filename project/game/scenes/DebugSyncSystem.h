#pragma once
#include <vector>
#include <memory>
#include <string>
#include <array>
#include <cstring>
#include "DebugState.h"
#include "IDebuggable.h"
#include "Object3d.h"

// 既存の Object3d をデバッグシステムに適合させるためのアダプター
class DebuggableObject : public IDebuggable {
private:
    Object3d* owner_;
    std::string name_;
    std::string typeName_;

public:
    DebuggableObject(Object3d* owner, const std::string& name, const std::string& typeName)
        : owner_(owner), name_(name), typeName_(typeName) {}

    void ExportDebugState(DebugObjectState& outState) const override {
        // 安全に文字列をコピー
        strcpy_s(outState.name, sizeof(outState.name), name_.c_str());
        strcpy_s(outState.typeName, sizeof(outState.typeName), typeName_.c_str());

        const Vector3& pos = owner_->GetPosition();
        outState.pos[0] = pos.x;
        outState.pos[1] = pos.y;
        outState.pos[2] = pos.z;

        const Vector3& rot = owner_->GetRotate();
        outState.rot[0] = rot.x;
        outState.rot[1] = rot.y;
        outState.rot[2] = rot.z;

        const Vector3& scale = owner_->GetScale();
        outState.scale[0] = scale.x;
        outState.scale[1] = scale.y;
        outState.scale[2] = scale.z;

        outState.isAlive = true;
        
        // 初期状態ではカスタムパラメータは0にしておく
        outState.customFloatCount = 0;
    }

    void ImportDebugState(const DebugObjectState& inState) override {
        owner_->SetPosition({inState.pos[0], inState.pos[1], inState.pos[2]});
        owner_->SetRotation({inState.rot[0], inState.rot[1], inState.rot[2]});
        owner_->SetScale({inState.scale[0], inState.scale[1], inState.scale[2]});
    }
};

// 同期マネージャ
class DebugSyncSystem {
public:
    DebugSyncSystem() = default;
    ~DebugSyncSystem() = default;

    // DebugState (DLL共有用構造体) とバインドする
    void BindToState(DebugState& state) {
        state.objectCount = &objectCount_;
        state.objects = objectsBuffer_.data();
    }

    // オブジェクトを登録する
    void Register(Object3d* object, const std::string& name, const std::string& type) {
        if (!object) return;
        debuggables_.push_back(std::make_shared<DebuggableObject>(object, name, type));
    }

    // 毎フレーム呼び出す同期処理 (ゲーム ➔ DLL)
    void Capture() {
        objectCount_ = static_cast<int>(debuggables_.size());
        for (int i = 0; i < objectCount_ && i < 100; ++i) {
            debuggables_[i]->ExportDebugState(objectsBuffer_[i]);
        }
    }

    // 毎フレーム呼び出す適用処理 (DLL ➔ ゲーム)
    void Apply() {
        for (int i = 0; i < objectCount_ && i < 100; ++i) {
            debuggables_[i]->ImportDebugState(objectsBuffer_[i]);
        }
    }
    
    // リプレイ復元時用：指定フレームから直接状態をインポートする
    void ImportFrameState(const std::vector<DebugObjectState>& frameObjects) {
        for (const auto& fObj : frameObjects) {
            for (auto& debuggable : debuggables_) {
                DebugObjectState current = {};
                debuggable->ExportDebugState(current);
                if (std::strcmp(current.name, fObj.name) == 0) {
                    debuggable->ImportDebugState(fObj);
                    break;
                }
            }
        }
    }
    
    // リプレイ記録時用：現在の状態をリストに吐き出す
    std::vector<DebugObjectState> ExportFrameState() const {
        std::vector<DebugObjectState> result;
        int count = static_cast<int>(debuggables_.size());
        for (int i = 0; i < count && i < 100; ++i) {
            DebugObjectState state = {};
            debuggables_[i]->ExportDebugState(state);
            result.push_back(state);
        }
        return result;
    }

private:
    std::vector<std::shared_ptr<IDebuggable>> debuggables_;
    
    int objectCount_ = 0;
    std::array<DebugObjectState, 100> objectsBuffer_ = {}; // DLLに渡すための固定メモリバッファ
};
