#include <Windows.h>

#include "GameFramework.h"
#include "GameManager.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  CoInitializeEx(0, COINIT_MULTITHREADED);

  GameFramework *gameManager = new GameManager();

  gameManager->Execute();

  delete gameManager;

  return 0;
}