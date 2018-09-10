#pragma once

#include <string>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ToolOutputFile.h>

void addDataToModule(const std::vector<char>& data,
	const std::string& varBeginName, const std::string& varSizeName,
	llvm::Module& mod, llvm::LLVMContext& ctxt);

void generateObjectFile(llvm::Module& mod, llvm::ToolOutputFile& objFile, const std::string& mArch);
void generateObjectFile(llvm::Module& mod, const std::string& objFilename, const std::string& mArch);
