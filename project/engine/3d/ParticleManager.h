#pragma once

#include <d3d12.h>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>

#include <Matrix4x4.h>
#include <Transform.h>
#include <Vector2.h>
#include <Vector3.h>
#include <Vector4.h>

class DirectXCommon;
class ShaderResourceViewManager;

class ParticleManager {
private:
  // 1グループの最大パーティクル数
  const uint32_t kMaxInstancePerGroup = 1024;

public:
  //================================================================================
  // シングルトン
  //================================================================================

  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  /// <returns>ParticleManager の唯一のインスタンス</returns>
  static ParticleManager *GetInstance();

  /// <summary>
  /// 終了
  /// </summary>
  void Shutdown();

private:
  static ParticleManager *instance;

  /// <summary>
  /// コンストラクタ
  /// </summary>
  ParticleManager() = default;

  /// <summary>
  /// デストラクタ
  /// </summary>
  ~ParticleManager() = default;

  /// <summary>
  /// コピーコンストラクタ禁止
  /// </summary>
  /// <param name="">コピー元（使用不可）</param>
  ParticleManager(ParticleManager &) = delete;

  /// <summary>
  /// 代入演算子禁止
  /// </summary>
  /// <param name="">代入元（使用不可）</param>
  /// <returns>このオブジェクトを返す</returns>
  ParticleManager &operator=(ParticleManager &) = delete;

public:
  //================================================================================
  // 初期化 / 更新 / 描画
  //================================================================================

  /// <summary>
  /// 初期化
  /// </summary>
  /// <param name="dxCommon">DirectXCommonのポインタ</param>
  /// <param name="srvManager">SrvManagerのポインタ</param>
  void Initialize(DirectXCommon *dxCommon,
                  ShaderResourceViewManager *srvManager);

  /// <summary>
  /// 更新
  /// </summary>
  /// <param name="viewmatrix">カメラのビュー行列</param>
  /// <param name="projectionMatrix">カメラの射影行列</param>
  void Update(const Matrix4x4 &viewmatrix, const Matrix4x4 &projectionMatrix);

  /// <summary>
  /// 描画
  /// </summary>
  void Draw();

  //================================================================================
  // パーティクルグループ作成 / パーティクル生成
  //================================================================================

  /// <summary>
  /// パーティクルグループを作成
  /// </summary>
  /// <param name="name">作成するパーティクルグループ名</param>
  /// <param
  /// name="textureFilePath">グループで使用するテクスチャのファイルパス</param>
  void CreateParticleGroup(const std::string groupName,
                           const std::string textureFilePath);

  /// <summary>
  /// 指定したパーティクルグループからパーティクルを発生させる
  /// </summary>
  /// <param name="groupName">発生させたいパーティクルグループ名</param>
  /// <param name="emitPosition">パーティクルの発生位置</param>
  /// <param name="count">発生させるパーティクル数</param>
  void Emit(const std::string groupName, const Vector3 &emitPosition,
            uint32_t count);

private:
  //================================================================================
  // 型エイリアス
  //================================================================================

  // namespace
  template <class InterfaceType>
  using ComPtr = Microsoft::WRL::ComPtr<InterfaceType>;

private:
  //================================================================================
  // 内部構造体
  //================================================================================

  // 頂点データ
  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
  };

  // パーティクル
  struct Particle {
    Transform transform;
    Vector3 velocity;
    Vector4 color;
    float lifeTime;
    float currentTime;
  };

  // GPUへ渡すインスタンス1個分
  struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
  };

  // マテリアルデータ
  struct MaterialData {
    std::string textureFilePath;
    uint32_t textureSrvIndex = 0;
  };

  // パーティクルグループ
  struct ParticleGroup {
    // マテリアル（テクスチャパスとSRVインデックス）
    MaterialData material;

    // パーティクルのリスト
    std::list<Particle> particles;

    // インスタンシングデータ用SRVインデックス
    uint32_t instancingSrvIndex = 0;

    // インスタンシング用リソース（StructuredBuffer）
    ComPtr<ID3D12Resource> instancingResource = nullptr;

    // 今フレーム描画するインスタンス数
    uint32_t instanceCount = 0;

    // instancingResource を Map したポインタ
    ParticleForGPU *instancingMappedPtr = nullptr;
  };

  // BlendMode
  enum class BlendMode {
    kNone,             // なし
    kNormal,           // 通常
    kAdd,              // 加算
    kSubtract,         // 減算
    kMultiply,         // 乗算
    kScreen,           // スクリーン
    kCountOfBlendMode, // カウント用
  };

  BlendMode blendMode_ = BlendMode::kAdd;

private:
  //================================================================================
  // 外部参照
  //================================================================================

  // DirectXCommonのポインタ
  DirectXCommon *dxCommon_ = nullptr;

  // SrvManagerのポインタ
  ShaderResourceViewManager *srvManager_ = nullptr;

private:
  //================================================================================
  // パーティクル管理データ
  //================================================================================

  // パーティクルグループ
  std::unordered_map<std::string, ParticleGroup> particleGroups_;

  // 1秒分
  static inline const float kDeltaTime = 1.0f / 60.0f;

  //================================================================================
  // GPU リソース
  //================================================================================

  // ルートシグネチャ
  ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

  // グラフィックスパイプラインステート
  ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  //================================================================================
  // GPUリソース（頂点）
  //================================================================================

  // 頂点リソース
  ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
  // バッファリソース内のデータを指すポインタ
  VertexData *vertexData_ = nullptr;
  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

  // 板ポリゴン（パーティクル）描画用の頂点データ
  std::vector<VertexData> vertices_;

private:
  //================================================================================
  // パイプライン構築（RootSignature / PSO）
  //================================================================================

  /// <summary>
  /// ルートシグネチャの生成
  /// </summary>
  void CreateRootSignature();
  /// <summary>
  /// グラフィックスパイプラインの生成
  /// </summary>
  void CreateGraphicsPipeline();

  //================================================================================
  // データ作成処理
  //================================================================================

  /// <summary>
  /// 頂点データの初期化
  /// </summary>
  void InitializeVertexData();
  /// <summary>
  /// 頂点データの作成
  /// </summary>
  void CreateVertexData();

  //================================================================================
  // パーティクル生成
  //================================================================================

  /// <summary>
  /// 1個分のパーティクルをランダム生成
  /// </summary>
  /// <param name="translate">パーティクル生成位置</param>
  /// <returns>生成されたパーティクル</returns>
  Particle MakeParticle(const Vector3 &translate);

  //================================================================================
  // BlendMode
  //================================================================================

  /// <summary>
  /// 指定したブレンドモードに対応
  /// </summary>
  /// <param name="mode">使いたいBlendMode</param>
  /// <returns>ブレンド設定を格納したD3D12_BLEND_DESC</returns>
  D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode);
};
