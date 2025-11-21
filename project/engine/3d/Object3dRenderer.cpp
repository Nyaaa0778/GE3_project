#include "Object3dRenderer.h"
#include "DirectXCommon.h"

#include <cassert>
#include <d3d12.h>

//================================================================================
// 初期化 / 描画設定
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
void Object3dRenderer::Initialize(DirectXCommon *dxCommon) {
  // 引数で受け取ってメンバ変数に記録する
  dxCommon_ = dxCommon;

  // グラフィックスパイプラインの生成
  CreateGraphicsPipeline();
}

/// <summary>
/// 共通描画設定
/// </summary>
void Object3dRenderer::SetupCommonRenderState() {
  // ルートシグネチャをセット
  dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
  // グラフィックスパイプラインステートをセット
  dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get());
  // プリミティブトポロジーをセット
  dxCommon_->GetCommandList()->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

//================================================================================
// パイプライン構築（RootSignature / PSO）
//================================================================================

/// <summary>
/// ルートシグネチャを作成
/// </summary>
void Object3dRenderer::CreateRootSignature() {
  // RootSignature作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // RootParamenterを作成、複数設定できるから配列(今回は結果1つなので長さ1の配列)
  D3D12_ROOT_PARAMETER rootParameters[4] = {};
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使用
  rootParameters[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderで使う
  rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド
  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使用
  rootParameters[1].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_VERTEX;              // PixelShaderで使う
  rootParameters[1].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0;                      // 0から
  descriptorRange[0].NumDescriptors = 1;                          // 数は1つ
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

  rootParameters[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
  rootParameters[2].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderを使う
  rootParameters[2].DescriptorTable.pDescriptorRanges =
      descriptorRange; // Tableの中身に配列を指定
  rootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange); // Tableで利用する数

  rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameters[3].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderを使う
  rootParameters[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う

  descriptionRootSignature.pParameters =
      rootParameters; // ルートパラメータ配列へのポインタ
  descriptionRootSignature.NumParameters =
      _countof(rootParameters); // 配列の長さ

  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  staticSamplers[0].Filter =
      D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
  staticSamplers[0].AddressU =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // たくさんのMipmapを使う
  staticSamplers[0].ShaderRegister = 0;
  // レジスタ番号0を使う staticSamplers[0].ShaderVisibility =
  D3D12_SHADER_VISIBILITY_PIXEL;
  descriptionRootSignature.pStaticSamplers = staticSamplers;
  descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

  // シリアライズしてバイナリする
  ID3DBlob *signatureBlob = nullptr;
  ID3DBlob *errorBlob = nullptr;
  HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
                                           D3D_ROOT_SIGNATURE_VERSION_1,
                                           &signatureBlob, &errorBlob);
  if (FAILED(hr)) {
    /*Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));*/
    assert(false);
  }
  // バイナリをもとに生成
  hr = dxCommon_->GetDevice()->CreateRootSignature(
      0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&rootSignature_));
  assert(SUCCEEDED(hr));
}
/// <summary>
/// グラフィックスパイプラインの生成
/// </summary>
void Object3dRenderer::CreateGraphicsPipeline() {
  // ルートシグネチャを生成
  CreateRootSignature();

  // InputLayout
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
  inputElementDescs[0].SemanticName = "POSITION";
  inputElementDescs[0].SemanticIndex = 0;
  inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[1].SemanticName = "TEXCOORD";
  inputElementDescs[1].SemanticIndex = 0;
  inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  inputElementDescs[2].SemanticName = "NORMAL";
  inputElementDescs[2].SemanticIndex = 0;
  inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
  D3D12_INPUT_LAYOUT_DESC inputLayOutDesc{};
  inputLayOutDesc.pInputElementDescs = inputElementDescs;
  inputLayOutDesc.NumElements = _countof(inputElementDescs);

  // BlendStateの設定
  D3D12_BLEND_DESC blendDesc{};
  // すべての色要素を書き込む
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasterizerStateの設定
  D3D12_RASTERIZER_DESC rasterizerDesc{};
  // 裏面(時計回り)を表示しない
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  // 三角形の中を塗りつぶす
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  // Shaderをコンパイル
  ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(
      L"resources/shaders/Object3d.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(
      L"resources/shaders/Object3d.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
  graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
  graphicsPipelineStateDesc.InputLayout = inputLayOutDesc;
  graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                  vertexShaderBlob->GetBufferSize()};
  graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                  pixelShaderBlob->GetBufferSize()};
  graphicsPipelineStateDesc.BlendState = blendDesc;
  graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

  // DepthStencilの設定
  graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // 書き込むRTVの情報
  graphicsPipelineStateDesc.NumRenderTargets = 1;
  graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  // 利用するトポロジ(形状)のタイプ、三角形
  graphicsPipelineStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  // どのように画面に色を打ち込むのか設定
  graphicsPipelineStateDesc.SampleDesc.Count = 1;
  graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
  // 実際に生成
  HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
      &graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
  assert(SUCCEEDED(hr));
}
