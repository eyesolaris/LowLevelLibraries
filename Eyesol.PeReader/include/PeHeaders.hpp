#if !defined _PEHEADERS_H_
#	define _PEHEADERS_H_
#	include "framework.hpp"
#   include "MemoryMappedIO.hpp"
#   include <optional>

// ** Base address: the address at which a binary is loaded.
// It is an address of the first byte of any bibary loaded into memory.
// Usually it will have the next values: 0x10000000 for a DLL,
// 0x00010000 for a Windows CE EXE, and 0x00400000 default for Windows NT EXEs.
// ** Virtual Address (VA): also called Linear Address. It is the address
// used in the binary to reference data or code in an absolute way,
// this value is dependent on the base address value.
// ** Relative Virtual Address (RVA): this value is widely used on PE Header
// to avoid the dependency of the base address. This value is added
// to the base address to get the virtual address, so in this way it will be easy
// to get values from the binary.
// RVA = VA - Base Address
// VA = RVA + Base Address
// ** Offset, or File Pointer: value pointing to the physical address in the file.

namespace Eyesol::Executables
{
    struct FileLocation
    {
        std::uint64_t AbsoluteOffset;
        std::size_t Length;

        bool empty() const { return Length == 0; }
    };

    namespace Mz
    {
        constexpr std::endian DOS_ENDIANNESS = std::endian::little;
        constexpr std::size_t BASE_ADDRESS_GRANULARITY = 64 * 1024; // 64 KiB
        constexpr std::size_t BASE_ADDRESS_32_SIZE = sizeof(std::uint32_t);
        constexpr std::size_t BASE_ADDRESS_64_SIZE = sizeof(std::uint64_t);
        constexpr std::uint16_t DOS_HEADER_MAGIC = 0x5A4D; // 'MZ'
        // A DOS .EXE page size, in bytes
        constexpr std::size_t DOS_PAGE_SIZE = 512;
        constexpr std::size_t DOS_HEADER_PARAGRAPH_SIZE = 16;
        constexpr std::size_t DOS_HEADER_ALIGNMENT = DOS_HEADER_PARAGRAPH_SIZE;

        // DOS Header formatted area includes all fields up to e_ovno (including)
        constexpr std::size_t DOS_HEADER_FORMATTED_AREA_START = 0;
        constexpr std::size_t DOS_HEADER_FORMATTED_AREA_END = 0x1C;
        constexpr std::size_t DOS_HEADER_EXTENDED_END = 0x40;

        constexpr std::size_t DOS_HEADER_RELOCATION_SIZE = sizeof(std::uint32_t);

        constexpr std::size_t DOS_HEADER_CHECKSUM_OFFSET = 0x12U;

        struct MicrosoftRichHeaderElement
        {
            std::uint16_t id;
            std::uint16_t use_count;
        };

        struct MzDosHeaderRelocation
        {
            std::uint16_t offset;
            std::uint16_t segment;
        };

        // MS-DOS 2.0 Extended Compatible EXE Header
        struct MzDosHeader
        {
            /* 00: MZ Header signature */
            std::uint16_t e_magic;
            /* 02: Bytes on last page of file */
            std::uint16_t e_cblp;
            /* 04: Pages in file */
            std::uint16_t e_cp;
            /* 06: Relocations */
            std::uint16_t e_crlc;
            /* 08: Size of header in paragraphs */
            std::uint16_t e_cparhdr;
            /* 0a: Minimum extra paragraphs needed */
            std::uint16_t e_minalloc;
            /* 0c: Maximum extra paragraphs needed */
            std::uint16_t e_maxalloc;
            /* 0e: Initial (relative) SS value */
            std::uint16_t e_ss;
            /* 10: Initial SP value */
            std::uint16_t e_sp;
            /* 12: Checksum (complement) */
            std::uint16_t e_csum;
            /* 14: Initial IP value */
            std::uint16_t e_ip;
            /* 16: Initial (relative) CS value */
            std::uint16_t e_cs;
            /* 18: File address of relocation table */
            std::uint16_t e_lfarlc;
            /* 1a: Overlay number */
            std::uint16_t e_ovno;
            // From now, all fields are *vendor-specific*
            // in terms of a DOS MZ file format
            /* 1c: Reserved words */
            std::uint16_t e_res[4];
            /* 24: OEM identifier (for e_oeminfo) */
            std::uint16_t e_oemid;
            /* 26: OEM information; e_oemid specific */
            std::uint16_t e_oeminfo;
            /* 28: Reserved words */
            std::uint16_t e_res2[10];
            /* 3c: Offset to extended header */
            std::uint32_t e_lfanew;
        };

        struct MzFileMetadata
        {
            MzFileMetadata()
                : header{},
                nominalDosDataLength{},
                actualDosDataLength{},
                actualDosFileLengh{},
                dosHeaderLoc{},
                dosRelocationsLoc{},
                dosStubCodeLoc{}
            {
            }

            MzDosHeader header;
            // A nominal DOS file length after loading into memory by DOS loader.
            // Actual DOS data in PE file may be less than this
            std::uint32_t nominalDosDataLength;
            // A length of the actual DOS MZ file data. Trailing data is not included.
            // In MZ-derived formats, actual MZ data may be less than nominal
            std::uint32_t actualDosDataLength;
            // Total file size, including all trailing data
            std::uint64_t actualDosFileLengh;
            FileLocation dosHeaderLoc;
            // Located inside the DOS header
            static constexpr FileLocation dosFormattedAreaLoc{ DOS_HEADER_FORMATTED_AREA_START, DOS_HEADER_FORMATTED_AREA_END };
            /* Can be used for a generic DOS MZ format */
            // Located inside the DOS header
            // FileLocation _dosHeaderVendorSpecificDataLoc;
            FileLocation dosRelocationsLoc;
            FileLocation dosStubCodeLoc;

            std::optional<bool> checksumValid;

            // Here, may be overlays data or other vendor-specific information
            // std::vector<unsigned char> _dosHeaderVendorSpecificData;
            std::vector<MzDosHeaderRelocation> dosRelocations;
            std::vector<unsigned char> dosStubCode;
        };
    }

    namespace Pe
    {
        constexpr std::endian PE_COFF_ENDIANNESS = std::endian::little;
        
        // By recomendation of Microsoft: https://web.archive.org/web/20160609191558/http://msdn.microsoft.com/en-us/gg463119.aspx
        constexpr std::size_t PE_HEADER_ALIGNMENT = 8;

        constexpr std::uint32_t PE_SIGNATURE = 0x00004550U;

        /*class EYESOLPEREADER_API Pe32File
        {
        public:
            Pe32File(MemoryMappedIO::MemoryMappedFile file)
                : _memFile{ std::move(file) }
            {
            }

            std::uint32_t DosFileSize() const
            {
                return _mzData.nominalDosFileLength;
            }

        private:
            MemoryMappedIO::MemoryMappedFile _memFile;

            Mz::MzFileMetadata _mzData;
        };*/
    }
}
#endif // _PEHEADERS_H_