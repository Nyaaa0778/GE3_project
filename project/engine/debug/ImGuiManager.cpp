#include "ImGuiManager.h"

#include "DirectXCommon.h"
#include "ShaderResourceViewManager.h"
#include "WinApp.h"

//================================================================================
// シングルトン
//================================================================================

std::unique_ptr<ImGuiManager> ImGuiManager::instance = nullptr;

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
/// <returns>ImGuiManagerの唯一のインスタンス</returns>
ImGuiManager* ImGuiManager::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<ImGuiManager>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void ImGuiManager::Finalize() { instance.reset(); }

/// <summary>
/// デストラクタ
/// </summary>
ImGuiManager::~ImGuiManager() {
#ifdef USE_IMGUI

	// ImGuiの終了処理、初期化と逆順に行う
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#endif
}

/// <summary>
/// 初期化
/// </summary>
void ImGuiManager::Initialize(
	[[maybe_unused]] DirectXCommon* dxCommon,
	[[maybe_unused]] ShaderResourceViewManager* srvManager) {

	// メンバ変数を記録
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;

#ifdef USE_IMGUI

	uint32_t fontSrvIndex = ShaderResourceViewManager::GetInstance()->Allocate();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	{
		ImGuiIO& io = ImGui::GetIO();

		// Fira Mono フォント（英数字用）をロード
		ImFont* font = io.Fonts->AddFontFromFileTTF("resources/fonts/FiraMono-Regular.ttf", 16.0f);

		// Fira Mono がロードできなかった場合のフォールバック
		if (font == nullptr) {
			io.Fonts->AddFontDefault();
		}

		// 日本語フォントをマージしてロードするための設定
		ImFontConfig config;
		config.MergeMode = true; // 既存のフォント（FiraMono）にマージする

		// Windows 標準の日本語フォント（例: ＭＳ ゴシック）
		const char* jpFontPath = "resources/fonts/msgothic.ttc";

		// 日本語のグリフレンジを指定してロード
		io.Fonts->AddFontFromFileTTF(jpFontPath, 16.0f, &config, io.Fonts->GetGlyphRangesJapanese());

		// フォントアトラスを明示的にビルド
		io.Fonts->Build();
	}

	ImGui::StyleColorsClassic();
	ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHwnd());
	ImGui_ImplDX12_Init(
		DirectXCommon::GetInstance()->GetDevice(),
		static_cast<int>(DirectXCommon::GetInstance()->GetSwapChainResourceNum()),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		ShaderResourceViewManager::GetInstance()->GetDescriptorHeap(),
		ShaderResourceViewManager::GetInstance()->GetCPUDescriptorHandle(
			fontSrvIndex),
		ShaderResourceViewManager::GetInstance()->GetGPUDescriptorHandle(
			fontSrvIndex));

#endif
}

/// <summary>
/// ImGui受付開始
/// </summary>
void ImGuiManager::Begin() {
#ifdef USE_IMGUI

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

#endif
}
/// <summary>
/// ImGui受付終了
/// </summary>
void ImGuiManager::End() {
#ifdef USE_IMGUI

	ImGui::Render();

#endif
}

/// <summary>
/// 描画
/// </summary>
void ImGuiManager::Draw() {
#ifdef USE_IMGUI

	ID3D12GraphicsCommandList* commandList =
		DirectXCommon::GetInstance()->GetCommandList();

	// デスクリプタヒープの配列をセットするコマンド
	ID3D12DescriptorHeap* ppHeaps[] = {
		ShaderResourceViewManager::GetInstance()->GetDescriptorHeap()};
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	// 描画コマンド発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif
}
