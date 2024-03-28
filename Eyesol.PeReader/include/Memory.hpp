#if !defined _MEMORY_H_
#	define _MEMORY_H_
#	include <framework.hpp>
namespace Eyesol::Memory
{
	template <typename T>
	concept PrimitiveType = std::is_trivial_v<T> && std::has_unique_object_representations_v<T>;

	template <PrimitiveType T, std::endian DataEndianness = std::endian::native>
	T UnalignedRead(void* ptr)
	{
		if constexpr (DataEndianness == std::endian::native)
		{
			if constexpr (UnalignedAccessAllowed)
			{
				// unaligned access allowed
				// actually an undefined behavior if unaligned, according to a standard.
				// but in this case, it always should work
				return *reinterpret_cast<T*>(ptr);
			}
			else
			{
				std::memcpy(&data, ptr, sizeof(T));
				return std::move(data);
			}
		}
		else
		{
			T data;
			char* dataPtr = reinterpret_cast<char*>(&data);
			char* bufPtr = reinterpret_cast<char*>(ptr);
			for (std::size_t i = 0; i < sizeof(T); i++)
			{
				dataPtr[i] = bufPtr[sizeof(T) - i - 1];
			}
			return std::move(data);
		}
	}

	template <PrimitiveType T>
	T UnalignedRead(void* ptr, std::endian dataEndianness)
	{
		static_assert(!Eyesol::Cpu::IsMixedEndianness, "Mixed endianness is not supported");

		if (dataEndianness == std::endian::little)
		{
			return UnalignedRead<T, std::endian::little>(ptr);
		}
		else // if (dataEndianness == std::endian::big)
		{
			return UnalignedRead<T, std::endian::big>(ptr);
		}
	}
}
#endif