#include "ParticleManager.h"

#include "DirectXCommon.h"
#include "MathUtility.h"
#include "Random.h"
#include "ShaderResourceViewManager.h"
#include "TextureManager.h"

#include <cassert>
#include <numbers>

using namespace MathUtility;

std::unique_ptr<ParticleManager> ParticleManager::instance = nullptr;

//================================================================================
// シングルトン
//================================================================================

/// <summary>
/// シングルトンインスタンスの取得
/// </summary>
ParticleManager* ParticleManager::GetInstance() {
	if (instance == nullptr) {
		instance = std::make_unique<ParticleManager>();
	}

	return instance.get();
}

/// <summary>
/// 終了
/// </summary>
void ParticleManager::Finalize() { instance.reset(); }

//================================================================================
// 初期化 / 更新 / 描画
//================================================================================

/// <summary>
/// 初期化
/// </summary>
/// <param name="dxCommon">DirectXCommonのポインタ</param>
/// <param name="srvManager">SrvManagerのポインタ</param>
void ParticleManager::Initialize(DirectXCommon* dxCommon,
	ShaderResourceViewManager* srvManager) {
	assert(dxCommon);
	assert(srvManager);

	// 引数で受け取ってメンバ変数に保存

	dxCommon_ = dxCommon;
	srvManager_ = srvManager;

	// 頂点データの初期化
	InitializeVertexData();

	// 頂点データの作成
	CreateVertexData();

	//==================================================
	// パイプライン生成
	//==================================================
	CreateGraphicsPipeline();
}

/// <summary>
/// 更新
/// </summary>
/// <param name="viewmatrix">カメラのビュー行列</param>
/// <param name="projectionMatrix">カメラの射影行列</param>
void ParticleManager::Update(const Matrix4x4& viewMatrix,
	const Matrix4x4& projectionMatrix) {
	const Matrix4x4 cameraWorld = MakeInverseMatrix(viewMatrix);

	Matrix4x4 billboardMatrix = MakeIdentityMatrix();
	const Matrix4x4 backToFront = MakeRotateYMatrix(std::numbers::pi_v<float>);
	billboardMatrix = Multiply(backToFront, cameraWorld);

	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	// ビュー行列と投影行列を合成
	const Matrix4x4 vp = Multiply(viewMatrix, projectionMatrix);

	// 全パーティクルグループを処理
	for (auto& [groupName, group] : particleGroups_) {
		group.instanceCount = 0;

		// グループ内の全パーティクルを処理
		for (auto it = group.particles.begin(); it != group.particles.end();) {
			Particle& p = *it;

			// ---- 寿命チェック（寿命に達してたら外す）----
			if (p.currentTime >= p.lifeTime) {
				it = group.particles.erase(it);
				continue;
			}

			//// ---- 場の影響（加速）----
			//// ※ accelerationField_ / IsCollision が無いならここはコメントアウトでOK
			// if (useAccelerationField_) {
			//   if (IsCollision(accelerationField_.area, p.transform.translate)) {
			//     p.velocity.x += accelerationField_.acceleration.x * dt;
			//     p.velocity.y += accelerationField_.acceleration.y * dt;
			//     p.velocity.z += accelerationField_.acceleration.z * dt;
			//   }
			// }

			// ---- 移動処理（速度を座標に加算）----
			p.transform.translation += p.velocity * kDeltaTime;

			// ---- 経過時間を加算 ----
			p.currentTime += kDeltaTime;

			// ---- World行列を計算（ビルボード固定）----
			const Matrix4x4 S = MakeScaleMatrix(p.transform.scale);
			const Matrix4x4 T = MakeTranslateMatrix(p.transform.translation);
			const Matrix4x4 worldMatrix = Multiply(Multiply(S, billboardMatrix), T);

			// ---- WVPを合成 ----
			const Matrix4x4 wvp = Multiply(worldMatrix, vp);

			// ---- インスタンシング用データ1個分を書き込み ----
			if (group.instanceCount < kMaxInstancePerGroup) {
				ParticleForGPU& out = group.instancingMappedPtr[group.instanceCount];
				out.World = worldMatrix;
				out.WVP = wvp;
				out.color = p.color;

				// フェードアウトしたいなら（画像には明記ないけど、よくやる）
				const float alpha = 1.0f - (p.currentTime / p.lifeTime);
				out.color.w = alpha;

				group.instanceCount++;
			}

			++it;
		}
	}
}

