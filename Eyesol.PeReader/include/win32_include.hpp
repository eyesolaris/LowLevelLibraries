#pragma once

#if defined _WIN32
#	define WIN32_LEAN_AND_MEAN             // ���������� ����� ������������ ����������� �� ���������� Windows
// ����� ���������� Windows
#	include <windows.h>
#	undef min
#	undef max
#else
#	error "NOT A WINDOWS"
#endif