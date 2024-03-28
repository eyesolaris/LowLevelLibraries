#if defined _WIN32
#	include "MemoryMappedIO.h"
#	include "win32.hpp"
namespace Eyesol::MemoryMappedIO
{
	namespace
	{
		std::wstring string_to_wstring(std::string str)
		{
			// first, find out a necessary buffer size
			int newStringCharsCount =
				::MultiByteToWideChar(
					CP_UTF8, // A defult encoding (May be CP_ACP?)
					0, // flags
					str.c_str(), // byte string
					-1, // byte string length (in case of -1, string must be null-terminated)
					nullptr, // a pointer to an output buffer. Ignored when the next param is 0
					0 // size of an output buffer. Zero makes a function to return a required buffer size
				);
			if (newStringCharsCount == 0)
			{
				throw std::runtime_error{ std::string("Some error while encoding a string to wstring occured. Error code: "
					+ std::to_string(::GetLastError())) };
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
					-1,
					newString.data(),
					static_cast<unsigned int>(newString.length())
				);
			if (charsWritten != newStringCharsCount)
			{
				throw std::runtime_error(std::string("Some error while encoding a string to wstring occured. Error code: ")
					+ std::to_string(::GetLastError()));
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

		class WindowsHandle
		{
			HANDLE _handle;

		public:
			WindowsHandle()
				: _handle{ INVALID_HANDLE_VALUE }
			{
			}

			WindowsHandle(HANDLE handle)
				: _handle{ handle }
			{
			}

			WindowsHandle(const WindowsHandle&) = delete;
			WindowsHandle& operator=(const WindowsHandle&) = delete;

			WindowsHandle(WindowsHandle&& other)
				: _handle{ other._handle }
			{
				other._handle = INVALID_HANDLE_VALUE;
			}

			WindowsHandle& operator=(WindowsHandle&& other)
			{
				_handle = other._handle;
				other._handle = INVALID_HANDLE_VALUE;
			}

			~WindowsHandle()
			{
				closeHandle();
			}

			HANDLE getHandle() const { return _handle; }

			// closes an old handle and places a new one
			void put(HANDLE handle)
			{
				closeHandle();
				_handle = handle;
			}

			// returns a handle preventing it's releasing
			HANDLE release()
			{
				HANDLE handle = _handle;
				_handle = INVALID_HANDLE_VALUE;
				return handle;
			}

		private:
			void closeHandle()
			{
				if (_handle != INVALID_HANDLE_VALUE)
				{
					::CloseHandle(_handle);
					_handle = INVALID_HANDLE_VALUE;
				}
			}
		};

		void* createMapViewOfFile(HANDLE fileMapping, std::uint64_t offset, std::size_t length, std::uint64_t totalFileLength)
		{
			// Maximum offset is totalFileLength - 1,
			// which produces maximum a one byte region
			if (offset >= totalFileLength)
			{
				throw std::out_of_range{ std::string("File offset is too long. Maximum ")
					+ std::to_string(totalFileLength - 1) + " is allowed" };
			}
			std::uint64_t maximumLength = totalFileLength - offset;
			if (maximumLength > std::numeric_limits<std::size_t>::max())
			{
				maximumLength = std::numeric_limits<std::size_t>::max();
			}
			if (length > maximumLength)
			{
				throw std::out_of_range{ "Map view length requested is too long. Maximum "
					+ std::to_string(maximumLength) + " is allowed with requested offset " + std::to_string(offset) };
			}
			ULARGE_INTEGER unsignedOffset{ .QuadPart = static_cast<std::uint64_t>(offset) };
			void* baseAddress =
				::MapViewOfFile(
					fileMapping,
					FILE_MAP_READ,
					unsignedOffset.HighPart,
					unsignedOffset.LowPart,
					length
				);
			if (baseAddress == nullptr)
			{
				throw std::runtime_error{ std::string("Some error occured while creating a file map view. Error code: ")
					+ std::to_string(::GetLastError()) };
			}
			return baseAddress;
		}


	}

	class MemoryMappedFileRegion;
	
	class MemoryMappedFileRegion::MemoryMappedFileRegionImpl;

	class MemoryMappedFile::MemoryMappedFileImpl
	{
		HANDLE _fileHandle;
		HANDLE _fileMappingObjectHandle;
		std::uint64_t _fileSize;

	public:
		MemoryMappedFileImpl(std::string path)
			: MemoryMappedFileImpl{ string_to_wstring(path) }
		{
		}

		MemoryMappedFileImpl(std::wstring path)
		{
			if (path.length() > 32767)
			{
				throw std::out_of_range{ "Too long file name: longer than 32767" };
			}
			if (path.length() > MAX_PATH)
			{
				constexpr wchar_t LONG_FILE_HEADER[]{ L'\\', L'\\', L'?', L'\\' };
				std::wstring correctedPath(path.length() + sizeof(LONG_FILE_HEADER), '\0');
				std::copy(std::begin(LONG_FILE_HEADER), std::end(LONG_FILE_HEADER), correctedPath.begin());
				path.copy(correctedPath.data() + sizeof(LONG_FILE_HEADER), path.length());
				path = std::move(correctedPath);
			}
			WindowsHandle openedFile;
			{
				// First, open a file
				HANDLE openedFileHandle = ::CreateFileW(
					path.data(), // file name
					GENERIC_READ, // desired access
					0, // share mode
					nullptr, // security attributes
					OPEN_EXISTING, // creattion disposition
					FILE_ATTRIBUTE_NORMAL, // flags and attributes
					nullptr // template file
				);
				if (openedFileHandle == INVALID_HANDLE_VALUE)
				{
					throw new std::runtime_error{
						std::string("Some error happend while opening a file. Error code: ")
						+ std::to_string(::GetLastError()) };
				}
				openedFile = openedFileHandle;
			}

			std::uint64_t fileSize;
			{
				LARGE_INTEGER largeInt;
				if (!::GetFileSizeEx(openedFile.getHandle(), &largeInt))
				{
					throw new std::runtime_error{ std::string("Some error happened while determining a file size. Error code: ")
						+ std::to_string(::GetLastError()) };
				}
				fileSize = static_cast<ULONGLONG>(largeInt.QuadPart);
			}
			WindowsHandle fileMappingObject;
			{
				// Then, create a file mapping in memory
				HANDLE fileMappingObjectHandle = ::CreateFileMappingW(
					openedFile.getHandle(), // file
					nullptr, // file mapping security attributes
					PAGE_READONLY, // page protection
					0, // maximum ize high
					0, // maximum size low
					nullptr // object name
				);
				if (fileMappingObjectHandle == INVALID_HANDLE_VALUE)
				{
					throw std::runtime_error{ std::string("Can't create file mapping. Error code: ")
						+ std::to_string(::GetLastError()) };
				}
				fileMappingObject = fileMappingObjectHandle;
			}
			_fileHandle = openedFile.release();
			_fileMappingObjectHandle = fileMappingObject.release();
			_fileSize = fileSize;
		}

		MemoryMappedFileImpl(std::u16string path)
			: MemoryMappedFileImpl{ u16string_to_wstring(path) }
		{
		}

		std::uint64_t fileSize() const { return _fileSize; }

		MemoryMappedFileRegion mapRegion(std::uint64_t offset, std::size_t length)
		{
			return MemoryMappedFileRegion(*this, offset, length);
		}

		// TODO: create a custom iterator

		~MemoryMappedFileImpl()
		{
			CloseHandle(_fileMappingObjectHandle);
			CloseHandle(_fileHandle);
		}

	private:
		friend class MemoryMappedFileRegion::MemoryMappedFileRegionImpl;
	};

	class MemoryMappedFileRegion::MemoryMappedFileRegionImpl
	{
		// It holds file handles opened
		std::shared_ptr<MemoryMappedFile::MemoryMappedFileImpl> _impl;
		unsigned char* _mapViewBaseAddress;
		// An offset of the view in the file
		std::int64_t _offset;
		// A length of the view
		std::uint64_t _length;

	public:
		MemoryMappedFileRegionImpl(const MemoryMappedFile& file, std::uint64_t offset, std::uint64_t length)
			: _impl{ file._impl },
			_mapViewBaseAddress{ reinterpret_cast<unsigned char*>(createMapViewOfFile(
				_impl->_fileMappingObjectHandle,
				offset,
				length,
				_impl->_fileSize)) },
			_offset{ offset },
			_length{ length }
		{
		}

		~MemoryMappedFileRegionImpl()
		{
			::UnmapViewOfFile(_mapViewBaseAddress);
		}

		const unsigned char* begin() const { return _mapViewBaseAddress; }
		const unsigned char* end() const { return _mapViewBaseAddress + _length; }
		std::uint64_t offset() const { return _offset; }
		std::size_t length() const { return _length; }

	private:
		friend class MemoryMappedFile;
	};

	MemoryMappedFile::MemoryMappedFile(std::string path)
		: _impl{ std::move(std::make_shared<MemoryMappedFileImpl>(path)) },
		_length{ _impl->fileSize() }
	{
	}

	MemoryMappedFile::MemoryMappedFile(std::wstring path)
		: _impl{ std::move(std::make_shared<MemoryMappedFileImpl>(path)) },
		_length{ _impl->fileSize() }
	{

	}

	MemoryMappedFile::MemoryMappedFile(std::u16string path)
		: _impl{ std::move(std::make_shared<MemoryMappedFileImpl>(path)) },
		_length{ _impl->fileSize() }
	{
	}

	MemoryMappedFile::MemoryMappedFile(const MemoryMappedFileRegion& region)
		: _impl{ region._impl->_impl },
		_length{ region._length }
	{
	}

	MemoryMappedFile::~MemoryMappedFile()
	{
	}

	MemoryMappedFileRegion MemoryMappedFile::mapRegion(std::uint64_t offset, std::size_t length)
	{
		return _impl->mapRegion(offset, length);
	}

	MemoryMappedFileRegion::MemoryMappedFileRegion(const MemoryMappedFile::MemoryMappedFileImpl& file, std::uint64_t offset, std::size_t length)
		: _impl{ std::make_shared<MemoryMappedFileRegionImpl>(file, offset, length) },
		_offset{ _impl->offset() },
		_length{ _impl->length() }
	{
	}

	const unsigned char* MemoryMappedFileRegion::begin() const
	{
		return _impl->begin();
	}

	const unsigned char* MemoryMappedFileRegion::end() const
	{
		return _impl->end();
	}
}
#endif