/// <summary>
/// 描画
/// </summary>
void ParticleManager::Draw() {
	assert(dxCommon_);
	assert(srvManager_);

	ID3D12GraphicsCommandList* cmd = dxCommon_->GetCommandList();
	assert(cmd);

	// コマンド：ルートシグネチャを設定
	cmd->SetGraphicsRootSignature(rootSignature_.Get());

	// コマンド：PSO を設定
	cmd->SetPipelineState(graphicsPipelineState_.Get());

	// コマンド：プリミティブトポロジー（描画形状）を設定
	// ※4頂点の板ポリなので TRIANGLESTRIP が自然
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// コマンド：VBV を設定
	cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// （RootParameter[0]のMaterial CBV と [3]ライトCBV を使うシェーダなら、ここで
	// SetGraphicsRootConstantBufferView を入れる）

	// 全てのパーティクルグループについて処理（1グループ=1DrawCall）
	for (auto& [name, group] : particleGroups_) {
		if (group.instanceCount == 0) {
			continue;
		}

		// コマンド：テクスチャのSRVのDescriptorTableを設定（root[1]）
		cmd->SetGraphicsRootDescriptorTable(
			1, srvManager_->GetGPUDescriptorHandle(group.material.textureSrvIndex));

		// コマンド：インスタンシングデータのSRVのDescriptorTableを設定（root[0]）
		cmd->SetGraphicsRootDescriptorTable(
			0, srvManager_->GetGPUDescriptorHandle(group.instancingSrvIndex));

		// コマンド：DrawCall（インスタンシング描画）
		cmd->DrawInstanced(static_cast<UINT>(vertices_.size()), group.instanceCount,
			0, 0);
	}
}

//================================================================================
// パイプライン構築（RootSignature / PSO）
//================================================================================

/// <summary>
/// ルートシグネチャの生成
/// </summary>
void ParticleManager::CreateRootSignature() {
	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// RootParameterを作成、複数設定できるから配列(今回は4つ)
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// DescriptorRange（SRV用）
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ★ root[0] : インスタンシング用 SRV (t0, VS)
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges =
		_countof(descriptorRange);

	// ★ root[1] : テクスチャ SRV (t0, PS)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[1].DescriptorTable.NumDescriptorRanges =
		_countof(descriptorRange);

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
	staticSamplers[0].ShaderRegister = 0;         // レジスタ番号0を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリ化する
	ComPtr<ID3DBlob> signatureBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1,
		signatureBlob.GetAddressOf(), errorBlob.GetAddressOf());

	if (FAILED(hr)) {
		// Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
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
void ParticleManager::CreateGraphicsPipeline() {
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
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayOutDesc {};
	inputLayOutDesc.pInputElementDescs = inputElementDescs;
	inputLayOutDesc.NumElements = _countof(inputElementDescs);

	blendMode_ = BlendMode::kAdd;
	// BlendMode prevBlendMode = blendMode_;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc {};
	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイル
	ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(
		L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(
		L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc {};
	// Depthの機能を有効化
	depthStencilDesc.DepthEnable = false;
	// 書き込む（※あなたの元コードは ZERO だったので、必要ならここを ZERO に戻す）
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	// 比較関数はLessEqual、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc {};
	graphicsPipelineStateDesc.pRootSignature =
		rootSignature_.Get(); // 例に寄せるなら rootSignature_ に統一してもOK
	graphicsPipelineStateDesc.InputLayout = inputLayOutDesc;
	graphicsPipelineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize()};
	graphicsPipelineStateDesc.BlendState = MakeBlendDesc(blendMode_);
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	// DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
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

//================================================================================
// データ作成処理
//================================================================================

/// <summary>
/// 頂点データの初期化
/// </summary>
void ParticleManager::InitializeVertexData() {
	vertices_.clear();
	vertices_.reserve(4);

	vertices_.push_back(
		{{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0, 0, -1}}); // 左上
	vertices_.push_back(
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, -1}}); // 左下
	vertices_.push_back(
		{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0, 0, -1}}); // 右上
	vertices_.push_back(
		{{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0, 0, -1}}); // 右下
}
/// <summary>
/// 頂点データの作成
/// </summary>
void ParticleManager::CreateVertexData() {
	// vertexResourceを作成
	const size_t vertexBufferSize = sizeof(VertexData) * vertices_.size();

	vertexBuffer_ = dxCommon_->CreateBufferResource(vertexBufferSize);

	// vertexBufferViewを作成
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(vertexBufferSize);
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// vertexResourceに頂点データを書き込む
	void* mappedVertex = nullptr;
	vertexBuffer_->Map(0, nullptr, &mappedVertex);
	std::memcpy(mappedVertex, vertices_.data(), vertexBufferSize);
	vertexBuffer_->Unmap(0, nullptr);
}

//================================================================================
// パーティクルグループ作成 / パーティクル生成
//================================================================================

