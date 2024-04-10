#include <Eyesol.PeReader.hpp>
#include "MemoryMappedIO.hpp"
#include "PeHeaders.hpp"
#include "MzParser.hpp"
#include "PeParser.hpp"

int Eyesol::start()
{
	Eyesol::MemoryMappedIO::MemoryMappedFile file("C:\\Windows\\explorer.exe");
	/*Eyesol::Executables::Pe::PeDosHeader header;
	static_assert(std::endian::native == std::endian::little);
	file.Read(header, 0, std::endian::native);*/
	Eyesol::Executables::Pe::PeParser parser;
	std::exception_ptr eptr{};
	auto parsedFile = parser.TryParse(file, &eptr);
	auto mzFile = std::dynamic_pointer_cast<Eyesol::Executables::Mz::MzExecutable>(parsedFile);
	auto&& metadata = mzFile->metadata();
	//Eyesol::Executables::Pe::Pe32File peFile{ std::move(file) };
	//auto sz = peFile.DosFileSize();
	return 0;
}