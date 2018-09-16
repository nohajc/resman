#pragma once

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

class StashCWD {
	llvm::SmallString<260> dir;

public:
	StashCWD() {
		llvm::sys::fs::current_path(dir);
	}

	StashCWD(const StashCWD&) = delete;
	StashCWD& operator=(const StashCWD&) = delete;

	~StashCWD() {
		llvm::sys::fs::set_current_path(dir.str());
	}
};

inline std::string removeFilename(llvm::StringRef file) {
	llvm::SmallString<260> fname{file};
	llvm::sys::path::remove_filename(fname);
	return fname.str();
}

inline std::string makeAbsolute(llvm::StringRef file) {
	llvm::SmallString<260> fname(file);
	llvm::sys::fs::make_absolute(fname);
	return fname.str();
}
