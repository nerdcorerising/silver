
#pragma once

#include <ostream>
#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <set>

#pragma warning (push, 1)
#pragma warning(disable:4996)   // deprecated functions
#pragma warning(disable:4291)   // no matching operator delete
#pragma warning(disable:4141)   // used more than once
#pragma warning(disable:4624)   // destructor implicitly defined as deleted
#pragma warning(disable:4244)   // conversion possible loss of data
#pragma warning(disable:4267)   // conversion from size_t
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4702)   // unreachable code
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/Verifier.h"
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
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
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
        SymbolTable<std::string, std::string> mVariableTypes;  // Track type names for variables
        std::map<std::string, llvm::Function *> mFunctions;
        std::map<std::string, llvm::StructType *> mStructTypes;
        std::map<std::string, std::shared_ptr<ast::ClassDeclaration>> mClasses;
        std::set<std::string> mLocalFunctions;  // Mangled names of local functions

        // Stack of ref-counted variables per scope (for generating release calls)
        std::vector<std::vector<std::string>> mRefCountedVarsStack;

        // Current namespace path for resolving local function calls
        std::string mCurrentNamespace;

        // Current class name and 'this' pointer for method generation
        std::string mCurrentClass;
        llvm::Value* mThisPtr;

        bool mOptimize;
        llvm::LLVMContext mContext;
        llvm::Module *mModule;
        llvm::Function *mMain;
        llvm::IRBuilder<> mBuilder;
        llvm::legacy::FunctionPassManager *mFpm;

        void addSystemCalls();

        void reportFatalError(std::string message);
        void reportFatalError(std::string message, std::shared_ptr<ast::Expression> expr);

        void putFunc(std::string name, llvm::Function *func);
        llvm::Function *getFunc(std::string name);

        llvm::Type *stringToType(std::string str);
        std::vector<llvm::Type *> getFunctionArgumentTypes(std::shared_ptr<ast::Function> function);

        void generateClassTypes(std::shared_ptr<ast::Assembly> assembly);
        void generateAssembly(std::shared_ptr<ast::Assembly> assembly);
        llvm::Function *generateFunctionPrototype(std::shared_ptr<ast::Function> function);
        llvm::Function *generateFunctionPrototypeWithName(std::shared_ptr<ast::Function> function, std::string mangledName);
        llvm::Value *generateFunction(std::shared_ptr<ast::Function> function);
        llvm::Value *generateFunctionWithName(std::shared_ptr<ast::Function> function, std::string mangledName);
        llvm::Value *generateExpression(std::shared_ptr<ast::Expression> expression);
        llvm::Value *generateBinaryExpression(std::shared_ptr<ast::BinaryExpressionNode> expression);
        llvm::Value *generateIntegerMath(std::string op, llvm::Value *lhs, llvm::Value *rhs);
        llvm::Value *generateFloatingPointMath(std::string op, llvm::Value *lhs, llvm::Value *rhs);
        llvm::Value *generateFunctionCall(std::shared_ptr<ast::FunctionCallNode> expression);
        llvm::Value *generateQualifiedCall(std::shared_ptr<ast::QualifiedCallNode> expression);
        llvm::Value *generateAlloc(std::shared_ptr<ast::AllocNode> allocNode);
        llvm::Value *generateMemberAccess(std::shared_ptr<ast::MemberAccessNode> memberNode);
        llvm::Value *generateMethodCall(std::shared_ptr<ast::MethodCallNode> call);
        void generateClassMethods(std::shared_ptr<ast::ClassDeclaration> classDecl);
        void generateNamespacePrototypes(std::shared_ptr<ast::NamespaceDeclaration> ns, std::string parentPath);
        void generateNamespaceBodies(std::shared_ptr<ast::NamespaceDeclaration> ns, std::string parentPath);
        std::string mangleName(std::string namespacePath, std::string funcName);
        void generateIf(std::shared_ptr<ast::IfBlockNode> ifNode);
        void generateWhile(std::shared_ptr<ast::WhileNode> whileNode);
        llvm::Value *generateBlock(std::shared_ptr<ast::BlockNode> block, llvm::Function * llvmFunc);
        llvm::Value *generateIntoBlock(llvm::BasicBlock *basicBlock, std::shared_ptr<ast::BlockNode> block);

        // Reference counting helpers
        bool isRefCountedType(const std::string& typeName);
        void generateRetain(llvm::Value* ptr);
        void generateRelease(llvm::Value* ptr);
        void enterRefCountScope();
        void leaveRefCountScope();
        void releaseAllInCurrentScope();
        void releaseAllScopes();  // For return statements - release all ref-counted vars
    public:
        CodeGen(std::shared_ptr<ast::Assembly> tree, std::string outFile="");

        void setOptimize(bool optimize) { mOptimize = optimize; }
        void generate();
        bool compileToExecutable(const std::string& outputPath);
        void freeResources();
    };
}
