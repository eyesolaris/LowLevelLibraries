#if !defined _COMPILER_H_
#	define _COMPILER_H_
namespace Eyesol
{
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
	}
}
#endif