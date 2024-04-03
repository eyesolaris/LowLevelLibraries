#include "MemoryMappedIO.hpp"
#include "Runtime.hpp"

namespace Eyesol::MemoryMappedIO
{
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

	MemoryMappedFileIterator::MemoryMappedFileIterator(MemoryMappedFileIterator&& other) noexcept
		: _iteratorImpl{ std::move(other._iteratorImpl) },
		_baseInFile{ other._baseInFile },
		_fileLength{ other._fileLength },
		_mapRegionLength{ other._mapRegionLength },
		_lastRegion{ other._lastRegion }
	{
		other.invalidate();
	}

	MemoryMappedFileIterator::MemoryMappedFileIterator(const MemoryMappedFile& file, std::uint64_t absoluteOffset)
		: _iteratorImpl{ file._impl },
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

	// Returns true if a resulting offset
	// is equal to start of the allocated mapped range
	bool MemoryMappedFileIterator::RecalculateData(std::uint64_t absoluteOffset)
	{
		std::uint64_t baseOffset;
		std::size_t offsetInRegion;
		std::size_t mapRegionLength;
		std::size_t granularity = CalculateMapRegionParameters(absoluteOffset, _fileLength, &baseOffset, &mapRegionLength, &offsetInRegion);
		
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
		return _iteratorImpl == other._iteratorImpl
			&& this->_baseInFile == other._baseInFile
			&& this->_mapRegionLength == other._mapRegionLength;
	}
}