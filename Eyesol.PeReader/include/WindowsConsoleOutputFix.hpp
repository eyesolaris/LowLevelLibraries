#if !defined _WINDOWSCONSOLEOUTPUTFIX_H_
#	define _WINDOWSCONSOLEOUTPUTFIX_H_
#	if defined _WIN32
#		include "framework.hpp"
#		include "Strings.hpp"

namespace Eyesol::Windows
{
	EYESOLPEREADER_API void FixStdStreams();
}
#	endif
#endif