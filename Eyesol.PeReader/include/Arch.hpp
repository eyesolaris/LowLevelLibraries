#if !defined _ARCH_H_
#	define _ARCH_H_

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
			// .NET IL
			Il,
			// Java bytecode
			Java,

		};

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
		}

		namespace Ia64
		{
			enum class Ia64VersionType
			{
				None,
				Unknown
			};
		}

		namespace Arm
		{
			enum class ArmVersionType
			{
				None,
				Unknown
			};
		}
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
			AsmJS,
			DotNet,
			Java,
			Uefi
		};
	}
}
#endif