
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

#include "parser/parser.h"
#include "symboltable.h"

// TODO: need to use the new generic symbol table, and switch the function part of the symbol table to 
// use the map instead

namespace codegen
{
    class CodeGen
    {
    private:
        std::string mOutFile;
        std::shared_ptr<ast::Assembly> mTree;
        SymbolTable<std::string, llvm::AllocaInst *> mTable;
        std::map<std::string, llvm::Function *> mFunctions;

        llvm::LLVMContext mContext;
        llvm::Module *mModule;
        llvm::Function *mMain;
        llvm::IRBuilder<> mBuilder;
        llvm::legacy::FunctionPassManager *mFpm;

        void putFunc(std::string name, llvm::Function *func);
        llvm::Function *getFunc(std::string name);

        llvm::Type *stringToType(std::string str);
        std::vector<llvm::Type *> getFunctionArgumentTypes(std::shared_ptr<ast::Function> function);

        void generateAssembly(std::shared_ptr<ast::Assembly> assembly);
        llvm::Function *generateFunctionPrototype(std::shared_ptr<ast::Function> function);
        llvm::Value *generateFunction(std::shared_ptr<ast::Function> function);
        llvm::Value *generateExpression(std::shared_ptr<ast::Expression> expression);
        llvm::Value *generateBinaryExpression(std::shared_ptr<ast::BinaryExpressionNode> expression);
        llvm::Value *generateIntegerMath(std::string op, llvm::Value *lhs, llvm::Value *rhs);
        llvm::Value *generateFloatingPointMath(std::string op, llvm::Value *lhs, llvm::Value *rhs);
        llvm::Value *generateFunctionCall(std::shared_ptr<ast::FunctionCallNode> expression);
        llvm::Value *generateBlock(std::shared_ptr<ast::BlockNode> block, llvm::Function * llvmFunc);
    public:
        CodeGen(std::shared_ptr<ast::Assembly> tree, std::string outFile="");

        void generate();
        void runJit();
        void freeResources();
    };
}
