#if !defined _PE_PARSER_H_
#	define _PE_PARSER_H_
#	include "MzParser.hpp"
#	include "PeHeaders.hpp"

namespace Eyesol::Executables::Pe
{
	class PeParseContext;

	class PeExecutable : public Mz::MzExecutable
	{
		virtual ExecutableObjectFormat format() const override;
		virtual ExecutableType type() const override;
		virtual Eyesol::Cpu::ArchType arch() const override;

		virtual uint64_t length() const override;

		virtual bool ContainsDebugInfo() const;
		virtual std::shared_ptr<DebugInfo> GetDebugInfo() const;

		void init(MemoryMappedIO::MemoryMappedFile file, PeParseContext& ctx);
	};

	class EYESOLPEREADER_API PeParser : public Mz::MzParser
	{
	protected:
		virtual bool TryParseTypeAndFormat(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, Mz::MzDosHeader& header, Mz::MzParseContext* ctx) const override;
		virtual std::shared_ptr<Mz::MzExecutable> ParseExecutable(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, Mz::MzParseContext& ctx) const override;
		virtual std::uint32_t CalculateActualMzDataLength(const MemoryMappedIO::MemoryMappedFile& file, uint32_t precalculatedLength, const Mz::MzParseContext& ctx) const override;

		virtual std::unique_ptr<Mz::MzParseContext> CreateParseContext() const override;
	};
}
#endif