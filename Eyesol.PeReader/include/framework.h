// header.h: включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cmath>

// Приведенный ниже блок ifdef — это стандартный метод создания макросов, упрощающий процедуру
// экспорта из библиотек DLL. Все файлы данной DLL скомпилированы с использованием символа EYESOLPEREADER_EXPORTS
// Символ, определенный в командной строке. Этот символ не должен быть определен в каком-либо проекте,
// использующем данную DLL. Благодаря этому любой другой проект, исходные файлы которого включают данный файл, видит
// функции EYESOLPEREADER_API как импортированные из DLL, тогда как данная DLL видит символы,
// определяемые данным макросом, как экспортированные.
#ifdef EYESOLPEREADER_EXPORTS
#define EYESOLPEREADER_API __declspec(dllexport)
#else
#define EYESOLPEREADER_API __declspec(dllimport)
#endif