#include "GamePlayScene.h"

#include <MyEngine.h>
#include "LevelLoader.h"
#include "MathUtility.h"
#include "LightManager.h"
#include "Input.h"
#include <thread>
#include <chrono>

GamePlayScene::GamePlayScene() = default;

GamePlayScene::~GamePlayScene() {
	Finalize();
}

void GamePlayScene::Initialize() {
	// カメラのインスタンス生成
	camera_ = std::make_unique<Camera>();

	// レベルデータのロード
	LevelLoader loader;
	std::unique_ptr<LevelData> levelData(loader.Load("TL1Sample"));

	// ロードしたカメラパラメータ（位置・回転）があれば設定、なければデフォルト値
	if (!levelData->cameras.empty()) {
		camera_->SetTranslate(levelData->cameras[0].translation);
		camera_->SetRotate(levelData->cameras[0].rotation);
	} else {
		camera_->SetRotate({0.3f, 0.0f, 0.0f});
		camera_->SetTranslate({0.0f, 6.0f, -20.0f});
	}
	camera_->CalculateMatrix();
	camera_->CreateConstantBuffer();

	// ロードしたライトパラメータ（位置・回転）があれば設定
	if (!levelData->lights.empty()) {
		const auto& lightData = levelData->lights[0];
		LightManager::GetInstance()->SetLocalLightPosition(lightData.translation);
		LightManager::GetInstance()->SetLocalLightIntensity(1.0f);
		LightManager::GetInstance()->SetLocalLightDistance(50.0f);
		LightManager::GetInstance()->SetDirectionalLightIntensity(0.0f);
	}

	// object3dの初期化 (Player)
	obj_ = std::make_unique<Object3d>();
	obj_->Initialize("sphere_player");
	obj_->SetCamera(camera_.get());
	obj_->SetColor({1.0f, 1.0f, 1.0f, 1.0f});

	// スナップショットの作成用バッファ
	SnapshotData snapshot;

	// 読み込んだレベルデータのオブジェクトを生成・初期化 (静的背景オブジェクト)
	for (const auto& objectData : levelData->objects) {
		if (objectData.filename.empty()) {
			continue;
		}
		auto newObj = std::make_unique<Object3d>();
		newObj->Initialize(objectData.filename);
		newObj->SetPosition(objectData.translation);
		newObj->SetRotation(objectData.rotation);
		newObj->SetScale(objectData.scaling);
		newObj->SetCamera(camera_.get());
		objects_.push_back(std::move(newObj));

		// スナップショットに追加
		SnapshotObject sobj;
		sobj.name = objectData.filename;
		sobj.filename = objectData.filename;
		sobj.translation = objectData.translation;
		sobj.rotation = objectData.rotation;
		sobj.scaling = objectData.scaling;
		snapshot.staticObjects.push_back(sobj);
	}

	for (const auto& spawner : levelData->spawners) {
		// ① プレイヤーの場合
		if (spawner.entityType.find("Player") != std::string::npos) {
			obj_->SetPosition(spawner.translation);
			obj_->SetRotation(spawner.rotation);
		}
		// ② 敵の場合
		else if (spawner.entityType.find("Enemy") != std::string::npos) {
			auto enemy = std::make_unique<Object3d>();
			enemy->Initialize("sphere_enemy");
			enemy->SetPosition(spawner.translation);
			enemy->SetRotation(spawner.rotation);
			enemy->SetCamera(camera_.get());
			enemy->SetColor({1.0f, 0.2f, 0.2f, 1.0f}); // 赤い敵
			enemies_.push_back(std::move(enemy));
			enemyInitialPositions_.push_back(spawner.translation);
		}
		// ③ アイテムの場合
		else if (spawner.entityType.find("Item") != std::string::npos) {
			auto item = std::make_unique<Object3d>();
			item->Initialize("cube");
			item->SetPosition(spawner.translation);
			item->SetRotation(spawner.rotation);
			item->SetCamera(camera_.get());
			item->SetColor({0.2f, 0.8f, 0.2f, 1.0f});
			objects_.push_back(std::move(item));
		}
	}

	// もしレベルデータに敵がいなかった場合のフォールバック（デバッグ用に必ず1体配置する）
	if (enemies_.empty()) {
		auto enemy = std::make_unique<Object3d>();
		enemy->Initialize("sphere_enemy");
		enemy->SetPosition({0.0f, 0.0f, 6.0f});
		enemy->SetRotation({0.0f, 0.0f, 0.0f});
		enemy->SetCamera(camera_.get());
		enemy->SetColor({1.0f, 0.2f, 0.2f, 1.0f}); // 赤い敵
		enemies_.push_back(std::move(enemy));
		enemyInitialPositions_.push_back({0.0f, 0.0f, 6.0f});
	}

	// リプレイマネージャにスナップショットを設定
	replayManager_.SetSnapshot(snapshot);

	// EXE側のアロケータ関数ポインタを取得
	ImGuiMemAllocFunc allocFunc = nullptr;
	ImGuiMemFreeFunc freeFunc = nullptr;
	void* userData = nullptr;
	ImGui::GetAllocatorFunctions(&allocFunc, &freeFunc, &userData);

	// DLLに渡す DebugState 構造体マッピングを設定
	debugState_.playerPos = playerPos_;
	debugState_.playerRot = playerRot_;
	debugState_.playerColor = playerColor_;

	debugState_.threadNodeCount = &threadNodeCount_;
	debugState_.threadNodes = threadNodes_;

	debugState_.enemyCount = &enemyCount_;
	debugState_.enemyPositions = enemyPositions_;
	debugState_.enemyRotations = enemyRotations_;
	debugState_.enemyHPs = enemyHPs_;

	debugState_.isPlayback = &isPlayback_;
	debugState_.playbackFrame = &playbackFrame_;
	debugState_.totalFrames = &totalFrames_;

	debugState_.isSocketSyncEnabled = &isSocketSyncEnabled_;
	debugState_.isSocketConnected = &isSocketConnected_;
	debugState_.socketPort = &socketPort_;

	debugState_.isBugTriggered = &isBugTriggered_;
	debugState_.bugMessage = bugMessage_;
	debugState_.triggerBugNow = &triggerBugNow_;
	debugState_.saveReplayNow = &saveReplayNow_;
	debugState_.loadReplayNow = &loadReplayNow_;

	debugState_.staticObjectCount = &staticObjectCount_;

	// 汎用オブジェクト同期システム初期化と登録
	debugSync_.BindToState(debugState_);
	debugSync_.Register(obj_.get(), "Player", "Player");
	for (size_t i = 0; i < enemies_.size(); ++i) {
		debugSync_.Register(enemies_[i].get(), "Enemy_" + std::to_string(i), "Enemy");
	}
	for (size_t i = 0; i < objects_.size(); ++i) {
		debugSync_.Register(objects_[i].get(), "Obj_" + std::to_string(i), "Object");
	}

	// [テスト] 新しい障害物の生成とデバッグシステムへの登録
	for (int i = 0; i < 2; ++i) {
		auto obs = std::make_unique<Object3d>();
		obs->Initialize("cube");
		obs->SetPosition({ (i == 0 ? -3.0f : 3.0f), 0.5f, 3.0f });
		obs->SetScale({ 1.0f, 1.0f, 1.0f });
		obs->SetCamera(camera_.get());
		obs->SetColor({ 0.5f, 0.5f, 0.5f, 1.0f }); // グレーの立方体
		
		// 登録処理
		debugSync_.Register(obs.get(), "Obstacle_" + std::to_string(i), "Obstacle");
		
		obstacles_.push_back(std::move(obs));
	}

	debugState_.allocFunc = (void*)allocFunc;
	debugState_.freeFunc = (void*)freeFunc;
	debugState_.allocUserData = userData;

	// Winsockサーバー起動 (ポート12345)
	socketServer_.Start(12345);

	// デバッグ用DLLを読み込む
	LoadDebugUI();
}

