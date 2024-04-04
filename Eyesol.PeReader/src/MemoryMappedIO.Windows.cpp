// Windows-specific MemoryMappedIO implementation
#if defined _WIN32
#	include "MemoryMappedIO.hpp"
#	include "Runtime.hpp"
#	include "Windows.hpp"

using namespace Eyesol::Windows;

namespace Eyesol::MemoryMappedIO
{
	#pragma region nameless namespace (Windows-specific functionality)
	namespace
	{
		// Windows limits
		constexpr std::uint16_t MAX_LONG_PATH_LENGTH = 32767;

		void* CreateMapViewOfFile(HANDLE fileMapping, std::uint64_t offset, std::size_t length, std::uint64_t totalFileLength)
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
				throw std::out_of_range{ std::string("File offset is too long: " + std::to_string(offset) + ". Maximum ")
					+ std::to_string(totalFileLength - 1) + " is allowed" };
			}
			std::uint64_t maximumLength = totalFileLength - offset;
			if (maximumLength > std::numeric_limits<std::size_t>::max())
			{
				maximumLength = std::numeric_limits<std::size_t>::max();
			}
			if (length > maximumLength)
			{
				throw std::out_of_range{ "Map view length requested is too long: " + std::to_string(length) + ". Maximum "
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
	#pragma endregion

	#pragma region Windows-specific Impl::MemoryMappedFileImpl
	class Impl::MemoryMappedFileImpl
	{
	public:
		HANDLE _fileHandle;
		HANDLE _fileMappingObjectHandle;
		std::uint64_t _fileLength;

		// TODO: create a custom iterator
		MemoryMappedFileImpl(HANDLE fileHandle, HANDLE fileMappingObjectHandle, std::uint64_t fileLength) noexcept
			: _fileHandle{ fileHandle },
			_fileMappingObjectHandle{ fileMappingObjectHandle },
			_fileLength{ fileLength }
		{
		}

		~MemoryMappedFileImpl()
		{
			BOOL ok = ::CloseHandle(_fileMappingObjectHandle);
			ok = ::CloseHandle(_fileHandle);
			_fileMappingObjectHandle = INVALID_HANDLE_VALUE;
			_fileHandle = INVALID_HANDLE_VALUE;
		}
	};
	#pragma endregion

	#pragma region Windows-specific Impl::MemoryMappedFileRegionImpl
	class Impl::MemoryMappedFileRegionImpl
	{
	public:
		// Holds file handles opened
		std::shared_ptr<Impl::MemoryMappedFileImpl> _impl;
		unsigned char* _mapViewBaseAddress;
		// An offset of the view in the file
		std::uint64_t _offset;
		// A length of the view
		std::uint64_t _length;

		MemoryMappedFileRegionImpl(const std::shared_ptr<Impl::MemoryMappedFileImpl>& fileHandle, void* regionStart, std::uint64_t offset, std::uint64_t length)
			: _impl{ fileHandle },
			_mapViewBaseAddress{ reinterpret_cast<unsigned char*>(regionStart) },
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
	};
	#pragma endregion

	#pragma region Windows-specific Impl namespace implementation
	namespace Impl
	{
		std::shared_ptr<MemoryMappedFileImpl> OpenFile(::std::string path, std::uint64_t& fileLength)
		{
			return OpenFile(utf8string_to_wstring(path), fileLength);
		}

		std::shared_ptr<MemoryMappedFileImpl> OpenFile(::std::u16string path, std::uint64_t& fileLength)
		{
			return OpenFile(u16string_to_wstring(path), fileLength);
		}

		std::shared_ptr<MemoryMappedFileImpl> OpenFile(::std::wstring path, std::uint64_t& fileLength)
		{
			if (path.length() > MAX_LONG_PATH_LENGTH)
			{
				throw std::out_of_range{ "Too long file name: longer than " + std::to_string(MAX_LONG_PATH_LENGTH) };
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
				if (largeInt.QuadPart < 0)
				{
					throw std::runtime_error{ "File size is less than zero" };
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
			fileLength = fileSize;
			// Construct a handle to return (don't release the handles yet,
			// in case of make_unique throwing an exception)
			auto ptr = std::make_unique<MemoryMappedFileImpl>(
				openedFile.getHandle(),
				fileMappingObject.getHandle(),
				fileSize);
			// Prevent handles from being untimely closed by the temporary handle holders
			openedFile.release();
			fileMappingObject.release();
			return std::move(ptr);
		}

		std::uint64_t GetFileLength(const MemoryMappedFileImpl& file)
		{
			return file._fileLength;
		}

		const std::shared_ptr<MemoryMappedFileImpl>& get_impl(const MemoryMappedFileRegionImpl& regionHandle)
		{
			return regionHandle._impl;
		}

		void GetRegionOffsetAndLength(const std::shared_ptr<MemoryMappedFileRegionImpl>& regionHandle, std::uint64_t& offset, std::uint64_t& length)
		{
			offset = regionHandle->_offset;
			length = regionHandle->_length;
		}

		std::shared_ptr<MemoryMappedFileRegionImpl> MapRegion(const std::shared_ptr<Impl::MemoryMappedFileImpl>& fileHandle, std::uint64_t offset, std::uint64_t length)
		{
			void* regionStart = reinterpret_cast<unsigned char*>(CreateMapViewOfFile(
				fileHandle->_fileMappingObjectHandle,
				offset,
				length,
				fileHandle->_fileLength));
			return std::make_shared<MemoryMappedFileRegionImpl>(fileHandle, regionStart, offset, length);
		}

		const unsigned char* RegionBegin(const Impl::MemoryMappedFileRegionImpl& region)
		{
			return region.begin();
		}

		const unsigned char* RegionEnd(const MemoryMappedFileRegionImpl& region)
		{
			return region.end();
		}
	}
	#pragma endregion
}
#endif