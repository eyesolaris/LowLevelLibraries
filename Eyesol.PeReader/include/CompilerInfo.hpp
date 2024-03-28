#if !defined _CPU_H_
#	define _CPU_H_

#	define _X86_VERSION_IMPL_ None
#	define _ARM_VERSION_IMPL_ None
#	define _IA64_VERSION_IMPL_ None

#	if defined _MSC_VER
#		define _COMPILER_TYPE_IMPL_ MSVC
#	elif defined __GNUC__
#		define _COMPILER_TYPE_IMPL_ Gcc
#	elif defined __clang__
#		define _COMPILER_TYPE_IMPL_ Clang
#	elif defined __EMSCRIPTEN__
#		define _COMPILER_TYPE_IMPL_ Emscripten
#	elif defined __MINGW32__
#		define _COMPILER_TYPE_IMPL_ MinGW32
#	elif
		define _COMPILER_TYPE_IMPL_ Unknown
#	endif

#	if defined _WIN32
#		define _OS_TYPE_IMPL_ Windows
#	elif defined __ANDROID__
#		define _OS_TYPE_IMPL_ Android
#	elif defined __linux__
#		define _OS_TYPE_IMPL_ Linux
#	elif defined __APPLE__
#		define _OS_TYPE_IMPL_ Darwin
#	elif defined __asmjs__
#		define _OS_TYPE_IMPL_ AsmJS
#	elif
#		define _OS_TYPE_IMPL_ Unknown
#	endif


#	if defined(__IA64__) \
 || _M_IA64 /*MSVC++*/ \
 || defined(__ia64__) /*GCC,Clang,Intel*/
// ... Itanium/IntelArchitecture 64
#		define _PROCESSOR_ARCHITECTURE_IMPL_ Ia64
#	elif __X86_64__ /*OpenWatcom*/ \
   || _M_X64 /*MSVC++*/ \
   || _M_AMD64 /*MSVC++ compatibility with older compilers*/ \
   || defined(__x86_64__) /*GCC,Clang,Intel*/
// ... x66-64/IA32e
#		define _PROCESSOR_ARCHITECTURE_IMPL_ X86_64
#		define _X86_VERSION_IMPL_ X64
#	elif __386__ || _M_I386 /*OpenWatcom*/ \
   || (defined(__DMC__) && defined(_M_IX86)) /*DigitalMars*/ \
   || (defined(_MSC_VER) && _M_IX86) /*MSVC++*/ \
   || defined(__i386__) /*GCC,Clang,Intel*/
// ... x86-32
#			define _X86_VERSION_IMPL_ X86_32
#		if _M_IX86 >= 600
		// Pentium Pro instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ PentiumPro
#		elif _M_IX86 >= 500
		// Pentium instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ Pentium
#		elif _M_IX86 >= 400
		// 80486 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _80486
#		elif _M_IX86 >= 300
		// 80386 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _80386
#		else
		// This must be GCC or Clang.
#			define _PROCESSOR_ARCHITECTURE_IMPL_ Unknown
#		endif
#	elif __I86__ || _M_I86 /*OpenWatcom*/ \
   || (defined(__DMC__) && !defined(_M_IX86)) /*DigitalMars*/
// ... x86-16
#		define _PROCESSOR_ARCHITECTURE_IMPL_ X86_16
#		if _M_IX86 >= 600
		// Pentium Pro instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ PentiumPro
#		elif _M_IX86 >= 500
		// Pentium instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ Pentium
#		elif _M_IX86 >= 400
		// 80486 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _80486
#		elif _M_IX86 >= 300
		// 80386 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _80386
#		elif _M_IX86 >= 200 || _M_I286
		// 80286 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _80286
#		elif _M_IX86 >= 100
		// 80186 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _80186
#		else
		// 8086 instructions and instruction scheduling
#			define _X86_VERSION_IMPL_ _8086
#		endif
#	endif
/*******************************************************
 * TODO : Implement more compiler versions detection,  *
 * add an ARM and IA64 version detection               *
 *******************************************************/

namespace Eyesol
{
	namespace Cpu
	{
		enum class ArchType
		{
			Unknown,
			X86_16,
			X86_32,
			X86_64,
			Arm32,
			Arm64,
			// Intel IA64 (Itanium)
			Ia64,
		};

		constexpr ArchType CurrentProcessorArch = ArchType::_PROCESSOR_ARCHITECTURE_IMPL_;

		constexpr bool IsLittleEndian = std::endian::native == std::endian::little;
		constexpr bool IsBigEndian = std::endian::native == std::endian::big;
		constexpr bool IsMixedEndianness
			= std::endian::native != std::endian::little
			&& std::endian::native != std::endian::big;
		constexpr bool IndifferentEndianness
			= std::endian::native == std::endian::little
			&& std::endian::native == std::endian::big;

		namespace X86
		{
			enum class X86VersionType
			{
				None,
				Unknown,
				_8086,
				_80186,
				_80286,
				_80386,
				_80486,
				Pentium,
				PentiumPro,
				X64
			};

			constexpr X86VersionType CurrentX86Version = X86VersionType::_X86_VERSION_IMPL_;
		}

		namespace Ia64
		{
			enum class Ia64VersionType
			{
				None,
				Unknown
			};

			constexpr Ia64VersionType CurrentIa64Version = Ia64VersionType::_IA64_VERSION_IMPL_;
		}

		namespace Arm
		{
			enum class ArmVersionType
			{
				None,
				Unknown
			};

			constexpr ArmVersionType CurrentArmVersion = ArmVersionType::_ARM_VERSION_IMPL_;
		}
	}

	namespace Compiler
	{
		enum class CompilerType
		{
			Unknown,
			MSVC,
			Gcc,
			Clang,
			MinGW32,
			Emscripten,
			OpenWatcom,
			DigitalMars,
		};

		constexpr CompilerType CurrentCompiler = CompilerType::_COMPILER_TYPE_IMPL_;
	}

	namespace OS
	{
		enum class OSType
		{
			Unknown,
			Windows,
			Linux,
			Android,
			// macOS & iOS
			Darwin,
			AsmJS
		};

		constexpr OSType CurrentOS = OSType::_OS_TYPE_IMPL_;
	}

	namespace Memory
	{
		constexpr bool UnalignedAccessAllowed = Eyesol::Compiler::CurrentCompiler == Eyesol::Compiler::CompilerType::MSVC
			&& (Eyesol::Cpu::CurrentProcessorArch == Eyesol::Cpu::ArchType::X86_32
				|| Eyesol::Cpu::CurrentProcessorArch == Eyesol::Cpu::ArchType::X86_64);
	}
}
#endif