#include "MzParser.hpp"
#include "Exceptions.hpp"
#include <stack>

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
		metadata.fullFileLength = fileLength;
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
		TryParseRichHeader(file, ctx);
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

	void MzParser::TryParseRichHeader(const MemoryMappedIO::MemoryMappedFile& file, MzParseContext& ctx)
	{
		// https://bytepointer.com/articles/the_microsoft_rich_header.htm
		// https://ntcore.com/files/richsign.htm

		// Structure of the "Rich" header.
		// Quotes from "Finding the Needle: A Study of the PE32
		// Rich Header and Respective Malware Triage":
		/*
		The header of the Rich Header is composed of four blocks where each is 0x04
bytes in length. The first block contains the ASCII representation of "DanS" - it
is speculated that "DanS" probably refers to Daniel Spalding who ran the linker
team in 1998 - while the next three blocks contain null padding. During
linking, this section is XORed with a generated checksum value that is contained
in the footer of the Rich Header.

The next section of the Rich Header is represented by an array of @comp.id
blocks. Each block is 0x08 bytes in length and contains information related to the
Product Identifier (ProdID), the minor version information for the compiler used
to create the product (mCV), and the number of times the product was included
during the linking process (Count). All fields are stored in little endian byte
order and XORed with the previously mentioned checksum value. The @comp.id
block consists of the following three values:

1. The mCV field contains the minor version information for the compiler used
to build the PE32 file. This version information allows the establishment of
a direct relationship between a particular version of the Microsoft Toolchain
and this @comp.id block in the Rich Header. For example, Microsoft's latest
Visual Studio 2015 release ships version 14.00.23918 of the MSVC compiler
(cl.exe). Therefore, object les created by this compiler will contain the
value of 0x5d6e. During the linking process for the building of a PE32, the
value will be added into the produced PE32's Rich Header in the mCV field
of the @comp.id block representing this object.
2. The ProdID provides information about the identity or type of the objects
used to build the PE32. With respect to type, each Visual Studio Version
produces a distinct range of values for this field. These values indicate whether
the referenced object was a C/C++ file, an assembly blob, or a resource file
before compilation as well as a subset of the compilation
ags. For example,
a C file compiled with Visual Studio 2015 will result in the value 0x104 being
copied into the Rich Header as ProdID in all PE32 files that include the
respective object file.
3. The Count field indicates how often the object identied by the former two
fields is referenced by this PE32 file. Using a simple C program as an example,
this fields will hold the value 0x1 zero-extended to span 32 bits, indicating
that the object file is used once by the PE32.

The final section of the Rich Header, the footer, is composed of three blocks
of information. The first block is 0x04 bytes in length and represents the ASCII
equivalent of "Rich". The next 0x04 bytes are the checksum value that are used
as the XOR key for enciphering the Rich Header. The final block section is used
as padding, typically null, and ensures that the total length of the Rich Header
is a multiple of 8. Unlike the previous two sections, the footer is not XORed with
the checksum value.
		*/

		uint32_t dataLength = ctx.metadata.actualDosDataLength;
		auto region = file.MapRegion(0, dataLength);
		const unsigned char* fileBegin = region.begin();
		const unsigned char* filePtr = fileBegin + dataLength - sizeof(uint32_t);
		bool richSignatureFound = false;
		while (filePtr != fileBegin)
		{
			uint32_t data;
			Memory::Read<std::endian::little>(filePtr, data);
			if (data == DOS_HEADER_RICH_SIGNATURE_MAGIC)
			{
				richSignatureFound = true;
				break;
			}
			filePtr -= sizeof(uint32_t);
		}
		if (!richSignatureFound)
		{
			return;
		}
		// Now filePtr points to a Rich signature
		size_t richSignatureOffset = static_cast<size_t>(static_cast<std::make_unsigned_t<ptrdiff_t>>(filePtr - fileBegin));
		uint32_t decryptKey;
		Memory::Read<std::endian::little>(filePtr + sizeof(uint32_t), decryptKey);
		filePtr -= sizeof(uint32_t);
		bool startFound = false;
		std::stack<RichHeaderElement> elements;
		size_t richHeaderStartOffset{};

		auto calculateHeaderStartOffset = [&filePtr, &fileBegin]() -> size_t
			{
				return static_cast<size_t>(static_cast<std::make_unsigned_t<ptrdiff_t>>(filePtr - fileBegin));
			};

		while (!startFound && (filePtr >= fileBegin))
		{
			uint32_t lastPart;
			Memory::Read<std::endian::little>(filePtr, lastPart);
			lastPart ^= decryptKey;
			if (lastPart == DOS_HEADER_DECRYPTED_DANS_SIGNATURE_MAGIC)
			{
				startFound = true;
				richHeaderStartOffset = calculateHeaderStartOffset();
				break;
			}
			filePtr -= sizeof(uint32_t);
			uint32_t firstPart;
			Memory::Read<std::endian::little>(filePtr, firstPart);
			firstPart ^= decryptKey;
			if (firstPart == DOS_HEADER_DECRYPTED_DANS_SIGNATURE_MAGIC)
			{
				startFound = true;
				richHeaderStartOffset = calculateHeaderStartOffset();
				break;
			}
			if (firstPart == 0 && lastPart == 0)
			{
				// Just an alignment element
				continue;
			}
			uint16_t id = static_cast<uint16_t>((firstPart & 0xFFFF0000) >> 16);
			uint16_t buildNumber = static_cast<uint16_t>((firstPart & 0xFFFF));
			elements.push(RichHeaderElement{ id, buildNumber, lastPart });
			filePtr -= sizeof(uint32_t);
		}
		if (!startFound)
		{
			return;
		}
		if (richHeaderStartOffset > std::numeric_limits<uint8_t>::max())
		{
			throw std::runtime_error{ "Something went wrong" };
		}
		size_t richHeaderEndOffset = ctx.metadata.actualDosDataLength;
		size_t richHeaderLength = richHeaderEndOffset - richHeaderStartOffset;
		auto&& richMetadataOpt = ctx.metadata.richMetadata;
		richMetadataOpt.emplace();
		auto&& richMetadata = richMetadataOpt.value();
		auto&& richHeader = richMetadata.header;
		richMetadata.loc = { richHeaderStartOffset, richHeaderLength };
		richMetadata.richSignatureOffset = richSignatureOffset;
		richHeader.decryptKey = decryptKey;
		auto&& vec = richHeader.elements;
		while (!elements.empty())
		{
			vec.push_back(elements.top());
			elements.pop();
		}
		if (vec.size() > std::numeric_limits<uint8_t>::max())
		{
			throw std::runtime_error{ "Something went wrong" };
		}
		// Now calculate the Rich header checksum
		// Header cannot end in unaligned address, so don't check
		// the end address alignment and don't read individual bytes
		// e_lfanew field is not included into the checksum

		// A checksum algorithm:
		/* While checksum values are traditionally straight forward to generate, the
Rich Header's checksum has interesting properties. Specifically, only 37 of each
@comp.id's 64 bits are calculated. As such, we present the following algorithm 2,
based on our reverse engineering work, which produces a valid Rich Header
checksum.
The Rich Header checksum is composed of two distinct values cd and cr that
are summed together. To calculate cr, we define the rol operator, which zero
extends its first argument to 32 bits and then performs a rotate left operation
equal to the second arguments value of the rst argument's bits. We define rol as:

rol(val, num) := ((val << num) & 0xffffffff)
| (val >> (32 − num))

	where << and >> denote logical left and right shift, and | and & are the binary
or/and operators. Then, the distinct parts of the checksum csum are calculated
in the following way:

1. For cd, all bytes contained in the MS-DOS header with the "lfanew" field
(ofset 0x3c) set to 0 are rotated to the left by their position relative to
the beginning of the MS-DOS header and summed together. Zeroing the
"lfanew" field is required as the linker can not fill in this value because
it does not know the final size of the Rich Header. Therefore is unable to
calculate the offset to the next header. Let n denote the length of the MS-DOS
header in bytes (most commonly 0x80) and let dos_i be the i-th byte of the
(modified) MS-DOS header:

cd = rol(dos_0, 0) + rol(dos_1, 1) + ... + rol(dos_n, n)

2. To calclulate cr, the algorithm first retrieves the list of m @comp.id blocks.
Then the algorithm combines the corresponding mCV and ProdID parts into
one 32 bit value. Finally, this value is rotated to the left by its respective
Count value:

cr = rol(ProdID_0 << 16 | mCV_0, Count_0 & 0x1f) + rol(ProdID_1 << 16 | mCV_1, Count_1 & 0x1f) + ...
  + rol(ProdID_m << 16 | mCV_m, Count_m & 0x1f)

It is noteworthy that despite the fact that Count beings as a 32 bit field, the
checksum algorithm only considers the least significant byte value (& 0xff).
Combined with the fact that m ≡ n mod 32 ⇒ rol(v, n) = rol(v, m), it is
sufficient to perform the calculation as indicated above.
The two values cd and cr, and the size of the MS-DOS header (0x80) are
then added together to form the final checksum value:
csum = 0x80 + cd + cr
		*/
		// A checksum of DOS header
		uint32_t cd{};
		// A checksum of decrypted "Rich" entries
		uint32_t cr{};

		// A supplementary function, rol(val, num)
		auto rol = [](uint32_t val, uint8_t num) -> uint32_t
			{
				uint32_t extendedVal = static_cast<uint32_t>(val);
				return ((extendedVal << num) & 0xffffffff) | (extendedVal >> static_cast<uint8_t>(32U - num));
			};

		for (uint8_t i = 0; i < static_cast<uint8_t>(richHeaderStartOffset); i++)
		{
			cd += rol(fileBegin[i], i);
		}
		// Fix out adding the e_lfanew value. We should have nulled it out,
		// but added the not nullified value.
		// So subtract it again to cancel out that's impact
		const unsigned char* lfanewPtr = fileBegin + DOS_HEADER_LFANEW_OFFSET;
		for (size_t i = 0; i < sizeof(uint32_t); i++)
		{
			cd -= rol(lfanewPtr[i], DOS_HEADER_LFANEW_OFFSET + i);
		}
		for (uint8_t i = 0; i < static_cast<uint8_t>(vec.size()); i++)
		{
			auto&& elem = vec[i];
			cr += rol((static_cast<uint32_t>(elem.id) << 16) + elem.build_number, elem.use_count & 0x1fU);
		}
		// Usually 0x80
		uint32_t fullDosHeaderSize = richHeaderStartOffset;
		ctx.metadata.actualDosDataLength = fullDosHeaderSize;
		auto correctedMzCodeLength = ctx.metadata.dosStubCode.size() - richHeaderLength;
		ctx.metadata.dosStubCode.resize(correctedMzCodeLength);
		//ctx.metadata.dosHeaderLoc = ;
		uint32_t checksum = fullDosHeaderSize + cd + cr;
		richMetadata.checksumValid = checksum == decryptKey;
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