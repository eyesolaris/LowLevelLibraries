#include <cassert>
#include "MemoryMappedIO.hpp"
#include "Runtime.hpp"

namespace Eyesol::MemoryMappedIO
{
	#pragma region Shared Impl namespace implementation
	namespace Impl
	{
		const ::std::shared_ptr<MemoryMappedFileImpl>& get_impl(const MemoryMappedFile& file)
		{
			return file._impl;
		}

		const ::std::shared_ptr<MemoryMappedFileRegionImpl>& get_impl(const MemoryMappedFileRegion& region)
		{
			return region._impl;
		}

		const ::std::shared_ptr<MemoryMappedFileImpl>& get_impl(const MemoryMappedFileIterator& iter)
		{
			return iter._fileImpl;
		}

		MemoryMappedFile from_impl(const std::shared_ptr<MemoryMappedFileImpl>& fileImpl)
		{
			return MemoryMappedFile{ fileImpl };
		}

		std::size_t CalculateMapRegionParameters(
			std::uint64_t absoluteOffset, // Absolute offset in the file
			std::uint64_t fileLength, // Total file length in bytes
			std::uint64_t* baseOffsetPtr, // Calculated offset of the region in the file
			std::size_t* regionLengthPtr, // Calculated length of the allocated region
			std::size_t* offsetInRegionPtr // Starting offset in the region
		) // returns granularity
		{
			if (absoluteOffset >= fileLength)
			{
				throw std::out_of_range{ "File offset is out of range of file length" };
			}
			std::size_t granularity = { Eyesol::Runtime::AllocationGranularity() };
			// Allocate a new region
			std::uint64_t base{ absoluteOffset / granularity * granularity };
			std::size_t length = granularity;
			/*if (base + regionLength < base) // uint64 overflow. Should be impossible
			{
				throw std::overflow_error{ "too long file" };
			}*/
			if (base + length > fileLength)
			{
				length = granularity - (base + length - fileLength);
			}
			if (baseOffsetPtr != nullptr)
			{
				*baseOffsetPtr = base;
			}
			if (regionLengthPtr != nullptr)
			{
				*regionLengthPtr = length;
			}
			if (offsetInRegionPtr != nullptr)
			{
				*offsetInRegionPtr = absoluteOffset % granularity;
			}
			return granularity;
		}
	}
	#pragma endregion

	#pragma region MemoryMappedFile implementation
	MemoryMappedFile::MemoryMappedFile() noexcept
		: _length{ 0 }
	{
	}

	MemoryMappedFile::MemoryMappedFile(std::string path)
	{
		_impl = Impl::OpenFile(path, _length);
	}

	MemoryMappedFile::MemoryMappedFile(std::u16string path)
	{
		_impl = Impl::OpenFile(path, _length);
	}

	MemoryMappedFile::MemoryMappedFile(std::wstring path)
	{
		_impl = Impl::OpenFile(path, _length);
	}

	MemoryMappedFile::MemoryMappedFile(const MemoryMappedFile& other)
		: _impl{ other._impl },
		_length{ other._length }
	{
	}

	MemoryMappedFile::MemoryMappedFile(const MemoryMappedFileRegion& region)
	{
		_impl = Impl::get_impl(*Impl::get_impl(region));
		_length = Impl::GetFileLength(*_impl);
	}

	MemoryMappedFile::MemoryMappedFile(const std::shared_ptr<Impl::MemoryMappedFileImpl>& impl)
		: _impl{ impl },
		_length{ Impl::GetFileLength(*impl) }
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

	MemoryMappedFile& MemoryMappedFile::operator=(const MemoryMappedFile& other)
	{
		_impl = other._impl;
		_length = other._length;
		return *this;
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
		Impl::CalculateMapRegionParameters(absoluteOffset, _length, &baseOffset, &regionLength, &offsetInRegion);
		if (Impl::get_impl(_regionCache) == nullptr || !_regionCache.WithinRange(absoluteOffset))
		{
			_regionCache = this->MapRegion(baseOffset, regionLength);
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

	std::string MemoryMappedFile::path() const
	{
		return Impl::GetFilePath(*_impl);
	}

	MemoryMappedFileRegion MemoryMappedFile::MapRegion(std::uint64_t offset, std::size_t length) const
	{
		if (empty())
		{
			throw std::runtime_error{ "object is empty" };
		}
		return MemoryMappedFileRegion(_impl, offset, length);
	}

	std::size_t MemoryMappedFile::Read(unsigned char* buf, std::size_t bufLength, std::uint64_t fileOffset, std::size_t bufOffset, std::size_t readLength) const
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
			Impl::CalculateMapRegionParameters(fileOffset + bytesRead, _length, &baseOffset, &regionLength, &offsetInRegion);
			std::size_t currentRegionBytesToRead = std::min(regionLength - offsetInRegion, bytesToRead - bytesRead);
			MemoryMappedFileRegion currentRegion = MapRegion(baseOffset, regionLength);
			// Buffers must not overlap
			std::memcpy(buf + bytesRead, currentRegion.data() + offsetInRegion, currentRegionBytesToRead);
			bytesRead += currentRegionBytesToRead;
		} while (bytesRead != bytesToRead);
		return bytesRead;
	}
	#pragma endregion

