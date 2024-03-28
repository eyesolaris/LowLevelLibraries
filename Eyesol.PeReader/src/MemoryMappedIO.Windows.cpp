#if defined _WIN32
#	include "MemoryMappedIO.hpp"
#	include "Runtime.hpp"
#	include "Eyesol.Windows.hpp"

using namespace Eyesol::Windows;

namespace Eyesol::MemoryMappedIO
{
	namespace
	{
		void* createMapViewOfFile(HANDLE fileMapping, std::uint64_t offset, std::size_t length, std::uint64_t totalFileLength)
		{
			auto allocGranularity = Eyesol::Runtime::AllocationGranularity();
			if (offset % allocGranularity != 0)
			{
				throw std::out_of_range{ "Invalid offset granularity. " + std::to_string(allocGranularity) + " is required" };
			}
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
				throw std::runtime_error{ FormatWindowsErrorMessage(::GetLastError(), "creating a file map view") };
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
			: MemoryMappedFileImpl{ utf8string_to_wstring(path) }
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
				path.copy(correctedPath.data() + sizeof(LONG_FILE_HEADER) / sizeof(wchar_t), path.length());
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
					throw std::runtime_error{ FormatWindowsErrorMessage(::GetLastError(), "opening a file") };
				}
				openedFile = openedFileHandle;
			}

			std::uint64_t fileSize;
			{
				LARGE_INTEGER largeInt;
				if (!::GetFileSizeEx(openedFile.getHandle(), &largeInt))
				{
					throw std::runtime_error{ FormatWindowsErrorMessage(::GetLastError(), "determining a file size") };
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
					throw std::runtime_error{ FormatWindowsErrorMessage(::GetLastError(), "creating a file mapping") };
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

		// TODO: create a custom iterator

		~MemoryMappedFileImpl()
		{
			BOOL ok = ::CloseHandle(_fileMappingObjectHandle);
			ok = ::CloseHandle(_fileHandle);
			_fileMappingObjectHandle = INVALID_HANDLE_VALUE;
			_fileHandle = INVALID_HANDLE_VALUE;
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
		std::uint64_t _offset;
		// A length of the view
		std::uint64_t _length;

	public:
		MemoryMappedFileRegionImpl(const std::shared_ptr<MemoryMappedFile::MemoryMappedFileImpl>& file, std::uint64_t offset, std::uint64_t length)
			: _impl{ file },
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

	MemoryMappedFile::MemoryMappedFile() noexcept
		: _impl{},
		_length{ 0 }
	{
	}

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

	MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
		: _impl{ std::move(other._impl) },
		_length{ other._length }
	{
		other._length = 0;
	}


	MemoryMappedFile::~MemoryMappedFile()
	{
	}

	MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept
	{
		_impl = std::move(other._impl);
		_length = other._length;
		other._length = 0;
		return *this;
	}

	MemoryMappedFileRegion MemoryMappedFile::mapRegion(std::uint64_t offset, std::size_t length) const
	{
		if (empty())
		{
			throw std::runtime_error{ "object is empty" };
		}
		return MemoryMappedFileRegion(_impl, offset, length);
	}

	MemoryMappedFileRegion::MemoryMappedFileRegion(const std::shared_ptr<MemoryMappedFile::MemoryMappedFileImpl>& file, std::uint64_t offset, std::size_t length)
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