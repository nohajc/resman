#include "fileio.h"
#include "fsutil.h"
#include <fstream>
#include <utility>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;

Expected<std::vector<char>> readFileIntoMemory(const std::string& ifname, const std::vector<StringRef>& searchPath) {
	auto errorOrMemBuf = MemoryBuffer::getFile(ifname, -1, false); // is the file in the current directory?
	if (errorOrMemBuf) {
		auto& memBuf = *errorOrMemBuf;
		return std::vector<char>{ memBuf->getBufferStart(), memBuf->getBufferEnd() };
	}
	std::error_code lastError = errorOrMemBuf.getError();

	StashCWD restoreCwdOnScopeExit;

	for (StringRef dir : searchPath) { // if not, try search path
		//llvm::outs() << "Searching input in " << dir << "\n";
		sys::fs::set_current_path(dir);

		auto errorOrMemBuf = MemoryBuffer::getFile(ifname, -1, false);
		if (!errorOrMemBuf) {
			lastError = errorOrMemBuf.getError();
			continue;
		}
		auto& memBuf = *errorOrMemBuf;
		return std::vector<char>{ memBuf->getBufferStart(), memBuf->getBufferEnd() };
	}

	return errorCodeToError(lastError);
}