	#pragma region MemoryMappedFileRegion implementation
	MemoryMappedFileRegion::MemoryMappedFileRegion(const std::shared_ptr<Impl::MemoryMappedFileImpl>& file, std::uint64_t offset, std::size_t length)
		: _impl{ Impl::MapRegion(file, offset, length) },
		_offset{ offset },
		_length{ length }
	{
	}

	unsigned char MemoryMappedFileRegion::operator[](std::uint64_t offsetInRegion) const
	{
		return *(Impl::RegionBegin(*_impl) + offsetInRegion);
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
		return Impl::RegionBegin(*_impl);
	}

	const unsigned char* MemoryMappedFileRegion::end() const
	{
		return Impl::RegionEnd(*_impl);
	}

	bool MemoryMappedFileRegion::WithinRange(std::uint64_t absoluteOffset) const noexcept
	{
		auto startingOffset = _offset;
		auto endingOffset = _offset + _length;
		return absoluteOffset >= startingOffset && absoluteOffset < endingOffset;
	}

	MemoryMappedFileRegion::~MemoryMappedFileRegion()
	{
	}
	#pragma endregion

	#pragma region MemoryMappedFileIterator implementation
	MemoryMappedFileIterator::MemoryMappedFileIterator(MemoryMappedFileIterator&& other) noexcept
		: _fileImpl{ std::move(other._fileImpl) },
		_baseInFile{ other._baseInFile },
		_fileLength{ other._fileLength },
		_mapRegionLength{ other._mapRegionLength },
		_lastRegion{ other._lastRegion }
	{
		other.invalidate();
	}

	MemoryMappedFileIterator::MemoryMappedFileIterator(const MemoryMappedFile& file, std::uint64_t absoluteOffset)
		: _fileImpl{ Impl::get_impl(file) },
		_baseInFile{},
		_fileLength{ file.length() },
		_mapRegionLength{},
		_lastRegion{ false }
	{
		if (!RecalculateData(absoluteOffset))
		{
			throw std::out_of_range{ "An address is not multiple of an allocation granularity" };
		}
	}

	MemoryMappedFileIterator::~MemoryMappedFileIterator()
	{
	}

	MemoryMappedFileIterator::MemoryMappedFileIterator(const MemoryMappedFileIterator& other)
		: _fileImpl{ other._fileImpl },
		_baseInFile{ other._baseInFile },
		_fileLength{ other._fileLength },
		_mapRegionLength{ other._mapRegionLength },
		_lastRegion{ other._lastRegion }
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
		return *this;
	}

	// Returns true if a resulting offset
	// is equal to start of the allocated mapped range
	bool MemoryMappedFileIterator::RecalculateData(std::uint64_t absoluteOffset)
	{
		std::uint64_t baseOffset;
		std::size_t offsetInRegion;
		std::size_t mapRegionLength;
		std::size_t granularity = Impl::CalculateMapRegionParameters(absoluteOffset, _fileLength, &baseOffset, &mapRegionLength, &offsetInRegion);
		
		//_currentRegion = file.mapRegion(absoluteOffset, mapRegionLength);
		_baseInFile = baseOffset;
		_mapRegionLength = mapRegionLength;
		_lastRegion = baseOffset + mapRegionLength >= _fileLength;
		return offsetInRegion == 0; 
	}

	bool MemoryMappedFileIterator::operator==(const MemoryMappedFileIterator& other) const noexcept
	{
		if (empty() && other.empty())
		{
			return true;
		}
		if (empty() || other.empty())
		{
			return false;
		}
		return _fileImpl == other._fileImpl
			&& this->_baseInFile == other._baseInFile
			&& this->_mapRegionLength == other._mapRegionLength;
	}

	std::partial_ordering MemoryMappedFileIterator::operator<=>(const MemoryMappedFileIterator& other) const
	{
		if (empty() && other.empty())
		{
			return std::partial_ordering::equivalent;
		}
		if (this->_fileImpl != other._fileImpl)
		{
			return std::partial_ordering::unordered;
		}
		auto thisAbsoluteOffset = _baseInFile;
		auto otherAbsoluteOffset = other._baseInFile;
		return thisAbsoluteOffset <=> otherAbsoluteOffset;
	}

	MemoryMappedFileRegion MemoryMappedFileIterator::operator*() const
	{
		if (empty())
		{
			throw std::runtime_error{ "Invalid iterator, or end of file is reached" };
		}
		MemoryMappedFile file = Impl::from_impl(_fileImpl);
		return file.MapRegion(_baseInFile, _mapRegionLength);
	}
	#pragma endregion
}