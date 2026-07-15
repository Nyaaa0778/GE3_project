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

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	// 1. シャープすぎず丸すぎない、メカニカルなエッジにする
	style.WindowRounding = 3.0f;
	style.FrameRounding = 3.0f;
	style.PopupRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 3.0f;

	// 2. ウィンドウのアウトラインだけ残し、中身はフラットに
	style.WindowBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.PopupBorderSize = 1.0f;

	// 3. カラーパレット：ステルスブラック × クリムゾンレッド
	ImVec4* colors = style.Colors;

	// 背景（光を吸収するような深い漆黒）
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.96f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.98f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

	// 枠線（目立ちすぎないダークグレー）
	colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);

	// アクセントカラー（攻撃的でスタイリッシュな深紅）
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.45f, 0.05f, 0.10f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.60f, 0.10f, 0.15f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.45f, 0.05f, 0.10f, 1.00f);

	colors[ImGuiCol_Button] = ImVec4(0.45f, 0.05f, 0.10f, 0.80f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.10f, 0.15f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.02f, 0.05f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.45f, 0.05f, 0.10f, 0.80f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.10f, 0.15f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.02f, 0.05f, 1.00f);

	// チェックマークやスライダー（発光感のある鮮やかな赤）
	colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.20f, 0.25f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.10f, 0.15f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.20f, 0.25f, 1.00f);

	// テキスト（コントラストを効かせたクリアな白）
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);

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
