#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Mangle.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

#include <llvm/IR/Verifier.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>

#include <iostream>
#include <utility>
#include <string>

#include "../common/fileio.h"
#include "../common/objcompiler.h"
#include "../common/libpacker.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;


#ifdef _WIN32
constexpr const char objext[] = ".obj";
constexpr const char libext[] = ".lib";
#else
constexpr const char objext[] = ".o";
constexpr const char libext[] = ".a";
#endif

static llvm::cl::OptionCategory ToolingResCompCategory("Resource Compiler");

static llvm::cl::opt<std::string> OutputFilePath("o",
	llvm::cl::Required,
	llvm::cl::desc("Specify output file"),
	llvm::cl::value_desc("path"),
	llvm::cl::cat(ToolingResCompCategory));

static llvm::cl::opt<std::string> MArch("march",
	llvm::cl::desc("Architecture to generate code for (native by default)"),
	llvm::cl::cat(ToolingResCompCategory));

static llvm::cl::list<std::string> ResSearchPath("R",
	llvm::cl::desc("Resource search path (can be used more than once for multiple paths)"),
	llvm::cl::value_desc("directory"),
	llvm::cl::cat(ToolingResCompCategory));

struct MangledStorageGlobals {
	std::string storageBegin;
	std::string storageSize;
};

class MangleStorageNamesASTVisitor : public RecursiveASTVisitor<MangleStorageNamesASTVisitor> {
	uint64_t resourceID;
	std::string resourcePath;
	MangledStorageGlobals& mangledGlob;
public:
	MangleStorageNamesASTVisitor(uint64_t resID, const std::string& resPath, MangledStorageGlobals& glob)
		: resourceID(resID), resourcePath(resPath), mangledGlob(glob) {}

	bool VisitVarDecl(VarDecl* decl) {
		if (!decl->isStaticDataMember()
			|| !decl->getMemberSpecializationInfo()
			|| !decl->isDefinedOutsideFunctionOrMethod()
			|| !decl->isThisDeclarationADefinition()
			|| !StringRef(decl->getQualifiedNameAsString()).startswith("resman::Resource")) {
			return true;
		}

		std::string mangledVarName;
		auto mangleCtxt = decl->getASTContext().createMangleContext();

		if (!mangleCtxt->shouldMangleDeclName(decl)) {
			return true;
		}

		{
			llvm::raw_string_ostream strout(mangledVarName);
			mangleCtxt->mangleName(decl, strout);
			strout.str();
		}

		if (mangledVarName.find("begin") != std::string::npos) {
			mangledGlob.storageBegin = mangledVarName;
		}
		else if (mangledVarName.find("size") != std::string::npos) {
			mangledGlob.storageSize = mangledVarName;
		}

		delete mangleCtxt;
		return true;
	}
};

static llvm::LLVMContext llvmCtxt;

class CompileResourcesASTVisitor : public RecursiveASTVisitor<CompileResourcesASTVisitor> {
	ASTContext& astCtxt;
	ArrayRef<StringRef> searchPath;
	llvm::Module& outputModule;

	void constructStorageGlobals(uint64_t resourceID, const std::string& resourcePath) {
		std::string code;
		llvm::raw_string_ostream codestream(code);
		codestream << "namespace resman {\n";
		codestream << R"__(
template <unsigned N>
struct Resource {
private:
	static const char storage_begin[];
	static const unsigned storage_size;
};
		)__";
		codestream << "template <> const char Resource<" << resourceID << ">::storage_begin[] = \"dummy\";\n";
		codestream << "template <> const unsigned Resource<" << resourceID << ">::storage_size{6};\n";
		codestream << "}";

