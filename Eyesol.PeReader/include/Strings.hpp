#if !defined _STRINGS_H_
#	define _STRINGS_H_
#	include <framework.hpp>
namespace Eyesol
{
	EYESOLPEREADER_API std::wstring utf8string_to_wstring(std::string str);
	EYESOLPEREADER_API std::string wstring_to_utf8string(std::wstring str);
	EYESOLPEREADER_API std::wstring u16string_to_wstring(std::u16string str);
}
#endif