void GamePlayScene::Update() {
	// DLLの変更監視とホットリロード
	UpdateDLL();

	// ソケットからリプレイログ自動ロード命令を受信したかチェック
	std::string loadPath = "";
	if (isSocketSyncEnabled_ && socketServer_.GetLoadReplayPath(loadPath)) {
		LoadReplay(loadPath);
	}

	// ソケットからタイムライン同期命令を受信したかチェック
	int socketFrame = 0;
	if (isSocketSyncEnabled_ && socketServer_.GetTargetFrame(socketFrame)) {
		isPlayback_ = true;
		playbackFrame_ = socketFrame;
		if (playbackFrame_ >= replayManager_.GetTotalFrames()) {
			playbackFrame_ = replayManager_.GetTotalFrames() - 1;
		}
		if (playbackFrame_ < 0) {
			playbackFrame_ = 0;
		}
	}

	// ソケットからテイクオーバー命令を受信したかチェック
	if (socketServer_.CheckTakeover()) {
		// テイクオーバーする時点の状態を強制的に適用する
		RollbackToFrameState(playbackFrame_);
		isPlayback_ = false;
	}

	// DLL UIからのトリガーチェック
	if (triggerBugNow_) {
		isBugTriggered_ = true;
		strcpy_s(bugMessage_, "Manual Bug Triggered from ImGui!");
		triggerBugNow_ = false;
	}
	if (saveReplayNow_) {
		replayManager_.SaveLog("logs/play_log.json");
		saveReplayNow_ = false;
	}
	if (loadReplayNow_) {
		LoadReplay("logs/play_log.json");
		loadReplayNow_ = false;
	}

	// 状態の同期と記録
	if (isPlayback_) {
		// 巻き戻し/再生中: 記録されたフレームの座標を適用
		RollbackToFrameState(playbackFrame_);
	} else {
		// プレイヤーのキーボード操作 (WASDまたは矢印キー)
		Vector3 pPos = obj_->GetPosition();
		float speed = 0.1f;
		bool moved = false;
		if (Input::GetInstance()->PushKey(DIK_W) || Input::GetInstance()->PushKey(DIK_UP)) { pPos.z += speed; moved = true; }
		if (Input::GetInstance()->PushKey(DIK_S) || Input::GetInstance()->PushKey(DIK_DOWN)) { pPos.z -= speed; moved = true; }
		if (Input::GetInstance()->PushKey(DIK_A) || Input::GetInstance()->PushKey(DIK_LEFT)) { pPos.x -= speed; moved = true; }
		if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_RIGHT)) { pPos.x += speed; moved = true; }
		obj_->SetPosition(pPos);

		// 糸の生成 (一定距離動くごとにノードを追加)
		if (moved) {
			Vector3 lastNode = activeThreadNodes_.empty() ? Vector3(0.0f, -999.0f, 0.0f) : activeThreadNodes_.back();
			float dx = pPos.x - lastNode.x;
			float dy = pPos.y - lastNode.y;
			float dz = pPos.z - lastNode.z;
			float distSq = dx*dx + dy*dy + dz*dz;
			if (activeThreadNodes_.empty() || distSq > 0.5f * 0.5f) {
				activeThreadNodes_.push_back(pPos);
				if (activeThreadNodes_.size() > 100) {
					activeThreadNodes_.erase(activeThreadNodes_.begin()); // 最大100ノードに制限
				}
			}
		}

		// 敵の自律移動 (円状に巡回 - 初期位置基準)
		enemyTimer_ += 0.016f;
		for (size_t i = 0; i < enemies_.size(); ++i) {
			if (i < enemyInitialPositions_.size()) {
				Vector3 basePos = enemyInitialPositions_[i];
				Vector3 ePos;
				ePos.x = basePos.x + sinf(enemyTimer_ + i * 2.0f) * 2.5f;
				ePos.y = basePos.y;
				ePos.z = basePos.z + cosf(enemyTimer_ + i * 2.0f) * 2.5f;
				enemies_[i]->SetPosition(ePos);
			}

			Vector3 eRot = enemies_[i]->GetRotate();
			eRot.y = -(enemyTimer_ + i * 2.0f);
			enemies_[i]->SetRotation(eRot);
		}

		// バグトリガー衝突判定 (プレイヤーが敵に接近するとバグイベント発生)
		Vector3 playerPos = obj_->GetPosition();
		for (size_t i = 0; i < enemies_.size(); ++i) {
			Vector3 enemyPos = enemies_[i]->GetPosition();
			float dx = playerPos.x - enemyPos.x;
			float dy = playerPos.y - enemyPos.y;
			float dz = playerPos.z - enemyPos.z;
			float distSq = dx*dx + dy*dy + dz*dz;
			if (distSq < 1.0f * 1.0f && !isBugTriggered_) {
				isBugTriggered_ = true;
				strcpy_s(bugMessage_, "Bug: Player collided with Enemy!");
			}
		}

		// 毎フレームの状態を記録
		RecordFrameState();
	}

	// メンバ変数の値を DLL共有変数 (flat arrays) にキャプチャする
	CaptureStateToVars();

