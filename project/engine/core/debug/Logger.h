#pragma once
#include <string>

namespace Logger {
	/// <summary>
	/// std::stringのメッセージを出力ウィンドウに表示
	/// </summary>
	/// <param name="message">メッセージの内容</param>
	void Log(const std::string& message);
}; // namespace Logger
