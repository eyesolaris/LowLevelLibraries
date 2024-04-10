#if !defined _EXCEPTIONS_H_
#	define _EXCEPTIONS_H_
#	include <exception>
#	include <stdexcept>
#	include <string>
namespace Eyesol
{
	class EyesolException : public std::logic_error
	{
	public:
		EyesolException(std::string message)
			: std::logic_error{ message }
		{
		}

		EyesolException(const char* str)
			: std::logic_error{ str }
		{
		}
	};

	class NotImplementedException : public EyesolException
	{
	public:
		NotImplementedException()
			: EyesolException{ "Function not implemented" }
		{
		}
	};
}
#endif