#ifdef USE_IMGUI
	// DLLからホットリロード対応UI関数を呼び出す
	if (drawDebugUI_) {
		drawDebugUI_(ImGui::GetCurrentContext(), &debugState_);
	}
#endif

	// 共有変数で変更された値をエンジン側に適用する
	ApplyStateFromVars();

	// カメラの行列計算
	if (camera_) {
		camera_->CalculateMatrix();
	}

	// オブジェクトの更新
	obj_->Update();

	for (auto& obj : objects_) {
		obj->Update();
	}

	for (auto& enemy : enemies_) {
		enemy->Update();
	}

	// 新しい障害物の更新
	for (auto& obs : obstacles_) {
		obs->Update();
	}

	// 糸（スレッド）表示用球体の更新
	while (threadSpherePool_.size() < activeThreadNodes_.size()) {
		auto sphere = std::make_unique<Object3d>();
		sphere->Initialize("sphere_thread");
		sphere->SetScale({0.15f, 0.15f, 0.15f});
		sphere->SetCamera(camera_.get());
		sphere->SetColor({1.0f, 0.5f, 0.0f, 1.0f}); // オレンジ色の糸
		threadSpherePool_.push_back(std::move(sphere));
	}
	for (size_t i = 0; i < activeThreadNodes_.size(); ++i) {
		threadSpherePool_[i]->SetPosition(activeThreadNodes_[i]);
		threadSpherePool_[i]->Update();
	}
}

