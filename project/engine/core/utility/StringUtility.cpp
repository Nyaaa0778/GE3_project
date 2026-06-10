#include "StringUtility.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>  
#include <stringapiset.h>

namespace StringUtility {
	/// <summary>
	/// std::stringからstd::wstringに変換
	/// </summary>
	/// <param name="str">変換したい文字列</param>
	/// <returns>wstringに変換した文字列</returns>
	std::wstring ConvertString(const std::string& str) {
		if (str.empty()) {
			return std::wstring();
		}

		auto sizeNeeded =
			MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]),
				static_cast<int>(str.size()), NULL, 0);
		if (sizeNeeded == 0) {
			return std::wstring();
		}
		std::wstring result(sizeNeeded, 0);
		MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]),
			static_cast<int>(str.size()), &result[0], sizeNeeded);
		return result;
	}

	/// <summary>
	/// std::wstringからstd::stringに変換
	/// </summary>
	/// <param name="str">変換したい文字列</param>
	/// <returns>stringに変換した文字列</returns>
	std::string ConvertString(const std::wstring& str) {
		if (str.empty()) {
			return std::string();
		}

		auto sizeNeeded =
			WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
				NULL, 0, NULL, NULL);
		if (sizeNeeded == 0) {
			return std::string();
		}
		std::string result(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
			result.data(), sizeNeeded, NULL, NULL);
		return result;
	}
} // namespace StringUtility
