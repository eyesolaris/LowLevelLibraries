#include "Strings.hpp"
#include "Windows.hpp"

namespace Eyesol
{
	std::wstring utf8string_to_wstring(std::string str)
	{
		if (str.size() > std::numeric_limits<DWORD>::max())
		{
			throw std::runtime_error{ "Too long string" }; // We can work it around
		}
		// first, find out a necessary buffer size
		int newStringCharsCount =
			::MultiByteToWideChar(
				CP_UTF8, // A defult encoding (May be CP_ACP?)
				0, // flags
				str.c_str(), // byte string
				static_cast<DWORD>(str.size()), // byte string length plus NULL (in case of -1, string must be null-terminated)
				nullptr, // a pointer to an output buffer. Ignored when the next param is 0
				0 // size of an output buffer. Zero makes a function to return a required buffer size
			);
		if (newStringCharsCount == 0)
		{
			throw std::runtime_error{ Windows::FormatWindowsErrorMessage(::GetLastError(), "determining a string to wstring conversion buffer size") };
		}
		if (newStringCharsCount < 0)
		{
			throw std::runtime_error{ "New string buffer size is less then zero" };
		}
		std::wstring newString(static_cast<std::size_t>(static_cast<unsigned int>(newStringCharsCount)), '\0');
		int charsWritten =
			::MultiByteToWideChar(
				CP_UTF8,
				0,
				str.c_str(),
				static_cast<DWORD>(str.size()),
				newString.data(),
				static_cast<unsigned int>(newString.length())
			);
		if (charsWritten != newStringCharsCount)
		{
			throw std::runtime_error{ Windows::FormatWindowsErrorMessage(::GetLastError(), "encoding a string to wstring") };
		}
		return std::move(newString);
	}

	std::string wstring_to_utf8string(std::wstring str)
	{
		if (str.size() > std::numeric_limits<DWORD>::max())
		{
			throw std::runtime_error{ "Too long string" }; // We can work it around
		}
		BOOL wasDefaultCharacterUsed = FALSE; // were there some unconvertable characters
		// first, find out a necessary buffer size
		int newStringCharsCount =
			::WideCharToMultiByte(
				CP_UTF8, // A defult encoding (May be CP_ACP?)
				0, // flags
				str.c_str(), // wide string
				static_cast<DWORD>(str.size()), // wide string length (in case of -1, string must be null-terminated)
				nullptr, // a pointer to an output buffer. Ignored when the next param is 0
				0, // size of an output buffer. Zero makes a function to return a required buffer size
				nullptr, // default character pointer
				&wasDefaultCharacterUsed
			);
		if (newStringCharsCount == 0)
		{
			throw std::runtime_error{ Windows::FormatWindowsErrorMessage(::GetLastError(), "determining a wstring to string conversion buffer size") };
		}
		if (newStringCharsCount < 0)
		{
			throw std::runtime_error{ "New string buffer size is less then zero" };
		}
		std::string newString(static_cast<std::size_t>(static_cast<unsigned int>(newStringCharsCount)), '\0');
		int charsWritten =
			::WideCharToMultiByte(
				CP_UTF8,
				0,
				str.data(),
				static_cast<DWORD>(str.size()),
				newString.data(),
				static_cast<unsigned int>(newString.length()),
				nullptr,
				nullptr
			);
		if (charsWritten != newStringCharsCount)
		{
			throw std::runtime_error{ Windows::FormatWindowsErrorMessage(::GetLastError(), "encoding a string to wstring") };
		}
		return std::move(newString);
	}

	std::wstring u16string_to_wstring(std::u16string str)
	{
		// In Windows, wchar_t is equivalent to UTF16
		std::wstring convertedStr(str.length(), L'\0');
		for (std::size_t i = 0; i < str.length(); i++)
		{
			convertedStr[i] = static_cast<wchar_t>(str[i]);
		}
		return std::move(convertedStr);
	}
}