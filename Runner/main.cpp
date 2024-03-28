#include <MemoryMappedIO.hpp>
#include <WindowsConsoleOutputFix.hpp>
#include <Eyesol.PeReader.hpp>


int main()
{
	Eyesol::Windows::FixStdStreams();
	try
	{
		Eyesol::MemoryMappedIO::MemoryMappedFile file("C:\\Windows\\explorer.exe");
		auto fileLength = file.length();
		std::cout << "File length: " << fileLength << std::endl;
		auto mappedFragment = file.mapRegion(65535, 2);
		auto ptr = mappedFragment.begin();
		for (auto byte : mappedFragment)
		{
			std::cout << byte;
		}
		std::cout << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return Eyesol::start();
}