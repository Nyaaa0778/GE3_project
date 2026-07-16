#include "Logger.h"

#include <Windows.h>
#include <debugapi.h>
#include <fstream>

namespace Logger {
	static std::vector<std::string> sLogs;

	/// <summary>
	/// std::stringのメッセージを出力ウィンドウに表示
	/// </summary>
	/// <param name="message">メッセージの内容</param>
	void Log(const std::string& message) { 
		OutputDebugStringA(message.c_str()); 
		sLogs.push_back(message);
	}

	const std::vector<std::string>& GetLogs() {
		return sLogs;
	}

	void ClearLogs() {
		sLogs.clear();
	}
}; // namespace Logger