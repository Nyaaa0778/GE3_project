#include "CityBackground.h"
#include "WireframeObject.h"
#include "Camera.h"
#include <cmath>
#include <random>

CityBackground::CityBackground() = default;
CityBackground::~CityBackground() = default;

void CityBackground::Initialize(Camera* camera) {
	camera_ = camera;

	// 1. 地面のネオングリッド生成 (シームレスに繋ぐため前後に2枚配置)
	groundGrid1_ = std::make_unique<WireframeObject>();
	groundGrid1_->Initialize();
	groundGrid1_->SetCamera(camera_);
	groundGrid1_->CreateGrid(gridLength_, 50);
	groundGrid1_->SetPosition({0.0f, 0.0f, gridLength_ * 0.5f});
	groundGrid1_->SetColor({0.0f, 0.85f, 1.0f, 0.6f}); // ネオンシアン (半透明)

	groundGrid2_ = std::make_unique<WireframeObject>();
	groundGrid2_->Initialize();
	groundGrid2_->SetCamera(camera_);
	groundGrid2_->CreateGrid(gridLength_, 50);
	groundGrid2_->SetPosition({0.0f, 0.0f, gridLength_ * 1.5f});
	groundGrid2_->SetColor({0.0f, 0.85f, 1.0f, 0.6f});

	// 2. 奥の巨大ネオンサン
	neonSun_ = std::make_unique<WireframeObject>();
	neonSun_->Initialize();
	neonSun_->SetCamera(camera_);
	neonSun_->CreateSphere(20.0f, 16);
	neonSun_->SetPosition({0.0f, 12.0f, 120.0f});
	neonSun_->SetColor({1.0f, 0.15f, 0.55f, 0.85f}); // ネオンマゼンタ / ホットピンク

	// 3. サイバーパンクビル群の生成
	GenerateCity();
}

void CityBackground::GenerateCity() {
	buildings_.clear();

	std::mt19937 rng(42); // シードを固定して美しい配置を再現
	std::uniform_real_distribution<float> randDist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> rand01(0.0f, 1.0f);

	// サイバーパンクカラーパレット
	const std::vector<Vector4> palette = {
		{0.0f, 0.95f, 1.0f, 0.9f},  // ネオンシアン
		{1.0f, 0.08f, 0.58f, 0.9f}, // ネオンピンク / マゼンタ
		{0.65f, 0.15f, 1.0f, 0.9f}, // ネオンパープル
		{0.0f, 1.0f, 0.55f, 0.9f},  // ネオングリーン
		{1.0f, 0.75f, 0.0f, 0.9f},  // ネオンアンバー
	};

	const float roadHalfWidth = 6.0f; // 中央道路の幅
	const int numBuildingsPerSide = 25;
	const float minZ = 5.0f;
	const float maxZ = 120.0f;

	for (int side = -1; side <= 1; side += 2) { // -1: 左側, +1: 右側
		for (int i = 0; i < numBuildingsPerSide; ++i) {
			Building b;

			// ビルのサイズ (幅, 高さ, 奥行き)
			float w = 2.5f + rand01(rng) * 3.5f;
			float d = 2.5f + rand01(rng) * 4.0f;
			float h = 6.0f + std::pow(rand01(rng), 1.8f) * 28.0f; // べき乗で低層多め・時々超高層

			// X位置: 道路の外側に多層的に配置
			float layer = rand01(rng);
			float x = side * (roadHalfWidth + w * 0.5f + layer * 25.0f);

			// Z位置
			float z = minZ + ((float)i / (float)numBuildingsPerSide) * (maxZ - minZ) + (randDist(rng) * 2.5f);

			// Y位置 (底面が地面 Y=0 に接するよう高さの半分)
			float y = h * 0.5f;

			b.basePosition = {x, y, z};
			b.scale = {w, h, d};
			b.baseColor = palette[rand() % palette.size()];
			b.pulsePhase = rand01(rng) * 6.28318f;
			b.pulseSpeed = 1.0f + rand01(rng) * 2.0f;

			b.wireObject = std::make_unique<WireframeObject>();
			b.wireObject->Initialize();
			b.wireObject->SetCamera(camera_);
			b.wireObject->CreateBox({1.0f, 1.0f, 1.0f}); // 単位立方体をスケールで拡大
			b.wireObject->SetScale(b.scale);
			b.wireObject->SetPosition(b.basePosition);
			b.wireObject->SetColor(b.baseColor);

			buildings_.push_back(std::move(b));
		}
	}
}

void CityBackground::Update() {
	time_ += 0.016f;

	// 1. 地面グリッドのスクロール
	if (scrollSpeed_ > 0.0f) {
		gridZ_ -= scrollSpeed_ * 0.016f;
		if (gridZ_ <= -gridLength_) {
			gridZ_ += gridLength_;
		}
	}
	if (groundGrid1_) {
		groundGrid1_->SetPosition({0.0f, 0.0f, gridZ_ + gridLength_ * 0.5f});
		groundGrid1_->Update();
	}
	if (groundGrid2_) {
		groundGrid2_->SetPosition({0.0f, 0.0f, gridZ_ + gridLength_ * 1.5f});
		groundGrid2_->Update();
	}

	// 2. 奥の巨大ネオンサンの微小回転
	if (neonSun_) {
		neonSun_->SetRotation({0.0f, time_ * 0.1f, 0.0f});
		neonSun_->Update();
	}

	// 3. ビル群のスクロールとネオン明滅アニメーション
	const float minZ = -10.0f;
	const float maxZ = 120.0f;

	for (auto& b : buildings_) {
		// スクロール移動 (速度が0より大きい場合のみ)
		if (scrollSpeed_ > 0.0f) {
			b.basePosition.z -= scrollSpeed_ * 0.016f;
			if (b.basePosition.z < minZ) {
				b.basePosition.z += (maxZ - minZ);
			}
			b.wireObject->SetPosition(b.basePosition);
		}

		// ネオンの呼吸（穏やかなパルス発光のみ、不快なチカチカ点滅は削除）
		if (enablePulse_) {
			float pulse = 0.85f + 0.15f * std::sin(time_ * b.pulseSpeed + b.pulsePhase);
			Vector4 currentColor = {
				b.baseColor.x * pulse,
				b.baseColor.y * pulse,
				b.baseColor.z * pulse,
				b.baseColor.w
			};
			b.wireObject->SetColor(currentColor);
		}

		b.wireObject->Update();
	}
}

void CityBackground::Draw() {
	// 地面
	if (groundGrid1_) groundGrid1_->Draw();
	if (groundGrid2_) groundGrid2_->Draw();

	// ネオンサン
	if (neonSun_) neonSun_->Draw();

	// ビル群
	for (auto& b : buildings_) {
		if (b.wireObject) {
			b.wireObject->Draw();
		}
	}
}

void CityBackground::SetCamera(Camera* camera) {
	camera_ = camera;
	if (groundGrid1_) groundGrid1_->SetCamera(camera);
	if (groundGrid2_) groundGrid2_->SetCamera(camera);
	if (neonSun_) neonSun_->SetCamera(camera);
	for (auto& b : buildings_) {
		if (b.wireObject) {
			b.wireObject->SetCamera(camera);
		}
	}
}
