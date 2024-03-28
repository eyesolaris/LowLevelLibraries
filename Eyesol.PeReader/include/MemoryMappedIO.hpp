#if !defined _MEMORIMAPPEDIO_H_
#	define _MEMORIMAPPEDIO_H_
#	include <framework.hpp>
namespace Eyesol::MemoryMappedIO
{
	class MemoryMappedFileRegion;
	class MemoryMappedFileIterator;

	class EYESOLPEREADER_API MemoryMappedFile
	{
	public:
		MemoryMappedFile() noexcept;
		MemoryMappedFile(std::string path);
		MemoryMappedFile(std::wstring path);
		MemoryMappedFile(std::u16string path);
		MemoryMappedFile(MemoryMappedFile&&) noexcept;

		MemoryMappedFile& operator=(MemoryMappedFile&&) noexcept;

		~MemoryMappedFile();

		std::uint64_t length() const { return _length; }
		bool empty() const { return _length == 0; }

		unsigned char operator[](std::uint64_t offset) const;

		MemoryMappedFileRegion mapRegion(std::uint64_t offset, std::size_t length) const;

	private:
		class EYESOLPEREADER_API MemoryMappedFileImpl;

		MemoryMappedFile(const MemoryMappedFileRegion&);

		// An order of fields is important, as it is expected
		// that _impl initializes first, and then - _size
		std::shared_ptr<MemoryMappedFileImpl> _impl;
		std::uint64_t _length;

		friend class MemoryMappedFileRegion;
		friend class MemoryMappedFileIterator;
	};

	class EYESOLPEREADER_API MemoryMappedFileIterator
	{
		class MemoryMappedFileIteratorImpl;

	public:
		MemoryMappedFileIterator(const MemoryMappedFileIterator& other);
		MemoryMappedFileIterator(MemoryMappedFileIterator&& other) noexcept;

		unsigned char operator*() const;
		// preincrement
		MemoryMappedFileIterator& operator++();
		// postincrement
		MemoryMappedFileIterator operator++(int);

		bool operator==(const MemoryMappedFileIterator& other) noexcept;
		int operator<=>(const MemoryMappedFileIterator& other) noexcept;

	private:
		MemoryMappedFileIterator() { }

		std::shared_ptr<MemoryMappedFileIteratorImpl> _iteratorImpl;

		friend class MemoryMappedFile;
	};

	class EYESOLPEREADER_API MemoryMappedFileRegion
	{
		class MemoryMappedFileRegionImpl;

	public:
		const unsigned char* begin() const;
		const unsigned char* end() const;
		// Total region length
		std::size_t length() const { return _length; }
		// Offset in the file
		std::uint64_t offset() const { return _offset; }

	private:
		friend class MemoryMappedFile;

		std::shared_ptr<MemoryMappedFileRegionImpl> _impl;
		std::uint64_t _offset;
		std::size_t _length;

		MemoryMappedFileRegion(const std::shared_ptr<MemoryMappedFile::MemoryMappedFileImpl>& file, std::uint64_t offset, std::size_t length);
	};
}
#endif