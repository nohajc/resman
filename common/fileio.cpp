#include "fileio.h"
#include <fstream>

std::vector<char> readFileIntoMemory(const std::string& ifname) {
	std::ifstream in(ifname, std::ios::binary);
	if (!in.is_open()) {
		throw std::runtime_error("Cannot open input file.");
	}

	in.seekg(0, std::ios::end);
	size_t fsize = in.tellg();

	std::vector<char> contents(fsize);
	in.seekg(0, std::ios::beg);
	in.read(contents.data(), fsize);

	return contents;
}
