#include "Logger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include<Windows.h>
#include <debugapi.h>

namespace Logger {
	/// <summary>
	/// std::stringのメッセージを出力ウィンドウに表示
	/// </summary>
	/// <param name="message">メッセージの内容</param>
	void Log(const std::string& message) { OutputDebugStringA(message.c_str()); }
}; // namespace Logger