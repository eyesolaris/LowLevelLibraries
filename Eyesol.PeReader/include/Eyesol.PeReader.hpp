#if !defined _EYESOL_PEREADER_H_
#	define _EYESOL_PEREADER_H_
#	include <framework.hpp>

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
#endif // _EYESOL_PEREADER_H_