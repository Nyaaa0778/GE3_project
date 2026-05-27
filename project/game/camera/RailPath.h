#pragma once
#include <Vector3.h>
#include <vector>
#include <string>

class RailPath {
public:
    // --- 点の操作 ---
    void AddPoint(const Vector3& point);
    void InsertPoint(size_t index, const Vector3& point);
    void DeletePoint(size_t index);
    void Clear();

    // --- ゲッター / セッター ---
    const std::vector<Vector3>& GetPoints()     const { return points_; }
    size_t GetPointCount() const { return points_.size(); }
    void SetPoint(size_t index, const Vector3& point);

    // Catmull-Rom 補間による評価 (t: 0.0f 〜 1.0f)
    Vector3 Evaluate(float t) const;

    // JSON 保存 / 読み込み
    bool SaveToJson(const std::string& filePath) const;
    bool LoadFromJson(const std::string& filePath);

private:
    // Catmull-Rom 補間の計算式（係数展開済み）
    static Vector3 CatmullRom(
        const Vector3& p0, const Vector3& p1,
        const Vector3& p2, const Vector3& p3,
        float t);

    std::vector<Vector3> points_;
};