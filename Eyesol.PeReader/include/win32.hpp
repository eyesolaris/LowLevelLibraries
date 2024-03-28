#pragma once

#if defined _WIN32
#	define WIN32_LEAN_AND_MEAN             // Исключение редко используемых компонентов из заголовков Windows
// Файлы заголовков Windows
#	include <windows.h>
#	undef min
#	undef max
#else
#	error "NOT A WINDOWS"
#endif