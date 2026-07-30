#pragma once

struct DebugObjectState;

class IDebuggable {
public:
    virtual ~IDebuggable() = default;

    // オブジェクトの現在の状態を共有構造体に書き出す（キャプチャ用）
    virtual void ExportDebugState(DebugObjectState& outState) const = 0;

    // デバッグUIや巻き戻しで変更された状態をオブジェクトに反映する（適用用）
    virtual void ImportDebugState(const DebugObjectState& inState) = 0;
};
