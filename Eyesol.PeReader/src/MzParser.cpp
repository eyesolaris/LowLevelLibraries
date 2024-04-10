#include "MzParser.hpp"
#include "Exceptions.hpp"

namespace Eyesol::Executables::Mz
{
	MzParseContext::~MzParseContext() { }

	//////// MZ Parser
	bool MzParser::TryParseTypeAndFormatPrivate(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, MzParseContext* ctx) const
	{
		MzDosHeader headerLoc;
		MzDosHeader& header = ctx ? ctx->header : headerLoc;
		ReadDosHeader(file, header);
		if (header.e_magic != DOS_HEADER_MAGIC)
		{
			return false;
		}
		if (ctx != nullptr)
		{
			ctx->mzHeaderRead = true;
		}
		if (TryParseTypeAndFormat(file, header, ctx))
		{
			return true;
		}
		return false;
	}

	bool MzParser::IsTypeSupported(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const
	{
		return TryParseTypeAndFormatPrivate(file, nullptr);
	}

	std::unique_ptr<MzParseContext> MzParser::CreateParseContext() const
	{
		return std::make_unique<MzParseContext>();
	}

	std::shared_ptr<Executable> MzParser::TryParse(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, std::exception_ptr* excPtr) const
	{
		std::unique_ptr<MzParseContext> ctx = CreateParseContext();
		if (!TryParseTypeAndFormatPrivate(file, ctx.get()))
		{
			return nullptr;
		}
		try
		{
			ReadDosMetadata(file, *ctx);
			return ParseExecutable(file, *ctx);
		}
		catch (...)
		{
			if (excPtr != nullptr)
			{
				*excPtr = std::current_exception();
			}
			return nullptr;
		}
	}


	bool MzParser::TryParseTypeAndFormat(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, MzDosHeader& header, MzParseContext* ctx) const
	{
		if (ctx != nullptr)
		{
			ctx->format = ExecutableObjectFormat::Mz;
			std::string filePath = file.path();
			// Is this correct?
			if (filePath.ends_with(".dll"))
			{
				ctx->type = ExecutableType::DynamicLib;
			}
			else // if (filePath.ends_with(".exe") || filePath.ends_with(".com"))
			{
				ctx->type = ExecutableType::Executable;
			}
		}
		return true;
	}

	std::shared_ptr<MzExecutable> MzParser::ParseExecutable(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx) const
	{
		std::shared_ptr<MzExecutable> exe = std::make_shared<MzExecutable>();
		exe->init(file, ctx);
		return exe;
	}

	const std::vector<std::string>& MzParser::SupportedFormatNames() const noexcept
	{
		return _supportedFormatNames;
	}

	std::uint32_t MzParser::CalculateActualMzDataLength(const MemoryMappedIO::MemoryMappedFile& file, uint32_t precalculatedLength, const MzParseContext& ctx) const
	{
		return precalculatedLength;
	}

	void MzParser::ReadDosHeader(const MemoryMappedIO::MemoryMappedFile& file, MzDosHeader& header)
	{
		if constexpr (DOS_ENDIANNESS == std::endian::native)
		{
			file.Read<DOS_ENDIANNESS>(header, 0);
		}
		else
		{
			// Assume big endian
			file.Read<DOS_ENDIANNESS>(header.e_magic, 0);
			file.Read<DOS_ENDIANNESS>(header.e_cblp, 2);
			file.Read<DOS_ENDIANNESS>(header.e_cp, 4);
			file.Read<DOS_ENDIANNESS>(header.e_crlc, 6);
			file.Read<DOS_ENDIANNESS>(header.e_cparhdr, 8);
			file.Read<DOS_ENDIANNESS>(header.e_minalloc, 0xa);
			file.Read<DOS_ENDIANNESS>(header.e_maxalloc, 0xc);
			file.Read<DOS_ENDIANNESS>(header.e_ss, 0xe);
			file.Read<DOS_ENDIANNESS>(header.e_sp, 0x10);
			file.Read<DOS_ENDIANNESS>(header.e_csum, 0x12);
			file.Read<DOS_ENDIANNESS>(header.e_ip, 0x14);
			file.Read<DOS_ENDIANNESS>(header.e_cs, 0x16);
			file.Read<DOS_ENDIANNESS>(header.e_lfarlc, 0x18);
			file.Read<DOS_ENDIANNESS>(header.e_ovno, 0x1a);
			file.Read<DOS_ENDIANNESS>(header.e_res, 0x1c);
			file.Read<DOS_ENDIANNESS>(header.e_oemid, 0x24);
			file.Read<DOS_ENDIANNESS>(header.e_oeminfo, 0x26);
			file.Read<DOS_ENDIANNESS>(header.e_res2, 0x28);
			file.Read<DOS_ENDIANNESS>(header.e_lfanew, 0x3c);
		}
	}

	void MzParser::ReadDosMetadata(const MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx) const
	{
		// https://bytepointer.com/download.php?name=msdos_encyclopedia_article4_program_structure_exe_com.pdf
		MzDosHeader& mzHeader = ctx.header;
		MzFileMetadata& metadata = ctx.metadata;
		if (!ctx.mzHeaderRead)
		{
			ReadDosHeader(file, mzHeader);
			ctx.mzHeaderRead = true;
		}
		if (mzHeader.e_magic != DOS_HEADER_MAGIC)
		{
			throw std::runtime_error{ "file is not a DOS executable" };
		}
		//isDerivedType = IsDerivedType(file, mzHeader, nullptr, nullptr);

		/*std::uint32_t peSig;
		file.Read<uint32_t, std::endian::little>(peSig, mzHeader.e_lfanew);
		isPeFile = peSig == PE_SIGNATURE;*/

		size_t headerSize = mzHeader.e_cparhdr * DOS_HEADER_PARAGRAPH_SIZE;
		metadata.dosHeaderLoc = { 0, headerSize };
		// e_cp must be not less than 1
		uint16_t lengthOfTheLastPage = mzHeader.e_cblp;
		uint16_t fullPagesCount = mzHeader.e_cp;
		if (lengthOfTheLastPage > 0)
		{
			--fullPagesCount;
		}
		uint32_t dosPartFileLength = fullPagesCount * DOS_PAGE_SIZE + mzHeader.e_cblp;
		metadata.nominalDosDataLength = dosPartFileLength;
		auto fileLength = file.length();
		metadata.actualDosFileLengh = fileLength;
		if (fileLength < dosPartFileLength)
		{
			dosPartFileLength = static_cast<uint32_t>(fileLength);
		}
		dosPartFileLength = CalculateActualMzDataLength(file, dosPartFileLength, ctx);
		metadata.actualDosDataLength = dosPartFileLength;

		// Write and move out a generic algorithm to read a plain MZ DOS and PE files
		/*
		constexpr auto DOS_HEADER_VENDOR_SPECIFIC_DATA_START = DOS_HEADER_FORMATTED_AREA_END;
		auto dosRelocationsOffset = _dosHeader.e_lfarlc;
		auto dosHeaderVendorSpecificDataLength = dosRelocationsOffset - DOS_HEADER_VENDOR_SPECIFIC_DATA_START;
		_dosHeaderVendorSpecificDataLoc = { DOS_HEADER_VENDOR_SPECIFIC_DATA_START, dosHeaderVendorSpecificDataLength };
		_dosHeaderVendorSpecificData.resize(dosHeaderVendorSpecificDataLength, 0);
		if (_memFile.Read(_dosHeaderVendorSpecificData.data(), dosHeaderVendorSpecificDataLength, DOS_HEADER_VENDOR_SPECIFIC_DATA_START, 0, dosHeaderVendorSpecificDataLength) < dosHeaderVendorSpecificDataLength)
		{
			throw std::runtime_error{ "File is not DOS EXE file" };
		}
		*/

		uint16_t dosRelocationsOffset = mzHeader.e_lfarlc;
		uint16_t relocationsCount = mzHeader.e_crlc;
		metadata.dosRelocationsLoc = { dosRelocationsOffset, relocationsCount * DOS_HEADER_RELOCATION_SIZE };
		auto&& dosRelocations = metadata.dosRelocations;
		for (std::size_t i = 0; i < relocationsCount; i++)
		{
			MzDosHeaderRelocation reloc;
			file.Read<std::endian::little>(reloc.offset, dosRelocationsOffset + i * DOS_HEADER_RELOCATION_SIZE);
			file.Read<std::endian::little>(reloc.segment, dosRelocationsOffset + i * DOS_HEADER_RELOCATION_SIZE + sizeof(uint16_t));
			dosRelocations.push_back(reloc);
		}
		size_t dosStubCodeStart = dosRelocationsOffset + relocationsCount * DOS_HEADER_RELOCATION_SIZE;
		//uint32_t peHeaderOffset = mzHeader.e_lfanew;
		size_t dosStubCodeLength = dosPartFileLength - dosStubCodeStart;
		metadata.dosStubCodeLoc = { dosStubCodeStart, dosStubCodeLength };
		auto&& dosStubCode = metadata.dosStubCode;
		dosStubCode.resize(dosStubCodeLength, 0);
		if (file.Read(dosStubCode.data(), dosStubCodeLength, dosStubCodeStart, 0, dosStubCodeLength) < dosStubCodeLength)
		{
			throw std::runtime_error{ "File is not DOS EXE file" };
		}
		uint16_t checksum = CalculateChecksum(file, ctx, false);
		if (mzHeader.e_csum != 0U)
		{
			metadata.checksumValid = ChecksumValid(checksum, mzHeader.e_csum);
		}
		// Implement reading of undocumented Microsoft "Rich" header
		// https://bytepointer.com/articles/the_microsoft_rich_header.htm
	}

	uint16_t MzParser::CalculateChecksum(const MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx, bool includeBytesBeyondActualDosPart)
	{
		uint32_t actualMzFileLength = ctx.metadata.actualDosDataLength;
		uint32_t nominalMzFileLength = ctx.metadata.nominalDosDataLength;
		// Nominal may be greater than actual
		MemoryMappedIO::MemoryMappedFileRegion region
			= includeBytesBeyondActualDosPart
			? file.MapRegion(0, nominalMzFileLength)
			: file.MapRegion(0, actualMzFileLength);
		// File format is little endian
		const unsigned char* const start = region.begin();
		uint16_t checksum{};
		for (const unsigned char* ptr = start; ptr != start + DOS_HEADER_CHECKSUM_OFFSET; ptr += sizeof(uint16_t))
		{
			uint16_t word;
			Memory::Read<std::endian::little>(ptr, word);
			checksum += word;
		}
		auto regionLength = region.length();
		// We treat the Checksum field as 0x0000, so omit addition
		for (size_t i = DOS_HEADER_CHECKSUM_OFFSET + sizeof(uint16_t); i < regionLength; i += 2)
		{
			uint16_t word;
			Memory::Read<std::endian::little>(start + i, word);
			checksum += word;
		}
		// If count of bytes is even
		if (regionLength % 2 == 1)
		{
			// treat the last byte as uint16 with high byte zeroed
			// Zero-extend read byte
			checksum += static_cast<uint16_t>(region.at(regionLength - 1));
		}
		// If the nominal size is greater than the actual size of DOS file,
		// the remaining *virtual* bytes will be null, so don't need to add it.
		return checksum;
	}

	bool MzParser::ChecksumValid(uint16_t checksum, uint16_t complementChecksum)
	{
		return checksum + complementChecksum == std::numeric_limits<uint16_t>::max();
	}

	ExecutableObjectFormat MzExecutable::format() const
	{
		return ExecutableObjectFormat::Mz;
	}

	ExecutableType MzExecutable::type() const
	{
		return _exeType;
	}

	Eyesol::Cpu::ArchType MzExecutable::arch() const
	{
		return Cpu::ArchType::X86_16;
	}

	uint64_t MzExecutable::length() const
	{
		return _metadata.actualDosDataLength;
	}

	// Maybe empty if file is memory-only
	std::string MzExecutable::path() const
	{
		return _file.path();
	}

	bool MzExecutable::ContainsDebugInfo() const
	{
		return false;
	}

	std::shared_ptr<DebugInfo> MzExecutable::GetDebugInfo() const
	{
		throw std::logic_error{ "File doesn't contain debug info" };
	}

	void MzExecutable::init(MemoryMappedIO::MemoryMappedFile file, const MzParseContext& ctx)
	{
		_file = std::move(file);
		_exeType = ctx.type.value();
		_metadata = ctx.metadata;
	}

	std::vector<std::string> MzParser::_supportedFormatNames{ "MZ" };
}