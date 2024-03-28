#include <iostream>
#include "Strings.hpp"
#include "WindowsConsoleOutputFix.hpp"
#include "win32.hpp"

namespace
{
	class ConsoleStreamBufWin32 : public std::streambuf
	{
	public:
		ConsoleStreamBufWin32(DWORD handleId, bool isInput);

	protected:
		// std::basic_streambuf
		virtual std::streambuf* setbuf(char_type* s, std::streamsize n);
		virtual int sync();
		virtual int_type underflow();
		virtual int_type overflow(int_type c = traits_type::eof());

	private:
		HANDLE const m_handle;
		bool const m_isInput;
		std::string m_buffer;
	};

	ConsoleStreamBufWin32::ConsoleStreamBufWin32(DWORD handleId, bool isInput) :
		m_handle(::GetStdHandle(handleId)),
		m_isInput(isInput),
		m_buffer()
	{
		if (m_isInput)
		{
			setg(0, 0, 0);
		}
	}

	std::streambuf* ConsoleStreamBufWin32::setbuf(char_type* /*s*/, std::streamsize /*n*/)
	{
		return 0;
	}

	int ConsoleStreamBufWin32::sync()
	{
		if (m_isInput)
		{
			::FlushConsoleInputBuffer(m_handle);
			setg(0, 0, 0);
		}
		else
		{
			if (m_buffer.empty())
			{
				return 0;
			}

			std::wstring const wideBuffer = Eyesol::utf8string_to_wstring(m_buffer);
			DWORD writtenSize;
			::WriteConsoleW(m_handle, wideBuffer.c_str(), wideBuffer.size(), &writtenSize, NULL);
		}

		m_buffer.clear();

		return 0;
	}

	ConsoleStreamBufWin32::int_type ConsoleStreamBufWin32::underflow()
	{
		if (!m_isInput)
		{
			return traits_type::eof();
		}

		if (gptr() >= egptr())
		{
			wchar_t wideBuffer[128];
			DWORD readSize;
			if (!::ReadConsoleW(m_handle, wideBuffer, ARRAYSIZE(wideBuffer) - 1, &readSize, NULL))
			{
				return traits_type::eof();
			}

			wideBuffer[readSize] = L'\0';
			m_buffer = Eyesol::wstring_to_utf8string(wideBuffer);

			setg(&m_buffer[0], &m_buffer[0], &m_buffer[0] + m_buffer.size());

			if (gptr() >= egptr())
			{
				return traits_type::eof();
			}
		}

		return sgetc();
	}

	ConsoleStreamBufWin32::int_type ConsoleStreamBufWin32::overflow(int_type c)
	{
		if (m_isInput)
		{
			return traits_type::eof();
		}

		m_buffer += traits_type::to_char_type(c);
		return traits_type::not_eof(c);
	}

	void FixStdStream(unsigned long handleId, std::istream& stream)
	{
		if (::GetFileType(::GetStdHandle(handleId)) == FILE_TYPE_CHAR)
		{
			stream.rdbuf(new ConsoleStreamBufWin32(handleId, true));
		}
	}

	void FixStdStream(unsigned long handleId, std::ostream& stream)
	{
		if (::GetFileType(::GetStdHandle(handleId)) == FILE_TYPE_CHAR)
		{
			stream.rdbuf(new ConsoleStreamBufWin32(handleId, false));
		}
	}
}

void Eyesol::Windows::FixStdStreams()
{
	FixStdStream(STD_INPUT_HANDLE, std::cin);
	FixStdStream(STD_OUTPUT_HANDLE, std::cout);
	FixStdStream(STD_ERROR_HANDLE, std::cerr);
}