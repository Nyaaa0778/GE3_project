#include "RailPath.h"
#include <fstream>
#include <filesystem>
#include "json.hpp"
#include "MathUtility.h"

using namespace MathUtility;

// -------------------------------------------------------
//  点の操作
// -------------------------------------------------------

void RailPath::AddPoint(const Vector3& point) {
    points_.push_back(point);
}

void RailPath::InsertPoint(size_t index, const Vector3& point) {
    index = std::min(index, points_.size());
    points_.insert(points_.begin() + index, point);
}

void RailPath::DeletePoint(size_t index) {
    if (index < points_.size()) {
        points_.erase(points_.begin() + index);
    }
}

void RailPath::Clear() {
    points_.clear();
}

void RailPath::SetPoint(size_t index, const Vector3& point) {
    if (index < points_.size()) {
        points_[index] = point;
    }
}

// -------------------------------------------------------
//  Catmull-Rom 補間
// -------------------------------------------------------

Vector3 RailPath::CatmullRom(
    const Vector3& p0, const Vector3& p1,
    const Vector3& p2, const Vector3& p3,
    float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;

    return 0.5f * (
        2.0f * p1
        + (-p0 + p2) * t
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
}

Vector3 RailPath::Evaluate(float t) const {
    const size_t n = points_.size();
    if (n == 0) return Vector3(0.0f, 0.0f, 0.0f);
    if (n == 1) return points_[0];
    if (t <= 0.0f) return points_.front();
    if (t >= 1.0f) return points_.back();

    const size_t numSegments = n - 1;
    const float  progress = t * static_cast<float>(numSegments);
    size_t       seg = static_cast<size_t>(progress);
    const float  localT = progress - static_cast<float>(seg);

    // 末端クランプ
    if (seg >= numSegments) { seg = numSegments - 1; }

    const size_t i0 = (seg > 0) ? seg - 1 : 0;
    const size_t i1 = seg;
    const size_t i2 = seg + 1;
    const size_t i3 = (seg + 2 < n) ? seg + 2 : n - 1;

    return CatmullRom(points_[i0], points_[i1], points_[i2], points_[i3], localT);
}

// -------------------------------------------------------
//  JSON 保存 / 読み込み
// -------------------------------------------------------

bool RailPath::SaveToJson(const std::string& filePath) const {
    nlohmann::json root;
    auto& arr = root["points"] = nlohmann::json::array();

    for (const auto& pt : points_) {
        arr.push_back({{"x", pt.x}, {"y", pt.y}, {"z", pt.z}});
    }

    const std::filesystem::path path(filePath);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << root.dump(4);
    return true;
}

bool RailPath::LoadFromJson(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    nlohmann::json root;
    try {
        file >> root;
    }
    catch (...) {
        return false;
    }

    if (!root.contains("points") || !root["points"].is_array()) return false;

    points_.clear();
    points_.reserve(root["points"].size());

    for (const auto& j : root["points"]) {
        if (j.contains("x") && j.contains("y") && j.contains("z")) {
            points_.push_back({j["x"].get<float>(), j["y"].get<float>(), j["z"].get<float>()});
        }
    }
    return true;
}