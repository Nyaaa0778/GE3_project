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

    // rail_splineの読み込み
    if (deserialized.contains("rail_spline")) {
        for (const auto& point : deserialized["rail_spline"]) {
            if (point.is_array() && point.size() >= 3) {
                levelData->railSpline.push_back({
                    point[0].get<float>(),
                    point[1].get<float>(),
                    point[2].get<float>()
                });
            }
        }
    } else if (deserialized.contains("objects")) {
        // Fallback: type "CURVE" のオブジェクトを検索し、control_points を取得
        for (const auto& obj : deserialized["objects"]) {
            if (obj.contains("type") && obj["type"].get<std::string>() == "CURVE") {
                if (obj.contains("control_points")) {
                    for (const auto& pt : obj["control_points"]) {
                        if (pt.is_array() && pt.size() >= 3) {
                            levelData->railSpline.push_back({
                                pt[0].get<float>(),
                                pt[1].get<float>(),
                                pt[2].get<float>()
                            });
                        }
                    }
                }
                break;
            }
        }
    }

    // objects の全オブジェクトを走査
    for (nlohmann::json& object : deserialized["objects"]) {
        assert(object.contains("type"));

        ParseObject(object, levelData.get());
    }

    return levelData;
}

void LevelLoader::ParseObject(const nlohmann::json& object, LevelData* levelData) {

    if (object.contains("disabled")) {
        bool disabled = object["disabled"].get<bool>();
        if (disabled) {
            // 配置しない（再帰関数から抜けることで、このオブジェクトと子オブジェクトの生成をスキップ）
            return;
        }
    }

    // 種類のチェック
    std::string type = object["type"].get<std::string>();

    if (type.compare("MESH") == 0) {
        // メッシュ
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
    } else if (type.compare("PlayerSpawn") == 0) {
        // オブジェクトの初期位置
        levelData->spawners.emplace_back(LevelData::SpawnerData{});
        LevelData::SpawnerData& spawnerData = levelData->spawners.back();

        // エディタ側で仕込んだ文字列（種類）を読み込む
        if (object.contains("entity_type")) {
            spawnerData.entityType = object["entity_type"].get<std::string>();
        } else if (object.contains("name")) {
            // カスタムプロパティが無ければオブジェクト名で代用するなどの工夫もアリ
            spawnerData.entityType = object["name"].get<std::string>();
        }

        const nlohmann::json& transform = object["transform"];
        // 平行移動の数値を書き込む
        spawnerData.translation = {
            (float) transform["translation"][0],
            (float) transform["translation"][1],
            (float) transform["translation"][2]
        };
        // 回転の数値をラジアンで書き込む
        spawnerData.rotation = {
            (float) transform["rotation"][0],
            (float) transform["rotation"][1],
            (float) transform["rotation"][2]
        };
        // 拡縮の数値を書き込む
        if (transform.contains("scaling")) {
            spawnerData.scaling = {
                (float) transform["scaling"][0],
                (float) transform["scaling"][1],
                (float) transform["scaling"][2]
            };
        }
    } else if (type.compare("CAMERA") == 0) {
        // カメラ
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
    } else if (type.compare("LIGHT") == 0) {
        // ライト
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
    } else if (type.compare("SPRITE") == 0) {
        // スプライト
        levelData->sprites.emplace_back(LevelData::SpriteData{});
        LevelData::SpriteData& spriteData = levelData->sprites.back();

        if (object.contains("file_name")) {
            spriteData.filename = object["file_name"].get<std::string>();
        } else if (object.contains("name")) {
            spriteData.filename = object["name"].get<std::string>();
        }

        const nlohmann::json& transform = object["transform"];
        spriteData.translation = {(float) transform["translation"][0], (float) transform["translation"][1]};
        
        if (transform.contains("rotation")) {
            spriteData.rotation = (float) transform["rotation"][2];
        } else {
            spriteData.rotation = 0.0f;
        }

        if (transform.contains("scaling")) {
            spriteData.scaling = {(float) transform["scaling"][0], (float) transform["scaling"][1]};
        } else {
            spriteData.scaling = {1.0f, 1.0f};
        }

        if (object.contains("color")) {
            spriteData.color = {
                (float) object["color"][0],
                (float) object["color"][1],
                (float) object["color"][2],
                (float) object["color"][3]
            };
        } else {
            spriteData.color = {1.0f, 1.0f, 1.0f, 1.0f};
        }
    }

    // オブジェクト走査を再帰関数にまとめて、再帰呼出で枝を走査
    if (object.contains("children")) {
        for (const nlohmann::json& child : object["children"]) {
            // もう一度自分自身の関数を呼ぶ！（再帰呼び出し）
            ParseObject(child, levelData);
        }
    }
}

