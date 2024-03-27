// Eyesol.PeReader.cpp : Определяет экспортируемые функции для DLL.
//

#include "framework.h"
#include "Eyesol.PeReader.h"


// Пример экспортированной переменной
EYESOLPEREADER_API int nEyesolPeReader=0;

// Пример экспортированной функции.
EYESOLPEREADER_API int Eyesol::fnEyesolPeReader(void)
{
    return 0;
}

// Конструктор для экспортированного класса.
Eyesol::CEyesolPeReader::CEyesolPeReader()
{
    return;
}
