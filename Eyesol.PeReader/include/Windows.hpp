#if !defined _WINDOWS_H_
#	define _WINDOWS_H_
#	if defined _WIN32
#		include "framework.hpp"
#		include "win32_include.hpp"
#		include "Strings.hpp"
#		include "WindowsConsoleOutputFix.hpp"

namespace Eyesol::Windows
{
	struct WideStringTagType
	{
	};

	constexpr WideStringTagType WideStringTag;

	struct ByteStringTagType
	{
	};

	constexpr ByteStringTagType ByteStringTag;

	EYESOLPEREADER_API std::string GetLastErrorAsUtf8String(DWORD errorCode);
	EYESOLPEREADER_API std::wstring GetLastErrorAsString(DWORD errorCode, WideStringTagType);
	EYESOLPEREADER_API std::string FormatWindowsErrorMessage(DWORD code, std::string actionDescription);

	class WindowsHandle
	{
		HANDLE _handle;

	public:
		WindowsHandle() noexcept
			: _handle{ INVALID_HANDLE_VALUE }
		{
		}

		WindowsHandle(HANDLE handle) noexcept
			: _handle{ handle }
		{
		}

		WindowsHandle(const WindowsHandle&) = delete;
		WindowsHandle& operator=(const WindowsHandle&) = delete;

		WindowsHandle(WindowsHandle&& other) noexcept
			: _handle{ other._handle }
		{
			other._handle = INVALID_HANDLE_VALUE;
		}

		WindowsHandle& operator=(WindowsHandle&& other) noexcept
		{
			_handle = other._handle;
			other._handle = INVALID_HANDLE_VALUE;
			return *this;
		}

		~WindowsHandle()
		{
			closeHandle();
		}

		HANDLE getHandle() const { return _handle; }

		// closes an old handle and places a new one
		void put(HANDLE handle) noexcept
		{
			closeHandle();
			_handle = handle;
		}

		// returns a handle preventing it's releasing
		HANDLE release() noexcept
		{
			HANDLE handle = _handle;
			_handle = INVALID_HANDLE_VALUE;
			return handle;
		}

	private:
		void closeHandle() noexcept
		{
			if (_handle != INVALID_HANDLE_VALUE)
			{
				::CloseHandle(_handle);
				_handle = INVALID_HANDLE_VALUE;
			}
		}
	};
	
}
#	endif
#endif