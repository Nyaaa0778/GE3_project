#pragma once
#include <string>

#include <vector>

namespace Logger {
	/// <summary>
	/// std::stringのメッセージを出力ウィンドウに表示
	/// </summary>
	/// <param name="message">メッセージの内容</param>
	void Log(const std::string& message);

	// ログ履歴を取得する
	const std::vector<std::string>& GetLogs();

	// ログ履歴をクリアする
	void ClearLogs();
}; // namespace Logger