bool LevelLoader::Save(const std::string& filename, const LevelData* levelData) {
    if (!levelData) return false;

    const std::string fullpath = kDefaultBaseDirectory + filename + kExtension;

    nlohmann::json root;
    root["name"] = "scene";

    // rail_spline
    nlohmann::json splineJson = nlohmann::json::array();
    for (const auto& pt : levelData->railSpline) {
        splineJson.push_back({pt.x, pt.y, pt.z});
    }
    root["rail_spline"] = splineJson;

    // objects
    nlohmann::json objectsJson = nlohmann::json::array();
    for (const auto& obj : levelData->objects) {
        nlohmann::json objJson;
        objJson["name"] = obj.filename;
        objJson["type"] = "MESH";
        objJson["file_name"] = obj.filename;

        nlohmann::json transform;
        transform["translation"] = {obj.translation.x, obj.translation.y, obj.translation.z};
        transform["rotation"] = {obj.rotation.x, obj.rotation.y, obj.rotation.z};
        transform["scaling"] = {obj.scaling.x, obj.scaling.y, obj.scaling.z};
        objJson["transform"] = transform;

        objectsJson.push_back(objJson);
    }

    // spawners (PlayerSpawn)
    for (const auto& spawner : levelData->spawners) {
        nlohmann::json spawnerJson;
        spawnerJson["name"] = spawner.entityType;
        spawnerJson["type"] = "PlayerSpawn";
        spawnerJson["entity_type"] = spawner.entityType;

        nlohmann::json transform;
        transform["translation"] = {spawner.translation.x, spawner.translation.y, spawner.translation.z};
        transform["rotation"] = {spawner.rotation.x, spawner.rotation.y, spawner.rotation.z};
        transform["scaling"] = {spawner.scaling.x, spawner.scaling.y, spawner.scaling.z};
        spawnerJson["transform"] = transform;

        objectsJson.push_back(spawnerJson);
    }

    // cameras
    for (const auto& cam : levelData->cameras) {
        nlohmann::json camJson;
        camJson["name"] = "Camera";
        camJson["type"] = "CAMERA";

        nlohmann::json transform;
        transform["translation"] = {cam.translation.x, cam.translation.y, cam.translation.z};
        transform["rotation"] = {cam.rotation.x - 1.570796f, cam.rotation.y, cam.rotation.z};
        transform["scaling"] = {1.0f, 1.0f, 1.0f};
        camJson["transform"] = transform;

        objectsJson.push_back(camJson);
    }

    // lights
    for (const auto& light : levelData->lights) {
        nlohmann::json lightJson;
        lightJson["name"] = "Light";
        lightJson["type"] = "LIGHT";

        nlohmann::json transform;
        transform["translation"] = {light.translation.x, light.translation.y, light.translation.z};
        transform["rotation"] = {light.rotation.x, light.rotation.y, light.rotation.z};
        transform["scaling"] = {1.0f, 1.0f, 1.0f};
        lightJson["transform"] = transform;

        objectsJson.push_back(lightJson);
    }

    // sprites
    for (const auto& sprite : levelData->sprites) {
        nlohmann::json spriteJson;
        spriteJson["name"] = sprite.filename;
        spriteJson["type"] = "SPRITE";
        spriteJson["file_name"] = sprite.filename;

        nlohmann::json transform;
        transform["translation"] = {sprite.translation.x, sprite.translation.y, 0.0f};
        transform["rotation"] = {0.0f, 0.0f, sprite.rotation};
        transform["scaling"] = {sprite.scaling.x, sprite.scaling.y, 1.0f};
        spriteJson["transform"] = transform;

        spriteJson["color"] = {sprite.color.x, sprite.color.y, sprite.color.z, sprite.color.w};

        objectsJson.push_back(spriteJson);
    }

    root["objects"] = objectsJson;

    // ファイルに書き出す
    std::ofstream file(fullpath);
    if (file.fail()) {
        return false;
    }
    file << root.dump(4);
    return true;
}
