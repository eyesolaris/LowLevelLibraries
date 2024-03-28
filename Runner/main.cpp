#include <MemoryMappedIO.hpp>
#include <WindowsConsoleOutputFix.hpp>
#include <Eyesol.PeReader.hpp>
#include <vector>
#include <array>

int main()
{
	Eyesol::Windows::FixStdStreams();
	try
	{
		//Eyesol::MemoryMappedIO::MemoryMappedFile file("D:\\utf8.txt");
		Eyesol::MemoryMappedIO::MemoryMappedFile file("C:\\Windows\\explorer.exe");
		std::array<unsigned char, 65536 * 2> arr{};
		file.read(arr.data(), arr.size(), 1, 0, arr.size());
		auto fileLength = file.length();
		std::cout << "File length: " << fileLength << std::endl;
		std::vector<unsigned char> buffer;
		buffer.reserve(fileLength);
		auto current = file.begin();
		auto end = file.end();
		for (auto byte : file)
		{
			buffer.push_back(byte);
			std::cout << byte;
			++current;
		}
		/*for (auto ch : file)
		{
			buffer.push_back(ch);
		}*/
		bool ok = current < end;
		auto bufLength = buffer.size();
		std::cout << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return Eyesol::start();
}