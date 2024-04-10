#if !defined _MEMORY_H_
#	define _MEMORY_H_
#	include <framework.hpp>

namespace Eyesol::Memory
{
	static_assert(!Eyesol::Cpu::IsMixedEndianness, "Mixed endianness is not supported");

	template <std::unsigned_integral T>
	inline T AlignAddress(T oldAddress, std::size_t alignment)
	{
		if (oldAddress % alignment != 0)
		{
			oldAddress = (oldAddress / alignment + 1) * alignment;
		}
		return oldAddress;
	}

	template <typename T>
	concept IsAnyChar
		= std::is_same_v<T, unsigned char>
		|| std::is_same_v<T, signed char>
		|| std::is_same_v<T, char>;

	template <typename T>
	concept PrimitiveType = std::is_trivial_v<T> && std::has_unique_object_representations_v<T>;

	template <typename T>
	concept SubscriptableWithAnyCharValue = requires(T t)
	{
		{ t[std::declval<std::size_t>()] } -> IsAnyChar;
	};

	template <std::endian DataEndianness, PrimitiveType T>
	void Read(const void* ptr, T& obj)
	{
		if constexpr (DataEndianness == std::endian::native)
		{
			obj = *reinterpret_cast<const T*>(ptr);
		}
		else
		{
			char* dataPtr = reinterpret_cast<char*>(&obj);
			const char* bufPtr = reinterpret_cast<const char*>(ptr);
			for (std::size_t i = 0; i < sizeof(T); i++)
			{
				dataPtr[i] = bufPtr[sizeof(T) - i - 1];
			}
		}
	}

	template <std::endian DataEndianness, PrimitiveType T, SubscriptableWithAnyCharValue S>
	void Read(const S& container, std::size_t startOffset, T& obj)
	{
		unsigned char* dataPtr = reinterpret_cast<unsigned char*>(&obj);
		for (std::size_t i = 0; i < sizeof(T); i++)
		{
			if constexpr (DataEndianness == std::endian::native)
			{
				dataPtr[i] = container[startOffset + i];
			}
			else
			{
				dataPtr[i] = container[startOffset + sizeof(T) - i - 1];
			}
		}
	}

	template <PrimitiveType T, SubscriptableWithAnyCharValue S>
	void Read(const S& container, std::size_t startOffset, std::endian dataEndianness, T& obj)
	{
		unsigned char* dataPtr = reinterpret_cast<unsigned char*>(&obj);
		if (dataEndianness == std::endian::native)
		{
			for (std::size_t i = 0; i < sizeof(T); i++)
			{
				dataPtr[i] = container[startOffset + i];
			}
		}
		else
		{
			for (std::size_t i = 0; i < sizeof(T); i++)
			{
				dataPtr[i] = container[startOffset + sizeof(T) - i - 1];
			}
		}
	}

	template <std::endian DataEndianness, PrimitiveType T>
	void UnalignedRead(const void* ptr, T& obj)
	{
		if constexpr (DataEndianness == std::endian::native)
		{
			if constexpr (UnalignedAccessAllowed)
			{
				// unaligned access allowed
				// actually an undefined behavior if unaligned, according to a standard.
				// but in this case, it always should work
				obj = *reinterpret_cast<const T*>(ptr);
			}
			else
			{
				std::memcpy(&obj, ptr, sizeof(T));
			}
		}
		else
		{
			char* dataPtr = reinterpret_cast<char*>(&obj);
			const char* bufPtr = reinterpret_cast<const char*>(ptr);
			for (std::size_t i = 0; i < sizeof(T); i++)
			{
				dataPtr[i] = bufPtr[sizeof(T) - i - 1];
			}
		}
	}

	template <PrimitiveType T>
	void UnalignedRead(const void* ptr, std::endian dataEndianness, T& obj)
	{
		if (dataEndianness == std::endian::little)
		{
			return UnalignedRead<std::endian::little, T>(ptr, obj);
		}
		else // if (dataEndianness == std::endian::big)
		{
			return UnalignedRead<std::endian::big, T>(ptr, obj);
		}
	}
}
#endif // _MEMORY_H_