void GamePlayScene::Draw() {
	// プレイヤー描画
	obj_->Draw();

	//// 静的背景オブジェクト描画
	//for (auto& obj : objects_) {
	//	obj->Draw();
	//}

	// 敵オブジェクト描画
	for (auto& enemy : enemies_) {
		enemy->Draw();
	}

	//// 新しい障害物の描画
	//for (auto& obs : obstacles_) {
	//	obs->Draw();
	//}

	//// 糸（スレッド）描画
	//for (size_t i = 0; i < activeThreadNodes_.size(); ++i) {
	//	threadSpherePool_[i]->Draw();
	//}
}

void GamePlayScene::Finalize() {
	// ソケットサーバー停止
	socketServer_.Stop();
	// DLLアンロード
	UnloadDebugUI();
}

void GamePlayScene::LoadDebugUI() {
	if (std::filesystem::exists("debugUiBuild/debugUiTemp.dll")) {
		std::filesystem::remove("debugUiBuild/debugUiTemp.dll");
	}

	if (std::filesystem::exists("debugUiBuild/debugUi.dll")) {
		try {
			std::filesystem::copy("debugUiBuild/debugUi.dll", "debugUiBuild/debugUiTemp.dll");
			debugDll_ = LoadLibraryA("debugUiBuild/debugUiTemp.dll");
			if (debugDll_) {
				drawDebugUI_ = (DrawDebugUIFunc)GetProcAddress(debugDll_, "DrawDebugUI");
				dllLastWriteTime_ = std::filesystem::last_write_time("debugUiBuild/debugUi.dll");
			}
		} catch (...) {
			// DLLがコンパイル中でロックされている場合は次回試行する
		}
	}
}

void GamePlayScene::UnloadDebugUI() {
	if (debugDll_) {
		FreeLibrary(debugDll_);
		debugDll_ = nullptr;
		drawDebugUI_ = nullptr;
	}
}

void GamePlayScene::UpdateDLL() {
	if (std::filesystem::exists("debugUiBuild/debugUi.dll")) {
		auto writeTime = std::filesystem::last_write_time("debugUiBuild/debugUi.dll");
		if (writeTime != dllLastWriteTime_) {
			UnloadDebugUI();
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // ビルド書き込み完了を少し待つ
			LoadDebugUI();
		}
	}
}

