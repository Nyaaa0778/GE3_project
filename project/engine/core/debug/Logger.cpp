#include "Logger.h"

#include <Windows.h>
#include <debugapi.h>
#include <fstream>

namespace Logger {
	/// <summary>
	/// std::stringのメッセージを出力ウィンドウに表示
	/// </summary>
	/// <param name="message">メッセージの内容</param>
	void Log(const std::string& message) { OutputDebugStringA(message.c_str()); }
}; // namespace Logger