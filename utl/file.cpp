#include "file.h"
#include <fstream>
#include <filesystem>

namespace utl {

std::vector<uint8_t> ReadFile(std::string const &path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		LOG("Failed to load file %s", path.c_str());
		return std::vector<uint8_t>();
	}

	size_t size = static_cast<size_t>(file.tellg());
	std::vector<uint8_t> contents(size);

	file.seekg(0);
	file.read(reinterpret_cast<char *>(contents.data()), contents.size());

	file.close();

	return contents;
}

std::string GetPathDir(std::string path)
{
	return std::filesystem::path(path).parent_path().string();
}

std::string GetPathFilenameExt(std::string path)
{
	return std::filesystem::path(path).filename().string();
}

std::string GetPathExt(std::string path)
{
	return std::filesystem::path(path).extension().string();
}

} // utl