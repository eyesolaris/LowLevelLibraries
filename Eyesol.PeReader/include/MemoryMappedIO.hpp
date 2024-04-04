#if !defined _MEMORYMAPPEDIO_H_
#	define _MEMORYMAPPEDIO_H_
#	include <framework.hpp>

namespace Eyesol::MemoryMappedIO
{
	class MemoryMappedFile;
	class MemoryMappedFileRegion;
	class MemoryMappedFileIterator;

	namespace Impl
	{
		class MemoryMappedFileImpl;
		class MemoryMappedFileRegionImpl;

		const ::std::shared_ptr<MemoryMappedFileImpl>& get_impl(const MemoryMappedFile&);
		const ::std::shared_ptr<MemoryMappedFileRegionImpl>& get_impl(const MemoryMappedFileRegion&);
		const ::std::shared_ptr<MemoryMappedFileImpl>& get_impl(const MemoryMappedFileIterator&);

		const ::std::shared_ptr<MemoryMappedFileImpl>& get_impl(const MemoryMappedFileRegionImpl&);

		void GetRegionOffsetAndLength(const MemoryMappedFileRegionImpl& region, std::uint64_t& offset, std::uint64_t& length);

		MemoryMappedFile from_impl(const std::shared_ptr<MemoryMappedFileImpl>&);

		std::size_t CalculateMapRegionParameters(
			std::uint64_t absoluteOffset, // Absolute offset in the file
			std::uint64_t fileLength, // Total file length in bytes
			std::uint64_t* baseOffsetPtr, // Calculated offset of the region in the file
			std::size_t* regionLengthPtr, // Calculated length of the allocated region
			std::size_t* offsetInRegionPtr // Starting offset in the region
		); // returns granularity

		::std::shared_ptr<MemoryMappedFileImpl> OpenFile(::std::string str, std::uint64_t& fileLength);
		::std::shared_ptr<MemoryMappedFileImpl> OpenFile(::std::wstring str, std::uint64_t& fileLength);
		::std::shared_ptr<MemoryMappedFileImpl> OpenFile(::std::u16string str, std::uint64_t& fileLength);

		// May throw exceptions std::out_of_range and std::runtime_error
		::std::shared_ptr<MemoryMappedFileRegionImpl> MapRegion(const std::shared_ptr<Impl::MemoryMappedFileImpl>& fileHandle, std::uint64_t offset, std::uint64_t length);

		const unsigned char* RegionBegin(const Impl::MemoryMappedFileRegionImpl& region);
		const unsigned char* RegionEnd(const MemoryMappedFileRegionImpl& region);

		std::uint64_t GetFileLength(const MemoryMappedFileImpl&);
	}

	//class MemoryMappedFileImpl;

	class EYESOLPEREADER_API MemoryMappedFileRegion
	{
	public:
		MemoryMappedFileRegion()
			: _offset{},
			_length{}
		{
		}

		MemoryMappedFileRegion(const MemoryMappedFileRegion&) noexcept = default;
		MemoryMappedFileRegion(MemoryMappedFileRegion&&) noexcept = default;

		// An intrinsic constructor. Do not use
		MemoryMappedFileRegion(const std::shared_ptr<Impl::MemoryMappedFileImpl>& file, std::uint64_t offset, std::size_t length);

		~MemoryMappedFileRegion();

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
		bool WithinRange(std::uint64_t absoluteOffset) const noexcept;

	private:
		friend class Impl::MemoryMappedFileImpl;

		friend const ::std::shared_ptr<Impl::MemoryMappedFileRegionImpl>& Impl::get_impl(const MemoryMappedFileRegion&);

		std::shared_ptr<Impl::MemoryMappedFileRegionImpl> _impl;
		std::uint64_t _offset;
		std::size_t _length;
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
		std::size_t Read(unsigned char* buf, std::size_t bufLength, std::uint64_t fileOffset, std::size_t bufOffset, std::size_t readLength) const;

		unsigned char operator[](std::uint64_t absoluteOffset) const;

		MemoryMappedFileRegion MapRegion(std::uint64_t offset, std::size_t length) const;

	private:
		MemoryMappedFile(const MemoryMappedFileRegion&);
		MemoryMappedFile(const std::shared_ptr<Impl::MemoryMappedFileImpl>& impl);

		// An order of fields is important, as it is expected
		// that _impl initializes first, and then - _size
		std::shared_ptr<Impl::MemoryMappedFileImpl> _impl;
		std::uint64_t _length;

		mutable MemoryMappedFileRegion _regionCache;

		friend const ::std::shared_ptr<Impl::MemoryMappedFileImpl>& Impl::get_impl(const MemoryMappedFile&);
		friend MemoryMappedFile Impl::from_impl(const std::shared_ptr<MemoryMappedFileImpl>&);
		friend class Impl::MemoryMappedFileImpl;
	};

	class EYESOLPEREADER_API MemoryMappedFileIterator
	{
	public:
		MemoryMappedFileIterator(const MemoryMappedFileIterator& other);
		MemoryMappedFileIterator(MemoryMappedFileIterator&& other) noexcept;
		~MemoryMappedFileIterator();

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
		std::shared_ptr<Impl::MemoryMappedFileImpl> _fileImpl;
		std::uint64_t _baseInFile;
		std::uint64_t _fileLength;
		std::size_t _mapRegionLength;
		bool _lastRegion;

		bool RecalculateData(std::uint64_t absoluteOffset);

		void invalidate()
		{
			_fileImpl = nullptr;
			_baseInFile = 0;
			_mapRegionLength = 0;
		}

		friend const ::std::shared_ptr<Impl::MemoryMappedFileImpl>& Impl::get_impl(const MemoryMappedFileIterator&);
	};
}
#endif // _MEMORYMAPPEDIO_H_