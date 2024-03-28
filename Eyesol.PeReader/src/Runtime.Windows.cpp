#if defined _WIN32
#include "Runtime.hpp"
#include <win32.hpp>

namespace
{
	inline std::size_t QueryAllocationGranularity()
	{
		::SYSTEM_INFO info{};
		::GetSystemInfo(&info);
		return info.dwAllocationGranularity;
	}
}

std::size_t Eyesol::Runtime::AllocationGranularity()
{
	static std::size_t data = QueryAllocationGranularity();
	return data;
}
#endif