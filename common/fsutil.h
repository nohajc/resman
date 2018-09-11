#pragma once

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>

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
