#if !defined _MZ_READER_H_
#	define _MZ_READER_H_
#	include <optional>
#	include "Executable.hpp"
#	include "PeHeaders.hpp"

namespace Eyesol::Executables::Mz
{
	class MzExecutable;

	struct MzParseContext
	{
		std::optional<ExecutableObjectFormat> format;
		std::optional<ExecutableType> type;
		MzFileMetadata metadata;
		MzDosHeader& header = metadata.header;
		bool mzHeaderRead{};
		virtual ~MzParseContext();
	};

	class EYESOLPEREADER_API MzParser : public ExecutableParser
	{
	public:
		virtual const std::vector<std::string>& SupportedFormatNames() const noexcept;

		virtual bool IsTypeSupported(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const final;
		virtual std::shared_ptr<Executable> TryParse(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, std::exception_ptr* excPtr) const final;

	protected:
		virtual bool TryParseTypeAndFormat(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, MzDosHeader& header, MzParseContext* ctx) const;

		virtual std::unique_ptr<MzParseContext> CreateParseContext() const;
		virtual std::shared_ptr<MzExecutable> ParseExecutable(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx) const;
		virtual std::uint32_t CalculateActualMzDataLength(const MemoryMappedIO::MemoryMappedFile& file, uint32_t precalculatedLength, const MzParseContext& ctx) const;

	private:
		void ReadDosMetadata(const MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx) const;
		bool TryParseTypeAndFormatPrivate(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, MzParseContext* ctx) const;

		static void ReadDosHeader(const MemoryMappedIO::MemoryMappedFile& file, MzDosHeader& header);

		static uint16_t CalculateChecksum(const MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx, bool includeBytesBeyondActualDosPart);
		static bool ChecksumValid(uint16_t checksum, uint16_t complementChecksum);

		// Writes an offset of the "Rich" signature (if any) to the offset parameter
		static void TryParseRichHeader(const MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx);

		static std::vector<std::string> _supportedFormatNames;
	};

	class MzExecutable : public Executable
	{
	public:
		MzExecutable() noexcept
			: _exeType{}
		{
		}

		const MzFileMetadata& metadata() const
		{
			return _metadata;
		}

		virtual ExecutableObjectFormat format() const;
		virtual ExecutableType type() const;
		virtual Eyesol::Cpu::ArchType arch() const;

		virtual uint64_t length() const;
		// Maybe empty if file is memory-only
		virtual std::string path() const final;

		virtual bool ContainsDebugInfo() const;
		virtual std::shared_ptr<DebugInfo> GetDebugInfo() const;

		void init(MemoryMappedIO::MemoryMappedFile file, const MzParseContext& ctx);

	private:
		MemoryMappedIO::MemoryMappedFile _file;
		MzFileMetadata _metadata;
		ExecutableType _exeType;
	};
}
#endif