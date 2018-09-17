#pragma once

#include <vector>
#include <string>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

llvm::Expected<std::vector<char>> readFileIntoMemory(const std::string& ifname, const std::vector<llvm::StringRef>& searchPath = {});