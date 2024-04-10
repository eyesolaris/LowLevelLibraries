#if !defined _EXECUTABLE_H_
#	define _EXECUTABLE_H_
#	include <set>
#	include <exception>
#	include "framework.hpp"
#	include "Arch.hpp"
#	include "Compiler.hpp"
#	include "MemoryMappedIO.hpp"

namespace Eyesol::Executables
{
	enum class ExecutableObjectFormat
	{
		InvalidValue,
		/* Microsoft formats: */
		ComDos,
		// DOS EXE
		Mz,
		// Win3.x EXE (New Executable)
		Ne,
		Le,
		Pe,
		Pe32Plus,
		CoffObject,
		// Microsoft .LIB static library
		CoffArchiveLib,
		// .NET Assembly
		DotNetAssembly,
		// Mixed-mode .NET Assembly
		DotNetMixedAssembly,

		/* *nix formats:  */
		A_out,
		Elf,

		/* Apple formats */
		MachO,
		
		/* Java formats: */
		// Java .class file
		JavaClass,
		// JAR file
		JavaArchive,

		/* CP/M formats: */
		ComCpm,
	};

	enum class ExecutableType
	{
		InvalidValue,
		Executable,
		StaticLib,
		DynamicLib,
		ObjectFile
	};

	enum class DebugInfoType
	{
		Sym, // Old 16-bit Microsoft undocumented symbol format
		Coff,
		Pdb,
		PortablePdb
	};

	class EYESOLPEREADER_API DebugInfo
	{
	public:
		virtual DebugInfoType type() const = 0;
	};

	class EYESOLPEREADER_API IAdditionalExecutableTypes
	{
	public:
		virtual std::vector<ExecutableType> AdditionalTypes() const;
		virtual std::vector<Eyesol::Cpu::ArchType> AdditionalArchs() const;
		virtual bool IsFatBinary() const;
	};

	class EYESOLPEREADER_API Executable : public IAdditionalExecutableTypes
	{
	public:
		virtual ~Executable() noexcept = 0;

		virtual ExecutableObjectFormat format() const = 0;
		virtual ExecutableType type() const = 0;
		virtual Eyesol::Cpu::ArchType arch() const = 0;

		virtual uint64_t length() const = 0;
		// Maybe empty if file is memory-only
		virtual std::string path() const = 0;

		virtual bool ContainsDebugInfo() const = 0;
		virtual std::shared_ptr<DebugInfo> GetDebugInfo() const = 0;
	};

	class EYESOLPEREADER_API ExecutableParser : public IAdditionalExecutableTypes
	{
	public:
		virtual ~ExecutableParser() noexcept = 0;

		virtual const std::vector<std::string>& SupportedFormatNames() const noexcept = 0;

		virtual bool IsTypeSupported(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const = 0;
		virtual std::shared_ptr<Executable> TryParse(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, std::exception_ptr* excPtr) const = 0;
	};

	class EYESOLPEREADER_API CompoundExecutableParser : public ExecutableParser
	{
	public:
		// Should place partially compatible parsers in a specific order
		CompoundExecutableParser(std::vector<std::shared_ptr<ExecutableParser>> orderedParsers);

		const std::vector<std::string>& SupportedFormatNames() const noexcept;
		bool IsTypeSupported(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const;
		std::shared_ptr<Executable> TryParse(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, std::exception_ptr* excPtr) const;

	private:
		std::vector<std::shared_ptr<ExecutableParser>> _orderedParsers;

		std::vector<std::string> _supportedFormatNames;

		const std::shared_ptr<ExecutableParser>* FindSuitableParser(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const;
	};

	// Used for parsers of extendable executable formats
	/*class ExtendableParser : public ExecutableParser
	{
	protected:
		// For example, PE parsers may extend MZ parser
		virtual bool IsDerivedType(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const;
		virtual std::shared_ptr<Executable> ParseDerivedTypeExecutable(const Eyesol::MemoryMappedIO::MemoryMappedFile& file) const;
	};*/

	EYESOLPEREADER_API extern const std::vector<ExecutableParser> SupportedParsers;
}
#endif