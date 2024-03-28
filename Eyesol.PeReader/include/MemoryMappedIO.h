#if !defined _MEMORIMAPPEDIO_H_
#	define _MEMORIMAPPEDIO_H_
#	include <framework.hpp>
namespace Eyesol::MemoryMappedIO
{
	class MemoryMappedFileRegion;

	class EYESOLPEREADER_API MemoryMappedFile
	{
		MemoryMappedFile(std::string path);
		MemoryMappedFile(std::wstring path);
		MemoryMappedFile(std::u16string path);

		~MemoryMappedFile();

		std::uint64_t length() const { return _length; }

		MemoryMappedFileRegion mapRegion(std::uint64_t offset, std::size_t length);

	private:
		class MemoryMappedFileImpl;

		MemoryMappedFile(const MemoryMappedFileRegion&);

		// An order of fields is important, as it is expected
		// that _impl initializes first, and then - _size
		std::shared_ptr<MemoryMappedFileImpl> _impl;
		std::uint64_t _length;

		friend class MemoryMappedFileRegion;
	};

	class EYESOLPEREADER_API MemoryMappedFileRegion
	{
		class MemoryMappedFileRegionImpl;

		std::shared_ptr<MemoryMappedFileRegionImpl> _impl;
		std::uint64_t _offset;
		std::size_t _length;

	public:
		const unsigned char* begin() const;
		const unsigned char* end() const;
		// Total region length
		std::size_t length() const { return _length; }
		// Offset in the file
		std::uint64_t offset() const { return _offset; }

	private:
		friend class MemoryMappedFile;

		MemoryMappedFileRegion(const MemoryMappedFile::MemoryMappedFileImpl& file, std::uint64_t offset, std::size_t length);
	};
}
#endif