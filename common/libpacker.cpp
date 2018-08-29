#include "libpacker.h"
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>

using namespace llvm;

void packIntoLib(const std::string& ifname, const std::string& ofname) {
	SmallString<512> dir(ifname);
	sys::path::remove_filename(dir);
	std::string idir = dir.str();

	SmallString<512> fname(ofname);
	sys::fs::make_absolute(fname);
	auto ofname_abs = fname.str();

	sys::fs::set_current_path(idir);

	auto baseifname = sys::path::filename(ifname);
	auto memberOrErr = NewArchiveMember::getFile(baseifname, true);
	if (!memberOrErr) {
		throw std::runtime_error("Could not open generated objectfile.");
	}

	NewArchiveMember member[1] = { std::move(*memberOrErr) };

	// only takes absolute archive path!
	Error err = writeArchive(ofname_abs, member, true, object::Archive::K_GNU, true, false);
	if (err) {
		throw std::runtime_error("Error writing static library file.");
	}
}
