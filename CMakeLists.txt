cmake_minimum_required(VERSION 3.1)

project(silver)

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE $ENV{__CMakeBuildType})
set(CMAKE_INSTALL_PREFIX $ENV{__CMakeBinDir})

if (CMAKE_PLATFORM_UNIX)
    include_directories(/usr/local/Cellar/llvm@6/6.0.1_3/include/)
    link_directories(/usr/local/Cellar/llvm@6/6.0.1_3/lib/)
    set(ADDITIONAL_DEPS z curses m ffi)
endif (CMAKE_PLATFORM_UNIX)

if (WIN32)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include_directories("C:/personal/llvm_bin/LLVM_Debug_x64/include")
    link_directories("C:/personal/llvm_bin/LLVM_Debug_x64/lib")
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
    include_directories("C:/personal/llvm_bin/LLVM_Release_x64/include")
    link_directories("C:/personal/llvm_bin/LLVM_Release_x64/lib")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")
endif (WIN32)

set(DEPENDENCIES LLVMLTO LLVMPasses LLVMObjCARCOpts LLVMMIRParser LLVMSymbolize LLVMDebugInfoPDB LLVMDebugInfoDWARF LLVMCoverage LLVMTableGen LLVMDlltoolDriver LLVMOrcJIT LLVMXCoreDisassembler LLVMXCoreCodeGen LLVMXCoreDesc LLVMXCoreInfo LLVMXCoreAsmPrinter LLVMSystemZDisassembler LLVMSystemZCodeGen LLVMSystemZAsmParser LLVMSystemZDesc LLVMSystemZInfo LLVMSystemZAsmPrinter LLVMSparcDisassembler LLVMSparcCodeGen LLVMSparcAsmParser LLVMSparcDesc LLVMSparcInfo LLVMSparcAsmPrinter LLVMPowerPCDisassembler LLVMPowerPCCodeGen LLVMPowerPCAsmParser LLVMPowerPCDesc LLVMPowerPCInfo LLVMPowerPCAsmPrinter LLVMNVPTXCodeGen LLVMNVPTXDesc LLVMNVPTXInfo LLVMNVPTXAsmPrinter LLVMMSP430CodeGen LLVMMSP430Desc LLVMMSP430Info LLVMMSP430AsmPrinter LLVMMipsDisassembler LLVMMipsCodeGen LLVMMipsAsmParser LLVMMipsDesc LLVMMipsInfo LLVMMipsAsmPrinter LLVMLanaiDisassembler LLVMLanaiCodeGen LLVMLanaiAsmParser LLVMLanaiDesc LLVMLanaiAsmPrinter LLVMLanaiInfo LLVMHexagonDisassembler LLVMHexagonCodeGen LLVMHexagonAsmParser LLVMHexagonDesc LLVMHexagonInfo LLVMBPFDisassembler LLVMBPFCodeGen LLVMBPFAsmParser LLVMBPFDesc LLVMBPFInfo LLVMBPFAsmPrinter LLVMARMDisassembler LLVMARMCodeGen LLVMARMAsmParser LLVMARMDesc LLVMARMInfo LLVMARMAsmPrinter LLVMARMUtils LLVMAMDGPUDisassembler LLVMAMDGPUCodeGen LLVMAMDGPUAsmParser LLVMAMDGPUDesc LLVMAMDGPUInfo LLVMAMDGPUAsmPrinter LLVMAMDGPUUtils LLVMAArch64Disassembler LLVMAArch64CodeGen LLVMAArch64AsmParser LLVMAArch64Desc LLVMAArch64Info LLVMAArch64AsmPrinter LLVMAArch64Utils LLVMObjectYAML LLVMLibDriver LLVMOption LLVMWindowsManifest LLVMFuzzMutate LLVMX86Disassembler LLVMX86AsmParser LLVMX86CodeGen LLVMGlobalISel LLVMSelectionDAG LLVMAsmPrinter LLVMDebugInfoCodeView LLVMDebugInfoMSF LLVMX86Desc LLVMMCDisassembler LLVMX86Info LLVMX86AsmPrinter LLVMX86Utils LLVMMCJIT LLVMLineEditor LLVMInterpreter LLVMExecutionEngine LLVMRuntimeDyld LLVMCodeGen LLVMTarget LLVMCoroutines LLVMipo LLVMInstrumentation LLVMVectorize LLVMScalarOpts LLVMLinker LLVMIRReader LLVMAsmParser LLVMInstCombine LLVMTransformUtils LLVMBitWriter LLVMAnalysis LLVMProfileData LLVMObject LLVMMCParser LLVMMC LLVMBitReader LLVMCore LLVMBinaryFormat LLVMSupport LLVMDemangle ${ADDITIONAL_DEPS})

add_subdirectory("src")

