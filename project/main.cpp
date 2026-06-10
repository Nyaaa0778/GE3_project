#include <Windows.h>
#include <objbase.h>

#include "GameFramework.h"
#include "GameManager.h"

#include <memory>

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	CoInitializeEx(0, COINIT_MULTITHREADED);

	std::unique_ptr<GameFramework> gameManager = std::make_unique<GameManager>();

	gameManager->Execute();

	return 0;
}