		auto ast = buildASTFromCode(codestream.str());
		if (ast) {
			MangledStorageGlobals glob;
			MangleStorageNamesASTVisitor resCompVisitor(resourceID, resourcePath, glob);
			resCompVisitor.TraverseDecl(ast->getASTContext().getTranslationUnitDecl());

			//llvm::outs() << "begin: " << glob.storageBegin << ", size: " << glob.storageSize << '\n';
			try {
				auto data = readFileIntoMemory(resourcePath, searchPath);
				addDataToModule(data, glob.storageBegin, glob.storageSize, outputModule, llvmCtxt);
			}
			catch (const std::exception& ex) {
				llvm::errs() << ex.what() << '\n';
			}
		}
	}

	uint64_t evalTmplArgumentExpr(Expr* tmplArgExpr) {
		llvm::APSInt res;
		tmplArgExpr->EvaluateAsInt(res, astCtxt);
		return res.getZExtValue();
	}

	std::pair<uint64_t, bool> getTmplArgVal(const TemplateArgument& tmplArg) {
		switch (tmplArg.getKind()) {
		case TemplateArgument::ArgKind::Integral:
			return { tmplArg.getAsIntegral().getZExtValue(), true };
		case TemplateArgument::ArgKind::Expression:
			return { evalTmplArgumentExpr(tmplArg.getAsExpr()), true };
		default:
			return { 0, false };
		}
	}

public:
	CompileResourcesASTVisitor(ASTContext& ctxt, ArrayRef<StringRef> paths, llvm::Module& mod)
		: astCtxt(ctxt), searchPath(paths), outputModule(mod) {}

	bool VisitVarDecl(VarDecl* decl) { // TODO: add error handling to avoid duplicate resource IDs
		if (!decl->isConstexpr()
			|| !decl->isDefinedOutsideFunctionOrMethod()
			|| !decl->isThisDeclarationADefinition()) {
			return true;
		}

		auto specType = decl->getType()->getAs<TemplateSpecializationType>();
		if (!specType) {
			return true;
		}

		auto tmplName = specType->getTemplateName();
		if (tmplName.getKind() != TemplateName::NameKind::Template) {
			return true;
		}

		auto tmplDecl = tmplName.getAsTemplateDecl();
		if (tmplDecl->getQualifiedNameAsString() != "resman::Resource") {
			return true;
		}

		// Retrieve resource ID
		const auto& tmplArg = specType->getArg(0);
		uint64_t resourceID; bool isValid;
		std::tie(resourceID, isValid) = getTmplArgVal(tmplArg);
		if (!isValid) return true;

		// Retrieve resource path
		auto initializer = decl->getInit();
		if (!isa<CXXConstructExpr>(initializer)) {
			return true;
		}
		auto constructor = dyn_cast<CXXConstructExpr>(initializer);
		auto arg = *constructor->arg_begin();

		if (!isa<StringLiteral>(arg)) return true;
		auto pathValue = dyn_cast<StringLiteral>(arg);
		std::string resourcePath = pathValue->getString();

		//llvm::outs() << "Resource: ID = " << resourceID << ", PATH = \"" << resourcePath << "\"\n";
		constructStorageGlobals(resourceID, resourcePath);

		return true;
	}
};

class ResCompASTConsumer : public ASTConsumer {
	ArrayRef<StringRef> searchPath;
	llvm::Module& outputModule;

public:
	ResCompASTConsumer(ArrayRef<StringRef> paths, llvm::Module& mod)
		: searchPath(paths), outputModule(mod) {}

	void HandleTranslationUnit(clang::ASTContext& ctxt) override {
		CompileResourcesASTVisitor visitor(ctxt, searchPath, outputModule);
		visitor.TraverseDecl(ctxt.getTranslationUnitDecl());
	}
};

class ResCompFrontendAction : public ASTFrontendAction {
	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
		SmallString<260> fname{file};
		llvm::sys::path::remove_filename(fname);
		inputDirectory = fname.str();

		resSearchPath.back() = inputDirectory;
		return std::make_unique<ResCompASTConsumer>(resSearchPath, outputModule);
	}

	std::vector<StringRef> resSearchPath;
	std::string inputDirectory;
	llvm::Module& outputModule;
public:
	ResCompFrontendAction(ArrayRef<std::string> searchPath,	llvm::Module& mod)
		: resSearchPath(searchPath.begin(), searchPath.end()), outputModule(mod) {
		resSearchPath.push_back(""); // create extra slot
	}
};

template <typename F>
std::unique_ptr<FrontendActionFactory> newFrontendActionFactoryFromLambda(F construct) {
	class LambdaFrontendActionFactory : public FrontendActionFactory {
		F constructFunc;
	public:
		LambdaFrontendActionFactory(F construct) : constructFunc(construct) {}
		clang::FrontendAction *create() override { return constructFunc(); }
	};

	return std::unique_ptr<FrontendActionFactory>{ new LambdaFrontendActionFactory(construct) };
}

class ObjOrLibPath {
	enum class Type {
		Obj, Lib
	} type;

