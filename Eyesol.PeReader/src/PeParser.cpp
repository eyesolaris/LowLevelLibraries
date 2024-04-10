#include "PeParser.hpp"
#include "Exceptions.hpp"

namespace Eyesol::Executables::Pe
{
	struct PeParseContext : Mz::MzParseContext
	{

	};

	std::unique_ptr<Mz::MzParseContext> PeParser::CreateParseContext() const
	{
		return std::make_unique<PeParseContext>();
	}

	bool PeParser::TryParseTypeAndFormat(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, Mz::MzDosHeader& header, Mz::MzParseContext* ctx) const
	{
		if (header.e_lfanew == 0)
		{
			return false;
		}
		std::uint32_t peSig;
		file.Read<std::endian::little>(peSig, header.e_lfanew);
		if (peSig == PE_SIGNATURE)
		{
			if (ctx != nullptr)
			{
				// Now implement easy, but incorrect format determination
				ctx->format = ExecutableObjectFormat::Pe;
				ctx->type = ExecutableType::Executable;
			}
			return true;
		}
		return false;
	}

	std::shared_ptr<Mz::MzExecutable> PeParser::ParseExecutable(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, Mz::MzParseContext& ctx) const
	{
		PeParseContext& peCtx = static_cast<PeParseContext&>(ctx);

		throw NotImplementedException{};
	}

	std::uint32_t PeParser::CalculateActualMzDataLength(const MemoryMappedIO::MemoryMappedFile& file, uint32_t precalculatedLength, const Mz::MzParseContext& ctx) const
	{
		return ctx.header.e_lfanew;
	}

	void PeExecutable::init(MemoryMappedIO::MemoryMappedFile file, PeParseContext& ctx)
	{
		MzExecutable::init(std::move(file), ctx);
	}
}