#include "objcompiler.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/DiagnosticPrinter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/ADT/Triple.h>
#include <llvm/CodeGen/CommandFlags.def>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/CodeGen/MachineModuleInfo.h>

using namespace llvm;

static std::vector<Constant*> toConstantArray(const std::vector<char>& data, Type* elemType) {
	std::vector<Constant*> res(data.size());
	std::transform(data.cbegin(), data.cend(), res.begin(), [=](uint8_t val) {
		return ConstantInt::get(elemType, val);
	});

	return res;
}

void addDataToModule(const std::vector<char>& data,
	const std::string& varBeginName, const std::string& varSizeName,
	Module& mod, LLVMContext& ctxt) {

	auto dataSize = data.size();

	IntegerType* int32 = IntegerType::get(ctxt, 32);
	IntegerType* byte = IntegerType::get(ctxt, 8);
	ArrayType* byteArrayType = ArrayType::get(byte, dataSize);

	Constant* initializer = ConstantArray::get(byteArrayType, toConstantArray(data, byte));

	new GlobalVariable(mod, byteArrayType, true, GlobalValue::ExternalLinkage, initializer, varBeginName);
	new GlobalVariable(mod, int32, true, GlobalValue::ExternalLinkage, ConstantInt::get(int32, dataSize), varSizeName);
}



static void InlineAsmDiagHandler(const SMDiagnostic &SMD, void *Context,
	unsigned LocCookie) {
	bool *HasError = static_cast<bool *>(Context);
	if (SMD.getKind() == SourceMgr::DK_Error)
		*HasError = true;

	SMD.print(nullptr, errs());

	// For testing purposes, we print the LocCookie here.
	if (LocCookie)
		errs() << "note: !srcloc = " << LocCookie << "\n";
}

struct LLCDiagnosticHandler : public DiagnosticHandler {
	bool *HasError;
	LLCDiagnosticHandler(bool *HasErrorPtr) : HasError(HasErrorPtr) {}
	bool handleDiagnostics(const DiagnosticInfo &DI) override {
		if (DI.getSeverity() == DS_Error)
			*HasError = true;

		if (auto *Remark = dyn_cast<DiagnosticInfoOptimizationBase>(&DI))
			if (!Remark->isEnabled())
				return true;

		DiagnosticPrinterRawOStream DP(errs());
		errs() << LLVMContext::getDiagnosticMessagePrefix(DI.getSeverity()) << ": ";
		DI.print(DP);
		errs() << "\n";
		return true;
	}
};


void generateObjectFile(Module& mod, const std::string& ofname, const std::string& arch) {
	FileType = TargetMachine::CGFT_ObjectFile;

	LLVMInitializeX86TargetInfo();
	LLVMInitializeX86Target();
	LLVMInitializeX86TargetMC();
	LLVMInitializeX86AsmPrinter();
	LLVMInitializeX86AsmParser();

	PassRegistry *Registry = PassRegistry::getPassRegistry();
	initializeCore(*Registry);
	initializeCodeGen(*Registry);
	initializeLoopStrengthReducePass(*Registry);
	initializeLowerIntrinsicsPass(*Registry);
	initializeEntryExitInstrumenterPass(*Registry);
	initializePostInlineEntryExitInstrumenterPass(*Registry);
	initializeUnreachableBlockElimLegacyPassPass(*Registry);
	initializeConstantHoistingLegacyPassPass(*Registry);
	initializeScalarOpts(*Registry);
	initializeVectorization(*Registry);
	initializeScalarizeMaskedMemIntrinPass(*Registry);
	initializeExpandReductionsPass(*Registry);

	initializeScavengerTestPass(*Registry);

	LLVMContext& ctxt = mod.getContext();
	llvm_shutdown_obj Y;

	ctxt.setDiscardValueNames(true);

	bool hasError = false;
	ctxt.setDiagnosticHandler(
		llvm::make_unique<LLCDiagnosticHandler>(&hasError));
	ctxt.setInlineAsmDiagnosticHandler(InlineAsmDiagHandler, &hasError);

	Triple theTriple(sys::getDefaultTargetTriple());

	std::string mArch = arch;

	std::string error;
	const Target *theTarget = TargetRegistry::lookupTarget(mArch, theTriple, error);

	if (!theTarget) {
		throw std::runtime_error(error);
	}

	std::string CPUStr = getCPUStr(), FeaturesStr = getFeaturesStr();
	CodeGenOpt::Level OLvl = CodeGenOpt::Default;

	TargetOptions options = InitTargetOptionsFromCodeGenFlags();
	options.DisableIntegratedAS = false;
	options.MCOptions.ShowMCEncoding = false;
	options.MCOptions.MCUseDwarfDirectory = false;
	options.MCOptions.AsmVerbose = false;
	options.MCOptions.PreserveAsmComments = false;
	//options.MCOptions.IASSearchPaths = IncludeDirs;
	//options.MCOptions.SplitDwarfFile = SplitDwarfFile;

	std::unique_ptr<TargetMachine> target(theTarget->createTargetMachine(
		theTriple.getTriple(), CPUStr, FeaturesStr, options, getRelocModel(),
		getCodeModel(), OLvl));

	std::error_code errc;
	ToolOutputFile out(ofname, errc, sys::fs::F_None);

	if (errc) {
		throw std::runtime_error("Cannot open output file. ");
	}

	legacy::PassManager PM;
	TargetLibraryInfoImpl TLII(Triple(mod.getTargetTriple()));

	PM.add(new TargetLibraryInfoWrapperPass(TLII));
	mod.setDataLayout(target->createDataLayout());
	setFunctionAttributes(CPUStr, FeaturesStr, mod);

	{
		raw_pwrite_stream *OS = &out.os();
		SmallVector<char, 0> Buffer;
		std::unique_ptr<raw_svector_ostream> BOS;

		if (!out.os().supportsSeeking()) {
			BOS = make_unique<raw_svector_ostream>(Buffer);
			OS = BOS.get();
		}

		LLVMTargetMachine &LLVMTM = static_cast<LLVMTargetMachine&>(*target);
		MachineModuleInfo *MMI = new MachineModuleInfo(&LLVMTM);

		if (target->addPassesToEmitFile(PM, *OS, FileType, false, MMI)) {
			throw std::runtime_error("Incompatible target and file type.");
		}

		PM.run(mod);

		auto pHasError =
			((const LLCDiagnosticHandler *)(ctxt.getDiagHandlerPtr()))->HasError;
		if (*pHasError) {
			throw std::runtime_error("Fatal error.");
		}

		if (BOS) {
			out.os() << Buffer;
		}

		out.keep();
	}
}