	std::string objPath;
	std::string libPath;

	static Type getOutputType(StringRef path) {
		if (path.endswith(objext)) {
			return Type::Obj;
		}
		if (path.endswith(libext)) {
			return Type::Lib;
		}
		throw std::runtime_error("Output must be an object file or a static library.");
	}

public:
	ObjOrLibPath(StringRef path)
		: type(getOutputType(path))
		, objPath(type == Type::Obj ? path : "")
		, libPath(type == Type::Lib ? path : "") {
		if (type == Type::Lib) {
			using namespace std::string_literals;

			llvm::SmallString<260> filePath{ libPath };
			llvm::sys::path::replace_extension(filePath, "");
			filePath += "-%%%%%%%"s + objext;
			objPath = filePath.str();
		}
		// if type == Type::Obj, we won't need the libPath
	}

	bool isLib() const {
		return type == Type::Lib;
	}

	const std::string& obj() const {
		return objPath;
	}

	const std::string& lib() const {
		return libPath;
	}
};

class OpenOutputObjFile {
protected:
	SmallString<260> actualPath;
	int fd;

public:
	OpenOutputObjFile(const ObjOrLibPath& output) {
		std::error_code errc;

		// object file should have a unique name in case we're generating lib
		// to make sure we don't overwrite any existing file
		if (output.isLib()) {
			errc = llvm::sys::fs::createUniqueFile(output.obj(), fd, actualPath, llvm::sys::fs::all_write);
		}
		else {
			errc = llvm::sys::fs::openFileForWrite(output.obj(), fd, llvm::sys::fs::F_None);
			actualPath = output.obj();
		}

		if (errc) {
			throw std::runtime_error("Cannot open output file.");
		}
	}

	StringRef path() {
		return actualPath.str();
	}
};

class OutputObjFile : public OpenOutputObjFile, public llvm::ToolOutputFile {
public:
	OutputObjFile(const ObjOrLibPath& output)
		: OpenOutputObjFile(output), llvm::ToolOutputFile(actualPath, fd) {
		llvm::outs() << "Opened output file " << actualPath << '\n';
	}
};

int main(int argc, const char *argv[]) { // TODO: simulate `--` option to make sure we won't get any compiler db warnings
	CommonOptionsParser op(argc, argv, ToolingResCompCategory);
	auto sourcePathList = op.getSourcePathList();

	std::vector<std::pair<std::string, std::string>> virtualCpps;
	for (auto& srcPath : sourcePathList) {
		StringRef srcPathRef(srcPath);
		SmallString<260> fname{srcPath};

		if (srcPathRef.endswith_lower(".h") || srcPathRef.endswith_lower(".hpp")) {
			// TODO: do our own existence check for header files before trying to parse the virtual cpp
			llvm::sys::path::replace_extension(fname, ".cpp");
			std::string hdrName = llvm::sys::path::filename(srcPath);
			virtualCpps.push_back({ getAbsolutePath(fname.str()), "#include \"" + hdrName + "\"" });
			srcPath = fname.str();
		}
	}

	ClangTool tool(op.getCompilations(), sourcePathList);

	for (const auto& virt : virtualCpps) {
		tool.mapVirtualFile(virt.first, virt.second);
		//llvm::outs() << "Mapped virtual file " + virt + ".cpp\n";
	}

	auto mod = std::make_unique<llvm::Module>("resource", llvmCtxt);

	int returnCode = tool.run(newFrontendActionFactoryFromLambda([&] {
		return new ResCompFrontendAction(ResSearchPath, *mod);
	}).get());

	llvm::verifyModule(*mod);

	if (returnCode) return returnCode;

	try {
		ObjOrLibPath output{OutputFilePath};
		// objFile will have a randomized name in case we're generating static lib
		OutputObjFile objFile{output};

		generateObjectFile(*mod, objFile, MArch);
		objFile.os().flush();

		if (output.isLib()) {
			packIntoLib(objFile.path(), output.lib());
			// If a client specifies he only wants the static lib,
			// not calling `keep` will cause the object file to be deleted.
		}
		else { // On the other hand, if object file was specified, we do want to keep it.
			objFile.keep();
		}
	}
	catch (const std::exception& ex) {
		llvm::errs() << ex.what() << '\n';
		return 1;
	}

	return 0;
}