void GamePlayScene::CaptureStateToVars() {
	// Player
	Vector3 pPos = obj_->GetPosition();
	playerPos_[0] = pPos.x;
	playerPos_[1] = pPos.y;
	playerPos_[2] = pPos.z;

	Vector3 pRot = obj_->GetRotate();
	playerRot_[0] = pRot.x;
	playerRot_[1] = pRot.y;
	playerRot_[2] = pRot.z;

	Vector4 pCol = obj_->GetColor();
	playerColor_[0] = pCol.x;
	playerColor_[1] = pCol.y;
	playerColor_[2] = pCol.z;
	playerColor_[3] = pCol.w;

	// Thread nodes
	threadNodeCount_ = static_cast<int>(activeThreadNodes_.size());
	for (int i = 0; i < threadNodeCount_ && i < 100; ++i) {
		threadNodes_[i * 3 + 0] = activeThreadNodes_[i].x;
		threadNodes_[i * 3 + 1] = activeThreadNodes_[i].y;
		threadNodes_[i * 3 + 2] = activeThreadNodes_[i].z;
	}

	// Enemies
	enemyCount_ = static_cast<int>(enemies_.size());
	for (int i = 0; i < enemyCount_ && i < 5; ++i) {
		Vector3 ePos = enemies_[i]->GetPosition();
		enemyPositions_[i * 3 + 0] = ePos.x;
		enemyPositions_[i * 3 + 1] = ePos.y;
		enemyPositions_[i * 3 + 2] = ePos.z;

		Vector3 eRot = enemies_[i]->GetRotate();
		enemyRotations_[i * 3 + 0] = eRot.x;
		enemyRotations_[i * 3 + 1] = eRot.y;
		enemyRotations_[i * 3 + 2] = eRot.z;
	}

	isSocketConnected_ = socketServer_.IsConnected();
	socketPort_ = socketServer_.GetPort();
	totalFrames_ = replayManager_.GetTotalFrames();
	staticObjectCount_ = static_cast<int>(replayManager_.GetSnapshot().staticObjects.size());

	// 汎用オブジェクトのキャプチャ
	debugSync_.Capture();
}

void GamePlayScene::ApplyStateFromVars() {
	obj_->SetPosition({playerPos_[0], playerPos_[1], playerPos_[2]});
	obj_->SetRotation({playerRot_[0], playerRot_[1], playerRot_[2]});
	obj_->SetColor({playerColor_[0], playerColor_[1], playerColor_[2], playerColor_[3]});



	for (int i = 0; i < enemyCount_ && i < 5; ++i) {
		enemies_[i]->SetPosition({enemyPositions_[i * 3 + 0], enemyPositions_[i * 3 + 1], enemyPositions_[i * 3 + 2]});
		enemies_[i]->SetRotation({enemyRotations_[i * 3 + 0], enemyRotations_[i * 3 + 1], enemyRotations_[i * 3 + 2]});
	}

	// 汎用オブジェクトの適用
	debugSync_.Apply();
}

void GamePlayScene::RecordFrameState() {
	FrameState f;
	f.frameIndex = replayManager_.GetTotalFrames();
	f.playerTranslation = obj_->GetPosition();
	f.playerRotation = obj_->GetRotate();
	f.playerColor = obj_->GetColor();
	f.threadNodes = activeThreadNodes_;

	for (size_t i = 0; i < enemies_.size(); ++i) {
		EnemyFrameState e;
		e.index = static_cast<int>(i);
		e.translation = enemies_[i]->GetPosition();
		e.rotation = enemies_[i]->GetRotate();
		e.hp = enemyHPs_[i];
		e.animState = isBugTriggered_ ? "bugged" : "walk";
		f.enemies.push_back(e);
	}

	f.bugTrigger = isBugTriggered_;
	f.bugMsg = bugMessage_;

	// 汎用オブジェクトの記録
	f.objects = debugSync_.ExportFrameState();

	replayManager_.RecordFrame(f);
}

void GamePlayScene::RollbackToFrameState(int frame) {
	FrameState f;
	if (replayManager_.GetFrameState(frame, f)) {
		obj_->SetPosition(f.playerTranslation);
		obj_->SetRotation(f.playerRotation);
		obj_->SetColor(f.playerColor);

		activeThreadNodes_ = f.threadNodes;

		for (const auto& e : f.enemies) {
			if (e.index >= 0 && e.index < static_cast<int>(enemies_.size())) {
				enemies_[e.index]->SetPosition(e.translation);
				enemies_[e.index]->SetRotation(e.rotation);
				enemyHPs_[e.index] = e.hp;
			}
		}

		isBugTriggered_ = f.bugTrigger;
		strcpy_s(bugMessage_, f.bugMsg.c_str());
		
		// 汎用オブジェクトの復元
		debugSync_.ImportFrameState(f.objects);
		
		// 巻き戻し先のフレーム時間に合わせて敵の移動タイマーを同期する
		enemyTimer_ = frame * 0.016f;
	}
}

void GamePlayScene::LoadReplay(const std::string& path) {
	if (replayManager_.LoadLog(path)) {
		isPlayback_ = true;
		playbackFrame_ = 0;
	}
}


