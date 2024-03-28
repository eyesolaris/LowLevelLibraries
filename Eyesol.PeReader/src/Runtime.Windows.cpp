#if defined _WIN32
#include "Runtime.hpp"
#include <win32.hpp>

std::size_t Eyesol::Runtime::AllocationGranularity()
{
	::SYSTEM_INFO info{};
	::GetSystemInfo(&info);
	return info.dwAllocationGranularity;
}
#endif