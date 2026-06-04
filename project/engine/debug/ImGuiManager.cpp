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
		// デフォルトフォントを登録
		if (io.Fonts->Fonts.empty()) {
			io.Fonts->AddFontDefault();
		}
		// フォントアトラスを明示的にビルド
		io.Fonts->Build();
	}

	// モダンスタイリッシュテーマの適用
	ImGuiStyle& style = ImGui::GetStyle();
	
	// パディング・サイズ調整
	style.WindowPadding = ImVec2(15.0f, 15.0f);
	style.FramePadding = ImVec2(8.0f, 6.0f);
	style.CellPadding = ImVec2(6.0f, 6.0f);
	style.ItemSpacing = ImVec2(10.0f, 10.0f);
	style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	style.IndentSpacing = 20.0f;
	style.ScrollbarSize = 12.0f;
	style.GrabMinSize = 12.0f;
	
	// 丸み（コーナー半径）
	style.WindowRounding = 10.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 6.0f;
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 6.0f;
	style.TabRounding = 6.0f;
	
	// 枠線
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;
	
	// カラーパレット (洗練されたダーク＆ネオンブルーアクセント)
	ImVec4* colors = style.Colors;
	
	colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.58f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.08f, 0.11f, 0.95f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.16f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.11f, 0.98f);
	colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.28f, 0.60f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	
	colors[ImGuiCol_FrameBg]                = ImVec4(0.15f, 0.15f, 0.22f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.22f, 0.22f, 0.32f, 1.00f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.28f, 0.40f, 1.00f);
	
	colors[ImGuiCol_TitleBg]                = ImVec4(0.06f, 0.06f, 0.09f, 1.00f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.10f, 0.10f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	
	// ネオンブルーアクセントカラー (Check, Slider, Grab, Tabs, Headers)
	ImVec4 accentColor                      = ImVec4(0.00f, 0.70f, 1.00f, 1.00f);
	ImVec4 accentHovered                    = ImVec4(0.20f, 0.80f, 1.00f, 1.00f);
	ImVec4 accentActive                     = ImVec4(0.00f, 0.60f, 0.90f, 1.00f);
	
	colors[ImGuiCol_CheckMark]              = accentColor;
	colors[ImGuiCol_SliderGrab]             = accentColor;
	colors[ImGuiCol_SliderGrabActive]       = accentActive;
	
	colors[ImGuiCol_Button]                 = ImVec4(0.16f, 0.40f, 0.60f, 0.80f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.20f, 0.50f, 0.75f, 0.90f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.14f, 0.35f, 0.52f, 1.00f);
	
	colors[ImGuiCol_Header]                 = ImVec4(0.15f, 0.35f, 0.55f, 0.60f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.20f, 0.45f, 0.70f, 0.80f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.15f, 0.35f, 0.55f, 1.00f);
	
	colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.20f, 0.28f, 0.60f);
	colors[ImGuiCol_SeparatorHovered]       = accentHovered;
	colors[ImGuiCol_SeparatorActive]        = accentActive;
	
	colors[ImGuiCol_ResizeGrip]             = ImVec4(0.20f, 0.45f, 0.70f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered]      = accentHovered;
	colors[ImGuiCol_ResizeGripActive]       = accentActive;
	
	colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.18f, 0.26f, 0.86f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.20f, 0.35f, 0.55f, 0.80f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.16f, 0.28f, 0.44f, 1.00f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 0.90f);
	
	colors[ImGuiCol_PlotLines]              = accentColor;
	colors[ImGuiCol_PlotLinesHovered]       = accentHovered;
	colors[ImGuiCol_PlotHistogram]          = accentColor;
	colors[ImGuiCol_PlotHistogramHovered]   = accentHovered;
	
	colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = accentColor;
	colors[ImGuiCol_NavHighlight]           = accentColor;
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.05f, 0.05f, 0.08f, 0.60f);

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
	if (!isVisible_) {
		return;
	}

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
