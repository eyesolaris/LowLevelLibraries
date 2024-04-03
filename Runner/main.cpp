#include <MemoryMappedIO.hpp>
#include <WindowsConsoleOutputFix.hpp>
#include <Eyesol.PeReader.hpp>
#include <vector>
#include <array>
#include <chrono>

int main()
{
	Eyesol::Windows::FixStdStreams();
	Eyesol::Compiler::PrintCompilerFeatures(Eyesol::Compiler::CompilerFeaturesToPrint{});
	try
	{
		//Eyesol::MemoryMappedIO::MemoryMappedFile file("D:\\utf8.txt");
		Eyesol::MemoryMappedIO::MemoryMappedFile file("C:\\Windows\\explorer.exe");
		std::array<unsigned char, 65536 * 2> arr{};
		file.read(arr.data(), arr.size(), 1, 0, arr.size());
		auto fileLength = file.length();
		std::cout << "File length: " << fileLength << " (" << fileLength / 1024. / 1024 << " MiB)" << std::endl;
		std::vector<unsigned char> buffer(fileLength, 0);
		//buffer.reserve(fileLength);

		auto t1 = std::chrono::steady_clock::now();
		std::size_t currentPos = 0;
		for (auto region : file)
		{
			std::memcpy(buffer.data() + currentPos, region.begin(), region.length());
			currentPos += region.length();
		}
		auto t2 = std::chrono::steady_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>((t2 - t1));
		std::cout << std::endl;
		std::cout << "Elapsed: " << time / 1. << std::endl;
		auto bufLength = buffer.size();
		std::cout << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return Eyesol::start();
}