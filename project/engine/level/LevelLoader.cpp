#include "LevelLoader.h"

#include <fstream>

std::unique_ptr<LevelData> LevelLoader::Load(const std::string& filename) {
    // 連結して古パスを得る
    const std::string fullpath = kDefaultBaseDirectory + filename + kExtension;

    // ファイルストリーム
    std::ifstream file;

    // ファイルを開く
    file.open(fullpath);
    // ファイルをオープン失敗チェック
    if (file.fail()) { assert(0); }

    // JSON文字列から解凍したデータ
    nlohmann::json deserialized;

    // 解凍
    file >> deserialized;

    // 正しいレベルデータファイルかチェック
    assert(deserialized.is_object());
    assert(deserialized.contains("name"));
    assert(deserialized["name"].is_string());

    // name を文字列として取得
    std::string name = deserialized["name"].get<std::string>();
    // 正しいレベルデータファイルかチェック
    assert(name.compare("scene") == 0);

    // レベルデータ格納用インスタンスを生成
    std::unique_ptr<LevelData> levelData = std::make_unique<LevelData>();

    // objects の全オブジェクトを走査
    for (nlohmann::json& object : deserialized["objects"]) {
        assert(object.contains("type"));

        ParseObject(object, levelData.get());
    }

    return levelData;
}

void LevelLoader::ParseObject(const nlohmann::json& object, LevelData* levelData) {

    // 種類のチェック
    std::string type = object["type"].get<std::string>();

    // メッシュ
    if (type.compare("MESH") == 0) {
        // 要素追加
        levelData->objects.emplace_back(LevelData::ObjectData{});
        // 追加した要素の参照を得る
        LevelData::ObjectData& objectData = levelData->objects.back();

        if (object.contains("file_name")) {
            // ファイル名
            objectData.filename = object["file_name"].get<std::string>();
        } else if (object.contains("name")) {
            // name からファイル名を推測するフォールバック
            std::string name = object["name"].get<std::string>();
            if (name == "Cube" || name == "cube") {
                objectData.filename = "cube";
            } else if (name.find("ICO") != std::string::npos || name.find("Sphere") != std::string::npos || name.find("sphere") != std::string::npos) {
                objectData.filename = "sphere";
            } else {
                objectData.filename = name;
            }
        }

        // トランスフォームのパラメータ読み込み
        const nlohmann::json& transform = object["transform"];
        // 平行移動 (すでにエクスポーターでスワップ済みのため直読み)
        objectData.translation = {(float) transform["translation"][0], (float) transform["translation"][1], (float) transform["translation"][2]};
        // 回転 (すでにエクスポーターでラジアンに変換されているため直読み)
        objectData.rotation = {
            (float) transform["rotation"][0],
            (float) transform["rotation"][1],
            (float) transform["rotation"][2]
        };
        // 拡縮
        objectData.scaling = {(float) transform["scaling"][0], (float) transform["scaling"][1], (float) transform["scaling"][2]};
    }
    // カメラ
    else if (type.compare("CAMERA") == 0) {
        levelData->cameras.emplace_back(LevelData::CameraData{});
        LevelData::CameraData& cameraData = levelData->cameras.back();

        const nlohmann::json& transform = object["transform"];
        // 平行移動 (すでにエクスポーターでスワップ済みのため直読み)
        cameraData.translation = {(float) transform["translation"][0], (float) transform["translation"][1], (float) transform["translation"][2]};
        // 回転 (すでにエクスポーターでラジアンに変換されているため、90度(1.570796f)オフセットのみ適用)
        float rx = (float) transform["rotation"][0];
        float ry = (float) transform["rotation"][1];
        float rz = (float) transform["rotation"][2];
        cameraData.rotation = {
            1.570796f + rx,
            ry,
            rz
        };
    }
    // ライト
    else if (type.compare("LIGHT") == 0) {
        levelData->lights.emplace_back(LevelData::LightData{});
        LevelData::LightData& lightData = levelData->lights.back();

        const nlohmann::json& transform = object["transform"];
        // 平行移動 (すでにエクスポーターでスワップ済みのため直読み)
        lightData.translation = {(float) transform["translation"][0], (float) transform["translation"][1], (float) transform["translation"][2]};
        // 回転 (すでにエクスポーターでラジアンに変換されているため直読み)
        lightData.rotation = {
            (float) transform["rotation"][0],
            (float) transform["rotation"][1],
            (float) transform["rotation"][2]
        };
    }

    // オブジェクト走査を再帰関数にまとめて、再帰呼出で枝を走査
    if (object.contains("children")) {
        for (const nlohmann::json& child : object["children"]) {
            // もう一度自分自身の関数を呼ぶ！（再帰呼び出し）
            ParseObject(child, levelData);
        }
    }
}
