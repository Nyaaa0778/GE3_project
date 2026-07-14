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

		// FiraMonoを英語・数字用の主要フォントとして読み込む
		ImFont* font = io.Fonts->AddFontFromFileTTF("resources/fonts/FiraMono-Regular.ttf", 15.0f);
		if (font == nullptr) {
			// 読み込み失敗時はデフォルトフォントを使用
			io.Fonts->AddFontDefault();
		} else {
			// 日本語表示用にWindowsのシステムフォントをマージする
			ImFontConfig config;
			config.MergeMode = true;
			config.PixelSnapH = true;
			const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesJapanese();

			// メイリオの読み込みを試みる
			if (io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\meiryo.ttc", 15.0f, &config, glyphRanges) == nullptr) {
				// メイリオがない場合はMSゴシックを読み込む
				io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msgothic.ttc", 15.0f, &config, glyphRanges);
			}
		}

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
