#include "JsonLoader.h"
#include <fstream>
#include <filesystem>
#include <iostream>

// ========================================================
//  数学ベクトル構造体のJSONシリアライズ・デシリアライズ実装
// ========================================================

void ToJson(nlohmann::json& json, const Vector2& value) {
    json = nlohmann::json{{"x", value.x}, {"y", value.y}};
}

void FromJson(const nlohmann::json& json, Vector2& value) {
    value.x = json.value("x", 0.0f);
    value.y = json.value("y", 0.0f);
}

void ToJson(nlohmann::json& json, const Vector3& value) {
    json = nlohmann::json{{"x", value.x}, {"y", value.y}, {"z", value.z}};
}

void FromJson(const nlohmann::json& json, Vector3& value) {
    value.x = json.value("x", 0.0f);
    value.y = json.value("y", 0.0f);
    value.z = json.value("z", 0.0f);
}

void ToJson(nlohmann::json& json, const Vector4& value) {
    json = nlohmann::json{{"x", value.x}, {"y", value.y}, {"z", value.z}, {"w", value.w}};
}

void FromJson(const nlohmann::json& json, Vector4& value) {
    value.x = json.value("x", 0.0f);
    value.y = json.value("y", 0.0f);
    value.z = json.value("z", 0.0f);
    value.w = json.value("w", 0.0f);
}

// ========================================================
//  JsonLoader クラス実装
// ========================================================

std::optional<nlohmann::json> JsonLoader::Load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[JsonLoader] Failed to open file for reading: " << filePath << std::endl;
        return std::nullopt;
    }

    nlohmann::json root;
    try {
        file >> root;
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[JsonLoader] Parse error in " << filePath << ": " << e.what() << std::endl;
        return std::nullopt;
    }
    catch (...) {
        std::cerr << "[JsonLoader] Unknown error during parsing: " << filePath << std::endl;
        return std::nullopt;
    }

    return root;
}

bool JsonLoader::Save(const std::string& filePath, const nlohmann::json& json) {
    const std::filesystem::path path(filePath);
    try {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[JsonLoader] Failed to create directories for " << filePath << ": " << e.what() << std::endl;
        return false;
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[JsonLoader] Failed to open file for writing: " << filePath << std::endl;
        return false;
    }

    file << json.dump(4);
    return true;
}
