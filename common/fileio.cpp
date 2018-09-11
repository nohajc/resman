#include "fileio.h"
#include "fsutil.h"
#include <fstream>
#include <utility>
#include <llvm/Support/FileSystem.h>
#include <llvm/ADT/ScopeExit.h>

using namespace llvm;

static bool readFileInto(const std::string& ifname, std::vector<char>& contents) {
	std::ifstream in(ifname, std::ios::binary);
	if (!in.is_open()) {
		return false;
	}

	in.seekg(0, std::ios::end);
	size_t fsize = in.tellg();

	contents.resize(fsize);
	in.seekg(0, std::ios::beg);
	in.read(contents.data(), fsize);

	return true;
}

std::vector<char> readFileIntoMemory(const std::string& ifname, const std::vector<StringRef>& searchPath) {
	std::vector<char> contents;

	if (readFileInto(ifname, contents)) { // is the file in the current directory?
		return contents;
	}

	StashCWD restoreCwdOnScopeExit;

	for (StringRef dir : searchPath) { // if not, try search path
		//llvm::outs() << "Searching input in " << dir << "\n";
		sys::fs::set_current_path(dir);

		if (!readFileInto(ifname, contents)) {
			continue;
		}
		return contents;
	}
	throw std::runtime_error("Cannot find input file " + ifname + '.');
}
