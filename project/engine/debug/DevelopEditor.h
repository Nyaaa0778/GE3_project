#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

struct WorldTransform;
class Camera;

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
};
