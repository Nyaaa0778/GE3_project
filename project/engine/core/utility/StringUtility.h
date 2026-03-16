#pragma once
#include <string>

namespace StringUtility {
	/// <summary>
	/// std::stringからstd::wstringに変換
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	std::wstring ConvertString(const std::string& str);

	/// <summary>
	/// std::wstringからstd::stringに変換
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	std::string ConvertString(const std::wstring& str);
} // namespace StringUtility
