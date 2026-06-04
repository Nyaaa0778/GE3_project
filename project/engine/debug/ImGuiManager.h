#pragma once

#ifdef USE_IMGUI

#include "../externals/imgui/imgui.h"
#include "../externals/imgui/imgui_impl_dx12.h"
#include "../externals/imgui/imgui_impl_win32.h"

#endif

#include <memory>

class DirectXCommon;
class ShaderResourceViewManager;

class ImGuiManager {
public:
	//================================================================================
	// シングルトン
	//================================================================================

	// 唯一のインスタンス取得
	static ImGuiManager* GetInstance();

	/// <summary>
	/// コンストラクタ
	/// </summary>
	ImGuiManager() = default;
	/// <summary>
	/// デストラクタ
	/// </summary>
	~ImGuiManager();

	/// <summary>
	/// 終了
	/// </summary>
	static void Finalize();

	// unique_ptrからの削除を許可
	friend std::default_delete<ImGuiManager>;

private:
	static std::unique_ptr<ImGuiManager> instance;

	/// <summary>
	/// コピーコンストラクタ禁止
	/// </summary>
	/// <param name="">コピー元（使用不可）</param>
	ImGuiManager(ImGuiManager&) = delete;
	/// <summary>
	/// 代入演算子禁止
	/// </summary>
	/// <param name="">代入元（使用不可）</param>
	/// <returns>このオブジェクトを返す</returns>
	ImGuiManager& operator=(ImGuiManager&) = delete;

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon,
		ShaderResourceViewManager* srvManager);

	/// <summary>
	/// ImGui受付開始
	/// </summary>
	void Begin();
	/// <summary>
	/// ImGui受付終了
	/// </summary>
	void End();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 描画の表示・非表示を切り替える
	/// </summary>
	void ToggleVisibility() { isVisible_ = !isVisible_; }

	/// <summary>
	/// 表示状態を取得する
	/// </summary>
	bool IsVisible() const { return isVisible_; }

	/// <summary>
	/// 表示状態を設定する
	/// </summary>
	void SetVisible(bool visible) { isVisible_ = visible; }

private:
	//================================================================================
	// 外部参照
	//================================================================================

	// DirectXCommonのポインタ
	DirectXCommon* dxCommon_ = nullptr;
	// SrvManagerのポインタ
	ShaderResourceViewManager* srvManager_ = nullptr;

	// 表示・非表示フラグ
	bool isVisible_ = true;
};
