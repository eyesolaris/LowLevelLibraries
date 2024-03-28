// Eyesol.PeReader.cpp : Определяет экспортируемые функции для DLL.
//

#include <framework.hpp>
#include <Eyesol.PeReader.hpp>


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
