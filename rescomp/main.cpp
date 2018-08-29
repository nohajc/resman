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

#include <iostream>
#include <utility>

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

		llvm::outs() << decl->getName() << '\n';

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
	std::unique_ptr<llvm::Module> mod;

	void constructStorageGlobals(uint64_t resourceID, const std::string& resourcePath) {
		std::string code;
		llvm::raw_string_ostream codestream(code);
		codestream << "#include <cstddef>\n#include <cstdint>\n";
		codestream << "namespace resman {\n";
		codestream << R"__(
template <size_t N>
struct Resource {
private:
	static const char storage_begin[];
	static const uint32_t storage_size;
};
		)__";
		codestream << "template <> const char Resource<" << resourceID << ">::storage_begin[] = \"dummy\";\n";
		codestream << "template <> const uint32_t Resource<" << resourceID << ">::storage_size{6};\n";
		codestream << "}";

		auto ast = buildASTFromCode(codestream.str());
		if (ast) {
			MangledStorageGlobals glob;
			MangleStorageNamesASTVisitor resCompVisitor(resourceID, resourcePath, glob);
			resCompVisitor.TraverseDecl(ast->getASTContext().getTranslationUnitDecl());

			llvm::outs() << "begin: " << glob.storageBegin << ", size: " << glob.storageSize << '\n';
			try {
				auto data = readFileIntoMemory(resourcePath);
				addDataToModule(data, glob.storageBegin, glob.storageSize, getModule(), llvmCtxt);
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
	llvm::Module& getModule() {
		if (!mod) {
			mod.reset(new llvm::Module("resource", llvmCtxt));
		}
		return *mod;
	}

	CompileResourcesASTVisitor(ASTContext& ctxt)
		: astCtxt(ctxt) {}

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

		if (!isa<ImplicitCastExpr>(arg)) return true;
		auto strExpr = dyn_cast<ImplicitCastExpr>(arg)->getSubExpr();

		if (!isa<StringLiteral>(strExpr)) return true;
		auto pathValue = dyn_cast<StringLiteral>(strExpr);
		std::string resourcePath = pathValue->getString();

		llvm::outs() << "Resource: ID = " << resourceID << ", PATH = \"" << resourcePath << "\"\n";
		constructStorageGlobals(resourceID, resourcePath);

		return true;
	}
};

static std::pair<std::string, std::string> getOutputPaths(const std::string& inputPath) {
	SmallString<512> fnameBuf(inputPath);

	llvm::sys::path::replace_extension(fnameBuf, objext);
	std::string objPath = fnameBuf.str();

	llvm::sys::path::replace_extension(fnameBuf, libext);
	std::string libPath = fnameBuf.str();

	return { objPath, libPath };
}

class ResCompASTConsumer : public ASTConsumer {
	std::string filePath;

public:
	ResCompASTConsumer(StringRef file) : filePath(file) {}

	void HandleTranslationUnit(clang::ASTContext& ctxt) override {
		CompileResourcesASTVisitor visitor(ctxt);
		visitor.TraverseDecl(ctxt.getTranslationUnitDecl());

		std::string objPath;
		std::string libPath;
		std::tie(objPath, libPath) = getOutputPaths(filePath);

		//llvm::outs() << "Input: " << filePath << '\n';
		//llvm::outs() << "Output: " << libPath << '\n';

		auto& mod = visitor.getModule();
		llvm::verifyModule(mod);

		try {	
			generateObjectFile(mod, objPath, "x86-64");
			packIntoLib(objPath, libPath);
		}
		catch (const std::exception& ex) {
			llvm::errs() << ex.what() << '\n';
		}
	}
};

class ResCompFrontendAction : public ASTFrontendAction {
	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
		return std::make_unique<ResCompASTConsumer>(file);
	}
};

int main(int argc, const char *argv[]) {
	CommonOptionsParser op(argc, argv, ToolingResCompCategory);
	auto sourcePathList = op.getSourcePathList();

	std::vector<std::pair<std::string, std::string>> virtualCpps;
	for (auto& srcPath : sourcePathList) {
		StringRef srcPathRef(srcPath);
		SmallString<512> fname{srcPath};

		if (srcPathRef.endswith_lower(".h") || srcPathRef.endswith_lower(".hpp")) {
			llvm::sys::path::replace_extension(fname, ".cpp");
			virtualCpps.push_back({ getAbsolutePath(fname.str()), "#include \"" + srcPath + "\"" });
			srcPath = fname.str();
		}
	}

	ClangTool tool(op.getCompilations(), sourcePathList);

	for (const auto& virt : virtualCpps) {
		tool.mapVirtualFile(virt.first, virt.second);
		//llvm::outs() << "Mapped virtual file " + virt + ".cpp\n";
	}

	return tool.run(newFrontendActionFactory<ResCompFrontendAction>().get());
}
