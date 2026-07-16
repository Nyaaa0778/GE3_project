#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <Vector2.h>

struct WorldTransform;
class Camera;
struct ImDrawList;

class DevelopEditor {
public:
	struct EditorEntity {
		std::string name;
		WorldTransform* transform = nullptr;
		Camera* camera = nullptr;
		std::function<void()> onInspect = nullptr;
	};

public:
	static DevelopEditor* GetInstance();
	static void Finalize();

	void Initialize();
	void Update();

	void RegisterEntity(const std::string& name, WorldTransform* transform, std::function<void()> onInspect = nullptr);
	void RegisterCamera(const std::string& name, Camera* camera, std::function<void()> onInspect = nullptr);
	void ClearEntities();

	void RegisterGameViewOverlay(const std::function<void(ImDrawList* drawList, const Vector2& imageScreenPos, const Vector2& imageSize)>& callback);

	void SetOnPlaceObjectCallback(const std::function<void(const std::string& assetName)>& callback);
	void SetOnSaveCallback(const std::function<void()>& callback);
	void SetOnPlaceSpriteCallback(const std::function<void(const std::string& assetName)>& callback);

	bool IsEditorMode() const { return isEditorMode_; }
	bool IsPaused() const { return isPaused_; }
	void SetPaused(bool paused) { isPaused_ = paused; }

private:
	friend std::default_delete<DevelopEditor>;

	DevelopEditor() = default;
	~DevelopEditor() = default;
	DevelopEditor(const DevelopEditor&) = delete;
	DevelopEditor& operator=(const DevelopEditor&) = delete;

private:
	static std::unique_ptr<DevelopEditor> instance_;

	bool isEditorMode_ = true;
	bool isPaused_ = false;

	std::vector<EditorEntity> entities_;
	std::vector<std::function<void(ImDrawList* drawList, const Vector2& imageScreenPos, const Vector2& imageSize)>> gameViewOverlays_;
	std::function<void(const std::string& assetName)> onPlaceObjectCallback_ = nullptr;
	std::function<void()> onSaveCallback_ = nullptr;
	std::function<void(const std::string& assetName)> onPlaceSpriteCallback_ = nullptr;
};
