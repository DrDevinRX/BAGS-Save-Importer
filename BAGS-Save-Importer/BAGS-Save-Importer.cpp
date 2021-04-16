#include <Windows.h>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>

namespace fs = std::filesystem;

namespace BagsSaveImporter
{
	namespace BagsHeader
	{
		const size_t HeaderSize = 52U;

		char* ReadHeaderBytes(std::fstream& stream)
		{
			char* buf = new char[HeaderSize];
			stream.read(buf, HeaderSize);
			return buf;
		};

		bool WriteHeaderToFile(std::ofstream& stream, const char* header)
		{
			stream.write(header, HeaderSize);
			return stream.good();
		}

		bool VerifyHeaderIsSame(std::fstream& stream, const char* header)
		{
			bool retVal = true;
			if (!stream.seekg(0))
			{
				return false;
			}

			char* testBuf = ReadHeaderBytes(stream);

			for (size_t i = 0; i < HeaderSize; i++)
			{
				if (testBuf[i] != header[i])
				{
					retVal = false;
					break;
				}
			}

			delete[] testBuf;
			return retVal;
		}
	}

	namespace BagsFiles
	{
		std::vector<fs::path> SlotDataFiles;
		fs::path SlotDataOriginal;
		bool FoundSlotDataOriginal = false;

		void GetSaveFiles(const std::wstring& baseDir)
		{
			for (const auto& entry : fs::recursive_directory_iterator(baseDir))
			{
				if (std::regex_match(entry.path().filename().wstring(), std::wregex(L"SlotDataOriginal\\.dat")))
				{
					if (!FoundSlotDataOriginal)
					{
						FoundSlotDataOriginal = true;
						SlotDataOriginal = entry.path();
					}
				}
				else if (std::regex_match(entry.path().filename().wstring(), std::wregex(L"SlotData_[0-2]\\.dat")))
				{
					SlotDataFiles.push_back(entry.path());
				}
			}
		}
	}
}

int main(void)
{
	BagsSaveImporter::BagsFiles::GetSaveFiles(L".");

	if (BagsSaveImporter::BagsFiles::SlotDataFiles.size() == 0 || BagsSaveImporter::BagsFiles::FoundSlotDataOriginal == false)
	{
		std::wcout << L"Could not find save files or SlotDataOriginal.dat" << std::endl;
		return 1;
	}

	std::fstream stream = std::fstream(fs::absolute(BagsSaveImporter::BagsFiles::SlotDataOriginal).wstring(), std::ios::binary || std::ios::in);
	char* header = BagsSaveImporter::BagsHeader::ReadHeaderBytes(stream);
	stream.close();

	for (const auto& e : BagsSaveImporter::BagsFiles::SlotDataFiles)
	{
		std::ofstream slotDataStream = std::ofstream(fs::absolute(e).wstring(), std::ios::binary | std::ios::in || std::ios::out);
		if (!BagsSaveImporter::BagsHeader::WriteHeaderToFile(slotDataStream, header))
		{
			std::wcout << L"Failed to write new header to file" << std::endl;
			slotDataStream.close();
			return 2;
		}
		slotDataStream.close();

		std::fstream slotDataVerifyStream = std::fstream(fs::absolute(e).wstring(), std::ios::binary || std::ios::in);
		if (!BagsSaveImporter::BagsHeader::VerifyHeaderIsSame(slotDataVerifyStream, header))
		{
			std::wcout << L"Failed to verify new header on file" << std::endl;
			slotDataStream.close();
			return 3;
		}
		slotDataVerifyStream.close();
	}

	DWORD len = GetEnvironmentVariable(L"localappdata", nullptr, 0);
	wchar_t* appData = new wchar_t[len];
	if (len - 1 != GetEnvironmentVariable(L"localappdata", appData, len))
	{
		std::wcout << L"Failed to get %localappdata%" << std::endl;
		return 254;
	}

	std::wstringstream wss;
	wss << appData << L"\\Packages\\39EA002F.NieRAutomataPC_n746a19ndrrjg\\SystemAppData\\wgs";
	std::wcout << wss.str() << std::endl;

	for (const auto& e : fs::directory_iterator(wss.str()))
	{
		if (e.path().filename() != L"t" && e.is_directory())
		{
			wss << L"\\" << e.path().filename().wstring();
			break;
		}
	}

	std::wcout << L"Done! I processed " << BagsSaveImporter::BagsFiles::SlotDataFiles.size() << L" files." << std::endl;
	std::wcout << L"Place files in one of the three folders below with the same name as the file inside of it." << std::endl;
	std::wcout << L"Path: " << wss.str() << std::endl;
	std::wcout << L"Save folders inside: " << std::endl;

	for (const auto& e : fs::directory_iterator(wss.str()))
	{
		if (e.is_directory())
		{
			std::wcout << e.path().filename().wstring() << std::endl;
		}
	}

	delete[] appData;
	delete[] header;


	std::wcout << L"\n\n\n\nPress any key to continue...";
	std::cin.get();

	return 0;
}