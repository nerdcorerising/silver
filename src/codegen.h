
#pragma once

#include <ostream>
#include <iostream>
#include <fstream>
#include <memory>
#include <map>

#pragma warning (push, 1)
#pragma warning(disable:4996)
#pragma warning(disable:4291)
#pragma warning(disable:4141)
#pragma warning(disable:4624)
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
//#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
// #include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/Support/ManagedStatic.h"
#pragma warning (pop)

#include "parser.h"

class SymbolTable
{
private:
    std::vector<std::map<std::string, llvm::AllocaInst *>> mStack;
    std::map<std::string, llvm::AllocaInst *> mCurrent;

    std::map<std::string, llvm::Function *> mFunctions;

public:
    SymbolTable();
    void putVar(std::string name, llvm::AllocaInst *inst);
    llvm::AllocaInst *getVar(std::string name);

    void putFunc(std::string name, llvm::Function *func);
    llvm::Function *getFunc(std::string name);

    void enterContext();
    void leaveContext();
};

class CodeGen
{
private:
    std::string mOutFile;
    std::shared_ptr<ast::Assembly> mTree;
    SymbolTable mTable;

    llvm::LLVMContext mContext;
    llvm::Module *mModule;
    llvm::Function *mMain;
    llvm::IRBuilder<> mBuilder;
    llvm::legacy::FunctionPassManager *mFpm;

    llvm::Type *stringToType(std::string str);
    std::vector<llvm::Type *> getFunctionArgumentTypes(std::shared_ptr<ast::Function> function);

    void GenerateAssembly(std::shared_ptr<ast::Assembly> assembly);
    llvm::Value *GenerateFunction(std::shared_ptr<ast::Function> function);
    llvm::Value *GenerateExpression(std::shared_ptr<ast::Expression> expression);
    llvm::Value *GenerateBinaryExpression(std::shared_ptr<ast::BinaryExpressionNode> expression);
    llvm::Value *GenerateIntegerMath(std::string op, llvm::Value *lhs, llvm::Value *rhs);
    llvm::Value *GenerateFloatingPointMath(std::string op, llvm::Value *lhs, llvm::Value *rhs);
    llvm::Value *GenerateFunctionCall(std::shared_ptr<ast::FunctionCallNode> expression);
    llvm::Value *GenerateBlock(std::shared_ptr<ast::Block> block, llvm::Function * llvmFunc);
public:
    CodeGen(std::shared_ptr<ast::Assembly> tree, std::string outFile);

    void Generate();
    void RunJit();
    void FreeResources();
};
