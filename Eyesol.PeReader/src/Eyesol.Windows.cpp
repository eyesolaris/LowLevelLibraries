#if defined _WIN32
#	include "Eyesol.Windows.hpp"
namespace Eyesol::Windows
{
	//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
	std::string GetLastErrorAsUtf8String(DWORD errorCode)
	{
		std::wstring tempString = GetLastErrorAsString(errorCode, WideStringTag);

		return Eyesol::wstring_to_utf8string(tempString);
	}

	//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
	std::wstring GetLastErrorAsString(DWORD errorCode, WideStringTagType)
	{
		//Get the error message ID, if any.
		DWORD errorMessageID = errorCode;
		if (errorMessageID == 0) {
			return {}; //No error message has been recorded
		}

		LPWSTR messageBuffer = nullptr;

		//Ask Win32 to give us the string version of that message ID.
		//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
		size_t size = FormatMessageW(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errorMessageID,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&messageBuffer),
			0,
			NULL);

		//Copy the error message into a std::string.
		std::wstring message(messageBuffer, size);

		//Free the Win32's string's buffer.
		::LocalFree(messageBuffer);

		return message;
	}

	std::string FormatWindowsErrorMessage(DWORD code, std::string actionDescription)
	{
		return
			std::string("An error occured while ")
			+ actionDescription
			+ ", error code: "
			+ std::to_string(code)
			+ ", message: "
			+ GetLastErrorAsUtf8String(code);
	}
}
#endif