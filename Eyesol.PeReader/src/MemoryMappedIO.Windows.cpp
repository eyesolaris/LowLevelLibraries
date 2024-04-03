#if defined _WIN32
#	include <cassert>
#	include "MemoryMappedIO.hpp"
#	include "Runtime.hpp"
#	include "Windows.hpp"

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

	class MemoryMappedFileRegion;

	class MemoryMappedFileImpl
	{
	public:
		HANDLE _fileHandle;
		HANDLE _fileMappingObjectHandle;
		std::uint64_t _fileSize;

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
	public:
		// It holds file handles opened
		std::shared_ptr<MemoryMappedFileImpl> _impl;
		unsigned char* _mapViewBaseAddress;
		// An offset of the view in the file
		std::uint64_t _offset;
		// A length of the view
		std::uint64_t _length;

		MemoryMappedFileRegionImpl(const std::shared_ptr<MemoryMappedFileImpl>& file, std::uint64_t offset, std::uint64_t length)
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
		: _length{ 0 }
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
		_length{ region._impl->_impl->_fileSize }
	{
	}

	MemoryMappedFile::MemoryMappedFile(const std::shared_ptr<MemoryMappedFileImpl>& impl)
		: _impl{ impl },
		_length{ impl->_fileSize }
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

	unsigned char MemoryMappedFile::operator[](std::uint64_t absoluteOffset) const
	{
		if (absoluteOffset >= _length)
		{
			throw std::out_of_range{ "absolute offset is too long: "
				+ std::to_string(absoluteOffset) + ", maximum " + std::to_string(_length) + " is allowed" };
		}
		// If cache is empty
		std::size_t offsetInRegion;
		std::uint64_t baseOffset;
		std::size_t regionLength;
		CalculateMapRegionParameters(absoluteOffset, _length, &baseOffset, &regionLength, &offsetInRegion);
		if (_regionCache._impl == nullptr || !_regionCache.withinRange(absoluteOffset))
		{
			_regionCache = this->mapRegion(baseOffset, regionLength);
		}
		return _regionCache[offsetInRegion];
	}

	MemoryMappedFileIterator MemoryMappedFile::begin() const
	{
		return MemoryMappedFileIterator{ *this, 0 };
	}

	MemoryMappedFileIterator MemoryMappedFile::end() const
	{
		return MemoryMappedFileIterator{};
	}

	MemoryMappedFileRegion MemoryMappedFile::mapRegion(std::uint64_t offset, std::size_t length) const
	{
		if (empty())
		{
			throw std::runtime_error{ "object is empty" };
		}
		return MemoryMappedFileRegion(_impl, offset, length);
	}

	std::size_t MemoryMappedFile::read(unsigned char* buf, std::size_t bufLength, std::uint64_t fileOffset, std::size_t bufOffset, std::size_t readLength) const
	{
		if (fileOffset >= _length)
		{
			throw std::out_of_range{ "File offset is out of range" };
		}
		std::size_t remainingBufLength = bufLength - bufOffset;
		std::size_t bytesToRead = readLength;
		if (bytesToRead > remainingBufLength)
		{
			bytesToRead = remainingBufLength;
		}
		std::uint64_t remainingFileLength = _length - fileOffset;
		if (bytesToRead > remainingFileLength)
		{
			bytesToRead = { static_cast<std::size_t>(remainingFileLength) };
		}
		// Reading data region by region
		std::size_t bytesRead = 0;
		do
		{
			std::uint64_t baseOffset;
			std::size_t regionLength;
			std::size_t offsetInRegion;
			CalculateMapRegionParameters(fileOffset + bytesRead, _length, &baseOffset, &regionLength, &offsetInRegion);
			std::size_t currentRegionBytesToRead = std::min(regionLength - offsetInRegion, bytesToRead - bytesRead);
			MemoryMappedFileRegion currentRegion = mapRegion(baseOffset, regionLength);
			// Buffers must not overlap
			std::memcpy(buf + bytesRead, currentRegion.data() + offsetInRegion, currentRegionBytesToRead);
			bytesRead += currentRegionBytesToRead;
		} while (bytesRead != bytesToRead);
		return bytesRead;
	}

	MemoryMappedFileRegion::MemoryMappedFileRegion(const std::shared_ptr<MemoryMappedFileImpl>& file, std::uint64_t offset, std::size_t length)
		: _impl{ std::make_shared<MemoryMappedFileRegionImpl>(file, offset, length) },
		_offset{ _impl->offset() },
		_length{ _impl->length() }
	{
	}

	unsigned char MemoryMappedFileRegion::operator[](std::uint64_t offsetInRegion) const
	{
		return *(_impl->begin() + offsetInRegion);
	}

	unsigned char MemoryMappedFileRegion::at(std::uint64_t offsetInRegion) const
	{
		if (offsetInRegion >= _length)
		{
			throw std::out_of_range{ "Invalid offset in the region" };
		}
		return (*this)[offsetInRegion];
	}

	const unsigned char* MemoryMappedFileRegion::begin() const
	{
		return _impl->begin();
	}

	const unsigned char* MemoryMappedFileRegion::end() const
	{
		return _impl->end();
	}

	bool MemoryMappedFileRegion::withinRange(std::uint64_t absoluteOffset) const noexcept
	{
		auto startingOffset = _offset;
		auto endingOffset = _offset + _length;
		return absoluteOffset >= startingOffset && absoluteOffset < endingOffset;
	}

	MemoryMappedFileIterator::MemoryMappedFileIterator(const MemoryMappedFileIterator& other)
		: _iteratorImpl{ std::make_shared<MemoryMappedFileImpl>(*other._iteratorImpl) }
	{
	}

	MemoryMappedFileIterator& MemoryMappedFileIterator::operator++()
	{
		// first, check if the file is ended
		if (_lastRegion)
		{
			// No more regions
			// invalidate the iterator
			invalidate();
			return *this;
		}
		std::uint64_t nextAbsoluteOffset = _baseInFile + _mapRegionLength;
		bool ok = RecalculateData(nextAbsoluteOffset);
		assert(ok);
		/*MemoryMappedFile f{_iteratorImpl->_currentRegion};
		std::uint64_t fileSize = { f.length() };
		if (nextAbsoluteOffset >= fileSize)
		{
			// Invalidate an iterator
			_iteratorImpl = nullptr;
		}
		else
		{
			// Allocate a new mapped region
			// Allocate a new region
			std::uint64_t base, regionLength;
			CalculateMapRegionParameters(nextAbsoluteOffset, fileSize, &base, &regionLength, nullptr);
			_iteratorImpl->_currentRegion = std::move(f.mapRegion(base, regionLength));
			_iteratorImpl->_baseInFile = base;
			_iteratorImpl->_offsetInRegion = 0;
		}*/
		return *this;
	}

	std::partial_ordering MemoryMappedFileIterator::operator<=>(const MemoryMappedFileIterator& other) const
	{
		if (empty() && other.empty())
		{
			return std::partial_ordering::equivalent;
		}
		if (this->_iteratorImpl != other._iteratorImpl)
		{
			return std::partial_ordering::unordered;
		}
		auto thisAbsoluteOffset = _baseInFile;
		auto otherAbsoluteOffset = other._baseInFile;
		return thisAbsoluteOffset <=> otherAbsoluteOffset;
		/*if (thisAbsoluteOffset == otherAbsoluteOffset)
		{
			return std::partial_ordering::equivalent;
		}
		else if (thisAbsoluteOffset > otherAbsoluteOffset)
		{
			// If offset of left is greater, return a positive value
			return std::partial_ordering::less;
		}
		else
		{
			// Else return a negative value
			return std::partial_ordering::greater;
		}*/
	}

	MemoryMappedFileRegion MemoryMappedFileIterator::operator*() const
	{
		if (empty())
		{
			throw std::runtime_error{ "Invalid iterator, or end of file is reached" };
		}
		MemoryMappedFile file{ _iteratorImpl };
		return file.mapRegion(_baseInFile, _mapRegionLength);
	}
}
#endif