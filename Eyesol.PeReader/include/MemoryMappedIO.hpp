#if !defined _MEMORYMAPPEDIO_H_
#	define _MEMORYMAPPEDIO_H_
#	include <framework.hpp>

namespace Eyesol::MemoryMappedIO
{
	class MemoryMappedFile;
	class MemoryMappedFileImpl;
	class MemoryMappedFileIterator;

	class EYESOLPEREADER_API MemoryMappedFileRegion
	{
		class MemoryMappedFileRegionImpl;

	public:
		MemoryMappedFileRegion(const MemoryMappedFileRegion&) noexcept = default;
		MemoryMappedFileRegion(MemoryMappedFileRegion&&) noexcept = default;

		MemoryMappedFileRegion& operator=(const MemoryMappedFileRegion& other) = default;
		MemoryMappedFileRegion& operator=(MemoryMappedFileRegion&& other) noexcept = default;

		unsigned char operator[](std::uint64_t offsetInRegion) const;
		unsigned char at(std::uint64_t offsetInRegion) const;

		const unsigned char* begin() const;
		const unsigned char* end() const;

		const unsigned char* data() const { return begin(); }
		// Total region length
		std::size_t length() const { return _length; }
		// Offset in the file
		std::uint64_t offset() const { return _offset; }
		bool withinRange(std::uint64_t absoluteOffset) const noexcept;

	private:
		MemoryMappedFileRegion()
			: _offset{},
			_length{}
		{
		}

		friend class MemoryMappedFile;
		friend class MemoryMappedFileImpl;
		friend class MemoryMappedFileIterator;

		std::shared_ptr<MemoryMappedFileRegionImpl> _impl;
		std::uint64_t _offset;
		std::size_t _length;

		MemoryMappedFileRegion(const std::shared_ptr<MemoryMappedFileImpl>& file, std::uint64_t offset, std::size_t length);
	};

	class EYESOLPEREADER_API MemoryMappedFile
	{
	public:
		MemoryMappedFile() noexcept;
		MemoryMappedFile(std::string path);
		MemoryMappedFile(std::wstring path);
		MemoryMappedFile(std::u16string path);
		MemoryMappedFile(MemoryMappedFile&&) noexcept;

		MemoryMappedFile& operator=(MemoryMappedFile&&) noexcept;

		bool operator==(const MemoryMappedFile&) const noexcept = default;

		~MemoryMappedFile();

		std::uint64_t length() const
		{
			return _length;
		}

		bool empty() const { return _length == 0; }

		MemoryMappedFileIterator begin() const;
		MemoryMappedFileIterator end() const;

		// buffers must not overlap
		std::size_t read(unsigned char* buf, std::size_t bufLength, std::uint64_t fileOffset, std::size_t bufOffset, std::size_t readLength) const;

		unsigned char operator[](std::uint64_t absoluteOffset) const;

		MemoryMappedFileRegion mapRegion(std::uint64_t offset, std::size_t length) const;

	private:
		MemoryMappedFile(const MemoryMappedFileRegion&);
		MemoryMappedFile(const std::shared_ptr<MemoryMappedFileImpl>& impl);

		// An order of fields is important, as it is expected
		// that _impl initializes first, and then - _size
		std::shared_ptr<MemoryMappedFileImpl> _impl;
		std::uint64_t _length;

		mutable MemoryMappedFileRegion _regionCache;

		friend class MemoryMappedFileRegion;
		friend class MemoryMappedFileIterator;
	};

	class EYESOLPEREADER_API MemoryMappedFileIterator
	{
	public:
		MemoryMappedFileIterator(const MemoryMappedFileIterator& other);
		MemoryMappedFileIterator(MemoryMappedFileIterator&& other) noexcept;

		MemoryMappedFileRegion operator*() const;
		// preincrement
		MemoryMappedFileIterator& operator++();

		std::partial_ordering operator<=>(const MemoryMappedFileIterator& other) const;
		bool operator==(const MemoryMappedFileIterator& other) const noexcept;

		MemoryMappedFileIterator()
			: _baseInFile{},
			_fileLength{},
			_mapRegionLength{},
			_lastRegion{ false }
		{
		}

		MemoryMappedFileIterator(const MemoryMappedFile& file, std::uint64_t offset = 0U);

		std::uint64_t getBaseAddress() const { return _baseInFile; }
		bool empty() const { return _mapRegionLength == 0; }

	private:
		std::shared_ptr<MemoryMappedFileImpl> _iteratorImpl;
		std::uint64_t _baseInFile;
		std::uint64_t _fileLength;
		std::size_t _mapRegionLength;
		bool _lastRegion;

		friend class MemoryMappedFile;

		bool RecalculateData(std::uint64_t absoluteOffset);

		void invalidate()
		{
			_iteratorImpl = nullptr;
			_baseInFile = 0;
			_mapRegionLength = 0;
		}
	};

	std::size_t CalculateMapRegionParameters(
		std::uint64_t absoluteOffset, // Absolute offset in the file
		std::uint64_t fileLength, // Total file length in bytes
		std::uint64_t* baseOffsetPtr, // Calculated offset of the region in the file
		std::size_t* regionLengthPtr, // Calculated length of the allocated region
		std::size_t* offsetInRegionPtr // Starting offset in the region
	); // returns granularity
}
#endif // _MEMORYMAPPEDIO_H_