/// <summary>
/// パーティクルグループを作成
/// </summary>
/// <param name="name">作成するパーティクルグループ名</param>
/// <param
/// name="textureFilePath">グループで使用するテクスチャのファイルパス</param>
void ParticleManager::CreateParticleGroup(const std::string groupName,
	const std::string textureFilePath) {
	// 登録済みの名前かチェック
	const bool alreadyExists =
		(particleGroups_.find(groupName) != particleGroups_.end());
	assert(!alreadyExists);

	// 新たに空っぽのパーティクルグループを作成し、コンテナに登録

	ParticleGroup newGroup {};
	particleGroups_.emplace(groupName, std::move(newGroup));

	// 以降は登録した実体を参照して初期化していく
	ParticleGroup& group = particleGroups_.at(groupName);

	// 新たなパーティクルグループの初期化
	group.material.textureFilePath = textureFilePath;

	// SRVindexを設定
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	group.material.textureSrvIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

	// インスタンシング用リソース生成
	const size_t instanceBufferSize =
		sizeof(ParticleForGPU) * kMaxInstancePerGroup;

	group.instancingResource =
		dxCommon_->CreateBufferResource(instanceBufferSize);
	assert(group.instancingResource);

	// インスタンス数初期化
	group.instanceCount = 0;

	// instancingResource を Map して書き込みポインタ取得
	group.instancingResource->Map(
		0, nullptr, reinterpret_cast<void**>(&group.instancingMappedPtr));

	// インスタンシング用にSRV確保してSRVインデックス記録
	group.instancingSrvIndex = srvManager_->Allocate();

	// SRV生成
	srvManager_->CreateSRVforStructureBuffer(
		group.instancingSrvIndex, group.instancingResource.Get(),
		kMaxInstancePerGroup, sizeof(ParticleForGPU));
}

/// <summary>
/// 指定したパーティクルグループからパーティクルを発生させる
/// </summary>
/// <param name="groupName">発生させたいパーティクルグループ名</param>
/// <param name="emitPosition">パーティクルの発生位置</param>
/// <param name="count">発生させるパーティクル数</param>
void ParticleManager::Emit(const std::string groupName,
	const Vector3& emitPosition, uint32_t count) {
	// 登録済みのグループかチェックしてassert
	auto it = particleGroups_.find(groupName);
	assert(it != particleGroups_.end() && "Particle group not found.");

	ParticleGroup& group = it->second;

	// 新しいパーティクルを作成し、指定グループに登録
	for (uint32_t i = 0; i < count; ++i) {
		if (group.particles.size() >= kMaxInstancePerGroup) {
			break; // 上限以上は積まない
		}
		group.particles.push_back(MakeParticle(emitPosition));
	}
}

/// <summary>
/// 1個分のパーティクルをランダム生成
/// </summary>
/// <param name="translate">パーティクル生成位置</param>
/// <returns>生成されたパーティクル</returns>
ParticleManager::Particle
ParticleManager::MakeParticle(const Vector3& translate) {
	// -1.0f ～ 1.0f の一様乱数を使う
	Particle particle {};

	particle.transform.scale = {1.0f, 1.0f, 1.0f};
	particle.transform.rotation = {0.0f, 0.0f, 0.0f};

	Vector3 randomTranslate = Random::RangeVector3(-1.0f, 1.0f);

	// 位置をランダム配置
	particle.transform.translation = {
		randomTranslate.x + translate.x, // x
		randomTranslate.y + translate.y, // y
		randomTranslate.z + translate.z  // z
	};

	// 速度もランダム
	particle.velocity = Random::RangeVector3(-1.0f, 1.0f);

	particle.color = {Random::RangeFloat(0.0f, 1.0f),
		Random::RangeFloat(0.0f, 1.0f),
		Random::RangeFloat(0.0f, 1.0f), 1.0f};

	particle.lifeTime = Random::RangeFloat(1.0f, 3.0f);
	particle.currentTime = 0;

	return particle;
}

//================================================================================
// BlendMode
//================================================================================

/// <summary>
/// 指定したブレンドモードに対応
/// </summary>
/// <param name="mode">使いたいBlendMode</param>
/// <returns>ブレンド設定を格納したD3D12_BLEND_DESC</returns>
D3D12_BLEND_DESC ParticleManager::MakeBlendDesc(BlendMode mode) {
	D3D12_BLEND_DESC blendDesc {};
	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	switch (mode) {
	case BlendMode::kNone:
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		break;

	case BlendMode::kNormal:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		break;

	case BlendMode::kAdd:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;

	case BlendMode::kSubtract:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;

	case BlendMode::kMultiply:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
		break;

	case BlendMode::kScreen:
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		break;
	}

	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	return blendDesc;
}
