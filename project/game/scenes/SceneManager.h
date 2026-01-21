#pragma once

class IScene;

class SceneManager {
public:
  //================================================================================
  // シングルトン
  //================================================================================

  // 唯一のインスタンス取得
  static SceneManager *GetInstance();

  static void Shutdown();

  /// <summary>
  /// コンストラクタ
  /// </summary>
  SceneManager();
  /// <summary>
  /// デストラクタ
  /// </summary>
  ~SceneManager();

private:
  static SceneManager *instance;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  SceneManager(SceneManager &) = delete;
  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  SceneManager &operator=(SceneManager &) = delete;

public:
  void Update();

  void Draw();

public:
  // 次のシーンを予約
  void SetNextScene(IScene *nextScene) { nextScene_ = nextScene; }

private:
  // 今のシーン
  IScene *scene_ = nullptr;
  // 次のシーン
  IScene *nextScene_ = nullptr;

private:
  void ChangeScene();
};
