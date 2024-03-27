#include "framework.h"

namespace Eyesol
{
	// Этот класс экспортирован из библиотеки DLL
	class EYESOLPEREADER_API CEyesolPeReader {
	public:
		CEyesolPeReader(void);
		// TODO: добавьте сюда свои методы.
	};

	extern EYESOLPEREADER_API int nEyesolPeReader;

	EYESOLPEREADER_API int fnEyesolPeReader(void);

	EYESOLPEREADER_API int start();
}