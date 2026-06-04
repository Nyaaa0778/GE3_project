#pragma once
#include <string>
#include <vector>
#include <optional>
#include "json.hpp"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

// =================================================================
//  数学ベクトル構造体 (Vector2, Vector3, Vector4) のシリアライズ・デシリアライズ
//  (nlohmann::json が自動変換できるようにするためのヘルパー関数)
// =================================================================

/// <summary>
/// Vector2型をJSONオブジェクトにシリアライズします。
/// </summary>
/// <param name="json">格納先のJSONオブジェクト</param>
/// <param name="value">変換元のVector2構造体</param>
void ToJson(nlohmann::json& json, const Vector2& value);

/// <summary>
/// JSONオブジェクトからVector2型にデシリアライズします。
/// </summary>
/// <param name="json">読み込み元のJSONオブジェクト</param>
/// <param name="value">格納先のVector2構造体</param>
void FromJson(const nlohmann::json& json, Vector2& value);

/// <summary>
/// Vector3型をJSONオブジェクトにシリアライズします。
/// </summary>
/// <param name="json">格納先のJSONオブジェクト</param>
/// <param name="value">変換元のVector3構造体</param>
void ToJson(nlohmann::json& json, const Vector3& value);

/// <summary>
/// JSONオブジェクトからVector3型にデシリアライズします。
/// </summary>
/// <param name="json">読み込み元のJSONオブジェクト</param>
/// <param name="value">格納先のVector3構造体</param>
void FromJson(const nlohmann::json& json, Vector3& value);

/// <summary>
/// Vector4型をJSONオブジェクトにシリアライズします。
/// </summary>
/// <param name="json">格納先のJSONオブジェクト</param>
/// <param name="value">変換元のVector4構造体</param>
void ToJson(nlohmann::json& json, const Vector4& value);

/// <summary>
/// JSONオブジェクトからVector4型にデシリアライズします。
/// </summary>
/// <param name="json">読み込み元のJSONオブジェクト</param>
/// <param name="value">格納先のVector4構造体</param>
void FromJson(const nlohmann::json& json, Vector4& value);

// =================================================================
//  JsonLoader クラス
// =================================================================

/// <summary>
/// JSONファイルの読み書き、およびVector型配列の変換を管理するクラスです。
/// </summary>
class JsonLoader {
public:
    /// <summary>
    /// JSONファイルを読み込みます。
    /// </summary>
    /// <param name="filePath">ファイルパス (例: "resources/data.json")</param>
    /// <returns>読み込んだjson。失敗時は nullopt</returns>
    static std::optional<nlohmann::json> Load(const std::string& filePath);

    /// <summary>
    /// JSONファイルを保存します。（フォルダが存在しない場合は自動作成します）
    /// </summary>
    /// <param name="filePath">ファイルパス (例: "resources/data.json")</param>
    /// <param name="json">保存するjson</param>
    /// <returns>成功した場合は true</returns>
    static bool Save(const std::string& filePath, const nlohmann::json& json);

    /// <summary>
    /// 動的配列 (std::vector) をJSON配列に変換します。
    /// </summary>
    /// <typeparam name="T">配列要素の型 (Vector2/3/4など)</typeparam>
    /// <param name="vec">変換元の動的配列</param>
    /// <returns>変換後のJSON配列</returns>
    template <typename T>
    static nlohmann::json ToJsonArray(const std::vector<T>& vec) {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& item : vec) {
            nlohmann::json j;
            ToJson(j, item);
            arr.push_back(j);
        }
        return arr;
    }

    /// <summary>
    /// JSON配列から動的配列 (std::vector) を復元します。
    /// </summary>
    /// <typeparam name="T">配列要素の型 (Vector2/3/4など)</typeparam>
    /// <param name="arr">変換元のJSON配列</param>
    /// <returns>復元された動的配列</returns>
    template <typename T>
    static std::vector<T> FromJsonArray(const nlohmann::json& arr) {
        std::vector<T> vec;
        if (arr.is_array()) {
            vec.reserve(arr.size());
            for (const auto& item : arr) {
                T value;
                FromJson(item, value);
                vec.push_back(value);
            }
        }
        return vec;
    }
};
