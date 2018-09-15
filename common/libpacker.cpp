#include "libpacker.h"
#include "fsutil.h"
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

void packIntoLib(const std::string& ifname, const std::string& ofname) {
	StashCWD restoreCwdOnScopeExit;
	SmallString<260> dir(ifname);
	sys::path::remove_filename(dir);
	std::string idir = dir.str();

	SmallString<260> fname(ofname);
	sys::fs::make_absolute(fname);
	auto ofname_abs = fname.str();

	sys::fs::set_current_path(idir);

	auto baseifname = sys::path::filename(ifname);
	auto memberOrErr = NewArchiveMember::getFile(baseifname, true);
	if (!memberOrErr) {
		// TODO: custom exception that can capture llvm::Error and log it in the catch clause
		// logAllUnhandledErrors(std::move(memberOrErr.takeError()), llvm::errs(), "File IO error: ");
		throw std::runtime_error("Could not open generated objectfile.");
	}

	NewArchiveMember member[1] = { std::move(*memberOrErr) };

	// only takes absolute archive path!
	Error err = writeArchive(ofname_abs, member, true, object::Archive::K_GNU, true, false);
	if (err) {
		throw std::runtime_error("Could not write static library file.");
	}
}
