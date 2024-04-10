#if !defined _RUNTIME_H_
#	define _RUNTIME_H_
#	include <cstddef>

namespace Eyesol::Runtime
{
	std::size_t AllocationGranularity();

	// TODO: implement properly
	constexpr bool PosixCompatible = false;
}
#endif // _RUNTIME_H_