#if !defined _WIN32_INCLUDE_H_
#	define _WIN32_INCLUDE_H_

#	if defined _WIN32
#		define WIN32_LEAN_AND_MEAN             // Исключение редко используемых компонентов из заголовков Windows
// Файлы заголовков Windows
#		include <windows.h>
#		undef min // to get rid of unnoying problems
#		undef max // with std functions min() and max()
#	else
#		error "NOT A WINDOWS"
#	endif
#endif