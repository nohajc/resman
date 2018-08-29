#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/Path.h>

#include "../common/fileio.h"
#include "../common/objcompiler.h"
#include "../common/libpacker.h"

using namespace llvm;

struct Options {
	std::string ifname, objname, libname, headername, varname, arch;
};

Options parseOptions(int argc, char* argv[]) {
	using namespace std::string_literals;
	if (argc != 4) {
		throw std::runtime_error("Wrong number of arguments given.");
	}

	Options res;
	res.ifname = argv[1];
	SmallString<512> fname(res.ifname);

	res.arch = argv[3];
	auto archext = res.arch == "x86-64" ? "x64" : res.arch;

#ifdef _WIN32
	constexpr const char objext[] = ".obj";
	constexpr const char libext[] = ".lib";
#else
	constexpr const char objext[] = ".o";
	constexpr const char libext[] = ".a";
#endif

	fname += "_" + archext + objext;
	res.objname = fname.str();

	sys::path::replace_extension(fname, libext);
	res.libname = fname.str();

	res.headername = res.ifname + ".h";

	res.varname = argv[2];

	return res;
}

void generateHeaderFile(const std::string& varname, const std::string& headername) {
	std::ofstream out(headername);

	out << R"__(#pragma once

#include <cstdint>

extern "C" {
	extern const char )__" << varname << R"__([];
	extern const uint32_t )__" << varname << R"__(_size;
}
)__";
}

int main(int argc, char *argv[]) {
	try {
		auto args = parseOptions(argc, argv);

		LLVMContext ctxt;
		Module mod("resource", ctxt);

		auto data = readFileIntoMemory(args.ifname);
		addDataToModule(data, args.varname, args.varname + "_size", mod, ctxt);
		verifyModule(mod);

		generateObjectFile(mod, args.objname, args.arch);
		generateHeaderFile(args.varname, args.headername);
		packIntoLib(args.objname, args.libname);
	}
	catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << '\n';
		std::cout << "Usage: " << argv[0] << " INPUT_FILE VAR_NAME ARCH\n"; // ARCH can be x86 or x86-64
	}

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
