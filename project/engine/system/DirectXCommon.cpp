#include "DirectXCommon.h"

/// <summary>
/// 初期化
/// </summary>
void DirectXCommon::Initialize() {
  // DXGIファクトリーの生成
  ComPtr<IDXGIFactory7> dxgiFactory = nullptr;

  // HRESULTはWindows系のエラーコードであり、
  // 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
  HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
  // 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
  // どうにもできない場合が多いのでassertにしておく
  assert(SUCCEEDED(hr));

  // 使用するアダプタ用の変数、最初にnullptrを入れる
  ComPtr<IDXGIAdapter4> useAdapter = nullptr;
  // いい順にアダプタを読む
  for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
                       i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                       IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND;
       i++) {
    // アダプターの情報を取得する
    DXGI_ADAPTER_DESC3 adapterDesc{};
    hr = useAdapter->GetDesc3(&adapterDesc);
    assert(SUCCEEDED(hr));
    // ソフトウェアアダプタでなければ採用
    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
      // 採用したアダプタの情報をログに出力(wstring)
      Log(std::format(L"Use Adapter:{}\n", adapterDesc.Description));
      break;
    }
    useAdapter = nullptr;
  }
  // 適切なアダプタが見つからなかったら、起動できない
  assert(useAdapter != nullptr);

  ComPtr<ID3D12Device> device = nullptr;
  // 機能レベルとログの出力用の文字列
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0};
  const char *featureLevelString[] = {"12.2", "12.1", "12.0"};
  // 高い順に生成できるか？
  for (size_t i = 0; i < _countof(featureLevels); ++i) {
    // 採用したアダプターでデバイスを作成
    hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i],
                           IID_PPV_ARGS(&device));
    // 指定した機能レベルでデバイスが生成できたかを確認
    if (SUCCEEDED(hr)) {
      // 生成できたのでログ出力を行ってループを抜ける
      Log(std::format("FeatureLevel : {}\n", featureLevelString[i]));
      break;
    }
  }

  // デバイスの生成がうまくいかなかったので起動できない
  assert(device != nullptr);
  // 初期化完了用のログを出す
  Log("Complete create D3D12Device!!!\n");
}
