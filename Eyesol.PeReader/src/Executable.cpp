#include "Executable.hpp"
#include "Exceptions.hpp"

namespace Eyesol::Executables
{
	Executable::~Executable() noexcept
	{
	}

	ExecutableParser::~ExecutableParser() noexcept
	{
	}

	std::vector<ExecutableType> IAdditionalExecutableTypes::AdditionalTypes() const
	{
		return {};
	}

	std::vector<Eyesol::Cpu::ArchType> IAdditionalExecutableTypes::AdditionalArchs() const
	{
		return {};
	}

	bool IAdditionalExecutableTypes::IsFatBinary() const
	{
		return false;
	}


	CompoundExecutableParser::CompoundExecutableParser(std::vector<std::shared_ptr<ExecutableParser>> orderedParsers)
		: _orderedParsers{ std::move(orderedParsers) }
	{
		std::set<std::string> namesSet;
		for (auto p : _orderedParsers)
		{
			auto&& namesRef = p->SupportedFormatNames();
			namesSet.insert(namesRef.begin(), namesRef.end());
		}
		std::copy(namesSet.begin(), namesSet.end(), std::back_inserter(_supportedFormatNames));
	}

	const std::vector<std::string>& CompoundExecutableParser::SupportedFormatNames() const noexcept
	{
		return _supportedFormatNames;
	}

	bool CompoundExecutableParser::IsTypeSupported(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const
	{
		auto parserPtr = FindSuitableParser(file, format, type);
		return parserPtr != nullptr;
	}

	std::shared_ptr<Executable> CompoundExecutableParser::TryParse(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, std::exception_ptr* excPtr) const
	{
		auto parserPtr = FindSuitableParser(file, nullptr, nullptr);
		if (parserPtr == nullptr)
		{
			return nullptr;
		}
		return (*parserPtr)->TryParse(file, excPtr);
	}

	const std::shared_ptr<ExecutableParser>* CompoundExecutableParser::FindSuitableParser(const Eyesol::MemoryMappedIO::MemoryMappedFile& file, ExecutableObjectFormat* format, ExecutableType* type) const
	{
		for (auto&& parserPtr : _orderedParsers)
		{
			if (parserPtr->IsTypeSupported(file, format, type))
			{
				return &parserPtr;
			}
		}
		return nullptr;
	}

	namespace
	{
		std::vector<Eyesol::Executables::ExecutableParser> InitializeExecutables()
		{
			return {};
		}
	}

	extern const std::vector<ExecutableParser> SupportedParsers
		= InitializeExecutables();

	////// Extendable parser

}