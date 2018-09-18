#include "libpacker.h"
#include "fsutil.h"
#include "exceptions.h"
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

void packIntoLib(const std::string& ifname, const std::string& ofname) {
	StashCWD restoreCwdOnScopeExit;
	sys::fs::set_current_path(removeFilename(ifname));

	auto baseifname = sys::path::filename(ifname);
	auto memberOrErr = NewArchiveMember::getFile(baseifname, true);
	if (auto err = memberOrErr.takeError()) {
		throw llvm_error(std::move(err), "Could not open generated objectfile: ");
	}

	NewArchiveMember member[1] = { std::move(*memberOrErr) };

	// only takes absolute archive path!
	Error err = writeArchive(makeAbsolute(ofname), member, true, object::Archive::K_GNU, true, false);
	if (err) {
		throw llvm_error(std::move(err), "Could not write static library file: ");
	}
}
