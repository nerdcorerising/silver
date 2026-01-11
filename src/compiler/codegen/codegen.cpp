

#include "codegen.h"

#include <string>
#include <cstring>
#include <cstdio>
#include "logger.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"

using namespace std;
using namespace ast;

// Runtime functions built into the compiler for JIT execution
extern "C" {
    void silver_print_string(const char* s) {
        printf("%s\n", s);
    }
    void silver_print_int(int n) {
        printf("%d\n", n);
    }
    void silver_print_float(double f) {
        printf("%f\n", f);
    }
    int silver_strcmp(const char* a, const char* b) {
        return strcmp(a, b) == 0 ? 1 : 0;
    }
}

// TODO: type inference
// TODO: The call to printf causes a segfault, see what you're doing wrong

namespace codegen
{
    CodeGen::CodeGen(shared_ptr<Assembly> tree, string outFile) :
        mOutFile(outFile),
        mTree(tree),
        mTable(),
        mFunctions(),
        mOptimize(false),
        mContext(),
        mModule(nullptr),
        mMain(nullptr),
        mBuilder(mContext),
        mFpm(nullptr)
    {

    }

    void CodeGen::addSystemCalls()
    {
        llvm::Type *voidTy = llvm::Type::getVoidTy(mContext);
        llvm::Type *i32Ty = llvm::Type::getInt32Ty(mContext);
        llvm::Type *i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0);
        llvm::Type *doubleTy = llvm::Type::getDoubleTy(mContext);

        // silver_print_string(const char* s) -> void
        llvm::FunctionType *printStringTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
        llvm::Function *printStringFunc = llvm::Function::Create(
            printStringTy, llvm::Function::ExternalLinkage, "silver_print_string", mModule);
        putFunc("silver_print_string", printStringFunc);

        // silver_print_int(int n) -> void
        llvm::FunctionType *printIntTy = llvm::FunctionType::get(voidTy, {i32Ty}, false);
        llvm::Function *printIntFunc = llvm::Function::Create(
            printIntTy, llvm::Function::ExternalLinkage, "silver_print_int", mModule);
        putFunc("silver_print_int", printIntFunc);

        // silver_print_float(double f) -> void
        llvm::FunctionType *printFloatTy = llvm::FunctionType::get(voidTy, {doubleTy}, false);
        llvm::Function *printFloatFunc = llvm::Function::Create(
            printFloatTy, llvm::Function::ExternalLinkage, "silver_print_float", mModule);
        putFunc("silver_print_float", printFloatFunc);

        // silver_strcmp(const char* a, const char* b) -> int (1 if equal, 0 if not)
        llvm::FunctionType *strcmpTy = llvm::FunctionType::get(i32Ty, {i8PtrTy, i8PtrTy}, false);
        llvm::Function *strcmpFunc = llvm::Function::Create(
            strcmpTy, llvm::Function::ExternalLinkage, "silver_strcmp", mModule);
        putFunc("silver_strcmp", strcmpFunc);

        // silver_retain(void* ptr) -> void (increment ref count)
        llvm::FunctionType *retainTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
        llvm::Function *retainFunc = llvm::Function::Create(
            retainTy, llvm::Function::ExternalLinkage, "silver_retain", mModule);
        putFunc("silver_retain", retainFunc);

        // silver_release(void* ptr) -> void (decrement ref count, free if zero)
        llvm::FunctionType *releaseTy = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
        llvm::Function *releaseFunc = llvm::Function::Create(
            releaseTy, llvm::Function::ExternalLinkage, "silver_release", mModule);
        putFunc("silver_release", releaseFunc);

        // silver_alloc(size_t size) -> void* (allocate memory with ref count header)
        llvm::Type *i64Ty = llvm::Type::getInt64Ty(mContext);
        llvm::FunctionType *allocTy = llvm::FunctionType::get(i8PtrTy, {i64Ty}, false);
        llvm::Function *allocFunc = llvm::Function::Create(
            allocTy, llvm::Function::ExternalLinkage, "silver_alloc", mModule);
        putFunc("silver_alloc", allocFunc);

        // silver_refcount(void* ptr) -> int (get current ref count for debugging/testing)
        llvm::FunctionType *refcountTy = llvm::FunctionType::get(i32Ty, {i8PtrTy}, false);
        llvm::Function *refcountFunc = llvm::Function::Create(
            refcountTy, llvm::Function::ExternalLinkage, "silver_refcount", mModule);
        putFunc("silver_refcount", refcountFunc);
    }

    void CodeGen::reportFatalError(string message)
    {
        fprintf(stderr, "Error during codegen phase: %s\n", message.c_str());
        throw message;
    }

    void CodeGen::putFunc(string name, llvm::Function *func)
    {
        mFunctions.insert({name, func});
    }

    llvm::Function *CodeGen::getFunc(string name)
    {
        auto it = mFunctions.find(name);
        if (it == mFunctions.end())
        {
            return nullptr;
        }

        return (*it).second;
    }

    bool CodeGen::isRefCountedType(const string& typeName)
    {
        // User-defined class types are ref-counted
        return mStructTypes.find(typeName) != mStructTypes.end();
    }

    void CodeGen::generateRetain(llvm::Value* ptr)
    {
        llvm::Function* retainFunc = getFunc("silver_retain");
        if (retainFunc)
        {
            // Cast to i8* if needed
            llvm::Value* castedPtr = mBuilder.CreateBitCast(ptr,
                llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0));
            mBuilder.CreateCall(retainFunc, {castedPtr});
        }
    }

    void CodeGen::generateRelease(llvm::Value* ptr)
    {
        llvm::Function* releaseFunc = getFunc("silver_release");
        if (releaseFunc)
        {
            // Cast to i8* if needed
            llvm::Value* castedPtr = mBuilder.CreateBitCast(ptr,
                llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0));
            mBuilder.CreateCall(releaseFunc, {castedPtr});
        }
    }

    void CodeGen::enterRefCountScope()
    {
        mRefCountedVarsStack.push_back(vector<string>());
    }

    void CodeGen::leaveRefCountScope()
    {
        // Only generate release calls if the current block doesn't already have a terminator
        // (e.g., a return statement already handled releases via releaseAllScopes)
        llvm::BasicBlock *currentBlock = mBuilder.GetInsertBlock();
        if (currentBlock && !currentBlock->getTerminator())
        {
            releaseAllInCurrentScope();
        }
        mRefCountedVarsStack.pop_back();
    }

    void CodeGen::releaseAllInCurrentScope()
    {
        if (mRefCountedVarsStack.empty()) return;

        // Make sure we have a valid insert point
        llvm::BasicBlock* block = mBuilder.GetInsertBlock();
        if (!block) return;

        vector<string>& currentScope = mRefCountedVarsStack.back();
        for (const string& varName : currentScope)
        {
            llvm::AllocaInst* inst = mTable.get(varName);
            if (inst)
            {
                // Load the pointer and release it
                llvm::Value* ptr = mBuilder.CreateLoad(inst->getAllocatedType(), inst);
                generateRelease(ptr);
            }
        }
    }

    void CodeGen::releaseAllScopes()
    {
        // Release all ref-counted variables in all active scopes (for return statements)
        llvm::BasicBlock* block = mBuilder.GetInsertBlock();
        if (!block) return;

        for (auto it = mRefCountedVarsStack.rbegin(); it != mRefCountedVarsStack.rend(); ++it)
        {
            for (const string& varName : *it)
            {
                llvm::AllocaInst* inst = mTable.get(varName);
                if (inst)
                {
                    llvm::Value* ptr = mBuilder.CreateLoad(inst->getAllocatedType(), inst);
                    generateRelease(ptr);
                }
            }
        }
    }

    llvm::Type *CodeGen::stringToType(string str)
    {
        if (str == "int")
        {
            return llvm::Type::getInt32Ty(mContext);
        }
        else if (str == "float")
        {
            return llvm::Type::getDoubleTy(mContext);
        }
        else if (str == "string")
        {
            return llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0);
        }
        else if (str == "void")
        {
            return llvm::Type::getVoidTy(mContext);
        }
        else
        {
            // Check if it's a user-defined class type
            auto it = mStructTypes.find(str);
            if (it != mStructTypes.end())
            {
                // Return pointer to struct type
                return llvm::PointerType::get(it->second, 0);
            }
            reportFatalError("Unknown type: " + str);
            return nullptr;
        }
    }

    vector<llvm::Type *> CodeGen::getFunctionArgumentTypes(shared_ptr<Function> function)
    {
        vector<llvm::Type *> types;
        for (size_t i = 0; i < function->argCount(); ++i)
        {
            string typeName = ((*function).getArguments()[i])->getType();
            llvm::Type *t = stringToType(typeName);

            types.push_back(t);
        }

        return types;
    }

    void CodeGen::generateClassTypes(shared_ptr<Assembly> assembly)
    {
        vector<shared_ptr<ClassDeclaration>> classes = assembly->getClasses();

        for (auto it = classes.begin(); it != classes.end(); ++it)
        {
            shared_ptr<ClassDeclaration> classDecl = *it;
            string className = classDecl->getName();

            // Store the class declaration for later lookup
            mClasses[className] = classDecl;

            // Create field types
            vector<llvm::Type *> fieldTypes;
            vector<shared_ptr<Field>> fields = classDecl->getFields();
            for (auto fieldIt = fields.begin(); fieldIt != fields.end(); ++fieldIt)
            {
                string fieldTypeName = (*fieldIt)->getType();
                llvm::Type *fieldType;

                // Handle primitive types directly (can't use stringToType yet for class types)
                if (fieldTypeName == "int")
                {
                    fieldType = llvm::Type::getInt32Ty(mContext);
                }
                else if (fieldTypeName == "float")
                {
                    fieldType = llvm::Type::getDoubleTy(mContext);
                }
                else if (fieldTypeName == "string")
                {
                    fieldType = llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0);
                }
                else
                {
                    reportFatalError("Unsupported field type: " + fieldTypeName);
                    return;
                }
                fieldTypes.push_back(fieldType);
            }

            // Create the struct type
            llvm::StructType *structType = llvm::StructType::create(mContext, fieldTypes, className);
            mStructTypes[className] = structType;

            LOG("Codegen: Created struct type for class %s with %zu fields\n",
                    className.c_str(), fieldTypes.size());
        }
    }

    void CodeGen::generateAssembly(shared_ptr<Assembly> assembly)
    {
        addSystemCalls();

        vector<shared_ptr<Function>> functions = assembly->getFunctions();

        for (auto it = functions.begin(); it != functions.end(); ++it)
        {
            shared_ptr<Function> function = *it;
            generateFunctionPrototype(function);
        }

        for (auto it = functions.begin(); it != functions.end(); ++it)
        {
            shared_ptr<Function> function = *it;
            generateFunction(function);
        }
    }

    llvm::Function *CodeGen::generateFunctionPrototype(shared_ptr<Function> function)
    {
        llvm::Type *retType = stringToType(function->getReturnType());
        vector<llvm::Type *> argTypes = getFunctionArgumentTypes(function);
        llvm::FunctionType *type = llvm::FunctionType::get(retType, argTypes, false);

        llvm::Value *funcVal = mModule->getOrInsertFunction(function->getName(), type).getCallee();
        llvm::Function *llvmFunc = llvm::cast<llvm::Function>(funcVal);
        putFunc(function->getName(), llvmFunc);

        if (function->getName() == "main")
        {
            if (mMain != nullptr)
            {
                reportFatalError("Multiple entry points defined.");
                return nullptr;
            }

            mMain = llvmFunc;
        }

        return llvmFunc;
    }

    llvm::Value *CodeGen::generateFunction(shared_ptr<Function> function)
    {
        LOG("Codegen: Generating function %s\n", function->getName().c_str());
        mTable.enterContext();
        mVariableTypes.enterContext();
        llvm::Function *llvmFunc = getFunc(function->getName());
        if (llvmFunc == nullptr)
        {
            reportFatalError("Undefined function " + function->getName() + " referenced");
            return nullptr;
        }

        llvm::BasicBlock::Create(mContext, "entry", llvmFunc, 0);

        //for (size_t i = 0; i < function->argCount(); ++i)
        size_t i = 0;
        auto it = llvmFunc->arg_begin();
        for ( ; it != llvmFunc->arg_end(); ++it, ++i)
        {
            shared_ptr<Argument> a = function->getArguments()[i];
            llvm::IRBuilder<> temp(&llvmFunc->getEntryBlock(), llvmFunc->getEntryBlock().begin());
            llvm::AllocaInst *inst = temp.CreateAlloca(stringToType(a->getType()), 0, a->getName());

            temp.CreateStore(&(*it), inst);

            mTable.put(a->getName(), inst);
            mVariableTypes.put(a->getName(), a->getType());
        }

        generateBlock(function->getBlock(), llvmFunc);

        // For void functions without explicit return, add implicit ret void
        llvm::BasicBlock *lastBlock = &llvmFunc->back();
        if (lastBlock->getTerminator() == nullptr)
        {
            if (function->getReturnType() == "void")
            {
                mBuilder.SetInsertPoint(lastBlock);
                mBuilder.CreateRetVoid();
            }
            else
            {
                reportFatalError("Non-void function " + function->getName() + " missing return statement");
            }
        }

        LOG("Codegen: Running optimization passes on %s\n", function->getName().c_str());

        // Verify function before optimization to catch IR issues
        std::string verifyError;
        llvm::raw_string_ostream verifyStream(verifyError);
        if (llvm::verifyFunction(*llvmFunc, &verifyStream))
        {
            fprintf(stderr, "Function verification failed: %s\n", verifyStream.str().c_str());
            llvmFunc->print(llvm::errs());
        }

        if (mOptimize)
        {
            mFpm->run(*llvmFunc);
        }
        LOG("Codegen: Finished function %s\n", function->getName().c_str());

        mTable.leaveContext();
        mVariableTypes.leaveContext();
        return llvmFunc;
    }

    llvm::Value *CodeGen::generateBinaryExpression(shared_ptr<BinaryExpressionNode> expression)
    {
        string op = expression->getOperator();

        llvm::Value *rhs = generateExpression(expression->getRhs());

        if (op == "=")
        {
            shared_ptr<Expression> binLhs = expression->getLhs();

            if (binLhs == nullptr)
            {
                // only can store to variable.
                reportFatalError("Found a store with a null assignment");
                return nullptr;
            }

            if (binLhs->getExpressionType() != Identifier && binLhs->getExpressionType() != Declaration)
            {
                reportFatalError("Cannot assign a value to a non-identifier type.");
                return nullptr;
            }

            llvm::Value *inst;
            if (binLhs->getExpressionType() == Identifier)
            {
                shared_ptr<IdentifierNode> castLhs = dynamic_pointer_cast<IdentifierNode>(binLhs);
                inst = mTable.get(castLhs->getValue());
            }
            else // if(binLhs->getType() == Declaration)
            {
                llvm::Value *exp = generateExpression(binLhs);
                inst = exp;
            }

            return mBuilder.CreateStore(rhs, inst);
        }

        llvm::Value *lhs = generateExpression(expression->getLhs());

        // Handle logical operators (work with boolean/i1 values from comparisons)
        if (op == "&&")
        {
            return mBuilder.CreateAnd(lhs, rhs, "and");
        }
        else if (op == "||")
        {
            return mBuilder.CreateOr(lhs, rhs, "or");
        }

        if (lhs->getType() == llvm::Type::getDoubleTy(mContext) || rhs->getType() == llvm::Type::getDoubleTy(mContext))
        {
            return generateFloatingPointMath(op, lhs, rhs);
        }
        else if (lhs->getType() == llvm::Type::getInt32Ty(mContext) && rhs->getType() == llvm::Type::getInt32Ty(mContext))
        {
            return generateIntegerMath(op, lhs, rhs);
        }
        else if (lhs->getType()->isPointerTy() && rhs->getType()->isPointerTy())
        {
            // String comparison - call runtime strcmp function
            if (op == "==" || op == "!=")
            {
                llvm::Function *strcmpFunc = getFunc("silver_strcmp");
                if (strcmpFunc == nullptr)
                {
                    reportFatalError("silver_strcmp function not found");
                    return nullptr;
                }

                // Call silver_strcmp(lhs, rhs) - returns i32 (1 if equal, 0 if not)
                std::vector<llvm::Value*> args;
                args.push_back(lhs);
                args.push_back(rhs);
                llvm::Value *intResult = mBuilder.CreateCall(strcmpFunc, args, "strcmp_result");
                // Convert i32 to i1 for boolean operations
                llvm::Value *result = mBuilder.CreateICmpNE(intResult,
                    llvm::ConstantInt::get(llvm::Type::getInt32Ty(mContext), 0), "streq");

                if (op == "==")
                {
                    return result;
                }
                else // op == "!="
                {
                    return mBuilder.CreateNot(result, "strne");
                }
            }
            else
            {
                reportFatalError("Unsupported pointer operation: " + op);
                return nullptr;
            }
        }
        else
        {
            reportFatalError("Type mismatch in binary expression");
            return nullptr;
        }

    }

    llvm::Value *CodeGen::generateIntegerMath(string op, llvm::Value *lhs, llvm::Value *rhs)
    {
        ASSERT(lhs->getType() == llvm::Type::getInt32Ty(mContext));
        ASSERT(rhs->getType() == llvm::Type::getInt32Ty(mContext));

        if (op == "+")
        {
            return mBuilder.CreateAdd(lhs, rhs);
        }
        else if (op == "-")
        {
            return mBuilder.CreateSub(lhs, rhs);
        }
        else if (op == "*")
        {
            return mBuilder.CreateMul(lhs, rhs);
        }
        else if (op == "/")
        {
            return mBuilder.CreateSDiv(lhs, rhs);
        }
        else if (op == "%")
        {
            return mBuilder.CreateSRem(lhs, rhs);
        }
        else if (op == "<")
        {
            return mBuilder.CreateICmpSLT(lhs, rhs);
        }
        else if (op == ">")
        {
            return mBuilder.CreateICmpSGT(lhs, rhs);
        }
        else if (op == "==")
        {
            return mBuilder.CreateICmpEQ(lhs, rhs);
        }
        else if (op == "!=")
        {
            return mBuilder.CreateICmpNE(lhs, rhs);
        }
        else if (op == ">=")
        {
            return mBuilder.CreateICmpSGE(lhs, rhs);
        }
        else if (op == "<=")
        {
            return mBuilder.CreateICmpSLE(lhs, rhs);
        }
        else
        {
            reportFatalError("unknown operator");
            return nullptr;
        }
    }

    llvm::Value *CodeGen::generateFloatingPointMath(string op, llvm::Value *lhs, llvm::Value *rhs)
    {
        ASSERT(lhs->getType() == llvm::Type::getInt32Ty(mContext) || lhs->getType() == llvm::Type::getDoubleTy(mContext));
        ASSERT(rhs->getType() == llvm::Type::getInt32Ty(mContext) || rhs->getType() == llvm::Type::getDoubleTy(mContext));

        // Promotion for floats and ints to work together -- very fragile. Need to do better type checking.
        if (lhs->getType() == llvm::Type::getInt32Ty(mContext))
        {
            lhs = mBuilder.CreateSIToFP(lhs, llvm::Type::getDoubleTy(mContext));
        }

        if (rhs->getType() == llvm::Type::getInt32Ty(mContext))
        {
            rhs = mBuilder.CreateSIToFP(rhs, llvm::Type::getDoubleTy(mContext));
        }

        if (op == "+")
        {
            return mBuilder.CreateFAdd(lhs, rhs);
        }
        else if (op == "-")
        {
            return mBuilder.CreateFSub(lhs, rhs);
        }
        else if (op == "*")
        {
            return mBuilder.CreateFMul(lhs, rhs);
        }
        else if (op == "/")
        {
            return mBuilder.CreateFDiv(lhs, rhs);
        }
        else if (op == "%")
        {
            return mBuilder.CreateFRem(lhs, rhs);
        }
        else if (op == "<")
        {
            return mBuilder.CreateFCmpOLT(lhs, rhs);
        }
        else if (op == ">")
        {
            return mBuilder.CreateFCmpOGT(lhs, rhs);
        }
        else if (op == "==")
        {
            return mBuilder.CreateFCmpOEQ(lhs, rhs);
        }
        else if (op == "!=")
        {
            return mBuilder.CreateFCmpONE(lhs, rhs);
        }
        else if (op == ">=")
        {
            return mBuilder.CreateFCmpOGE(lhs, rhs);
        }
        else if (op == "<=")
        {
            return mBuilder.CreateFCmpOLE(lhs, rhs);
        }
        else
        {
            reportFatalError("unknown operator");
            return nullptr;
        }
    }

    llvm::Value *CodeGen::generateFunctionCall(shared_ptr<FunctionCallNode> expression)
    {
        shared_ptr<FunctionCallNode> call = dynamic_pointer_cast<FunctionCallNode>(expression);

        llvm::Function *func = getFunc(call->getName());
        if (func == nullptr)
        {
            reportFatalError("Function " + call->getName() + " is not defined.");
            return nullptr;
        }

        vector<llvm::Value *> args;
        for (size_t i = 0; i < call->argCount(); ++i)
        {
            shared_ptr<Expression> argNode = dynamic_pointer_cast<Expression>(call->getArgs()[i]);
            llvm::Value *arg = generateExpression(argNode);

            // Cast struct pointers to i8* for runtime functions that expect void*
            if (call->getName() == "silver_refcount" && arg->getType()->isPointerTy())
            {
                arg = mBuilder.CreateBitCast(arg,
                    llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0));
            }

            args.push_back(arg);
        }

        return mBuilder.CreateCall(func, args);
    }

    llvm::Value *CodeGen::generateAlloc(shared_ptr<AllocNode> allocNode)
    {
        string typeName = allocNode->getTypeName();

        // Look up the struct type
        auto structIt = mStructTypes.find(typeName);
        if (structIt == mStructTypes.end())
        {
            reportFatalError("Unknown type in alloc: " + typeName);
            return nullptr;
        }
        llvm::StructType *structType = structIt->second;

        // Look up the class declaration for field info
        auto classIt = mClasses.find(typeName);
        if (classIt == mClasses.end())
        {
            reportFatalError("Unknown class in alloc: " + typeName);
            return nullptr;
        }
        shared_ptr<ClassDeclaration> classDecl = classIt->second;
        vector<shared_ptr<Field>> fields = classDecl->getFields();

        // Get the size of the struct
        const llvm::DataLayout& dataLayout = mModule->getDataLayout();
        uint64_t structSize = dataLayout.getTypeAllocSize(structType);

        // Allocate memory on the heap via silver_alloc (includes ref count header, starts at refcount=1)
        llvm::Function *allocFunc = getFunc("silver_alloc");
        if (allocFunc == nullptr)
        {
            reportFatalError("silver_alloc function not found");
            return nullptr;
        }

        llvm::Value *sizeVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(mContext), structSize);
        llvm::Value *rawPtr = mBuilder.CreateCall(allocFunc, {sizeVal}, "alloc_raw");

        // Cast the i8* to the struct pointer type
        llvm::Type *structPtrType = llvm::PointerType::get(structType, 0);
        llvm::Value *structPtr = mBuilder.CreateBitCast(rawPtr, structPtrType, typeName + "_ptr");

        // Initialize fields with provided arguments
        vector<shared_ptr<Expression>> args = allocNode->getArgs();
        if (args.size() != fields.size())
        {
            reportFatalError("Alloc argument count doesn't match field count for " + typeName);
            return nullptr;
        }

        for (size_t i = 0; i < args.size(); ++i)
        {
            // Generate the value for this field
            llvm::Value *fieldValue = generateExpression(args[i]);

            // Get pointer to the field using GEP
            llvm::Value *fieldPtr = mBuilder.CreateStructGEP(structType, structPtr, i, fields[i]->getName() + "_ptr");

            // Store the value
            mBuilder.CreateStore(fieldValue, fieldPtr);
        }

        return structPtr;
    }

    llvm::Value *CodeGen::generateMemberAccess(shared_ptr<MemberAccessNode> memberNode)
    {
        // Get the object expression - should be an identifier for a struct variable
        shared_ptr<Expression> objectExpr = memberNode->getObject();

        // We need to find the type name of the object
        string typeName;
        llvm::Value *objectPtr;

        if (objectExpr->getExpressionType() == ExpressionType::Identifier)
        {
            shared_ptr<IdentifierNode> ident = dynamic_pointer_cast<IdentifierNode>(objectExpr);
            string varName = ident->getValue();

            // Look up the type from our tracking table
            typeName = mVariableTypes.get(varName);
            if (typeName.empty())
            {
                reportFatalError("Unknown variable in member access: " + varName);
                return nullptr;
            }

            // Load the pointer from the variable
            llvm::AllocaInst *inst = mTable.get(varName);
            objectPtr = mBuilder.CreateLoad(inst->getAllocatedType(), inst);
        }
        else
        {
            reportFatalError("Member access on non-identifier expression not yet supported");
            return nullptr;
        }

        // Look up the struct type
        auto structIt = mStructTypes.find(typeName);
        if (structIt == mStructTypes.end())
        {
            reportFatalError("Unknown struct type in member access: " + typeName);
            return nullptr;
        }
        llvm::StructType *structType = structIt->second;

        // Look up the class declaration
        auto classIt = mClasses.find(typeName);
        if (classIt == mClasses.end())
        {
            reportFatalError("Unknown class in member access: " + typeName);
            return nullptr;
        }
        shared_ptr<ClassDeclaration> classDecl = classIt->second;

        // Find the field index
        string memberName = memberNode->getMemberName();
        size_t fieldIndex = classDecl->getFieldIndex(memberName);
        if (fieldIndex == (size_t)-1)
        {
            reportFatalError("Unknown field: " + memberName + " in class " + typeName);
            return nullptr;
        }

        // Get field type
        vector<shared_ptr<Field>> fields = classDecl->getFields();
        string fieldTypeName = fields[fieldIndex]->getType();
        llvm::Type *fieldType;
        if (fieldTypeName == "int")
        {
            fieldType = llvm::Type::getInt32Ty(mContext);
        }
        else if (fieldTypeName == "float")
        {
            fieldType = llvm::Type::getDoubleTy(mContext);
        }
        else if (fieldTypeName == "string")
        {
            fieldType = llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0);
        }
        else
        {
            reportFatalError("Unsupported field type in member access: " + fieldTypeName);
            return nullptr;
        }

        // Generate GEP to get pointer to field
        llvm::Value *fieldPtr = mBuilder.CreateStructGEP(structType, objectPtr, fieldIndex, memberName + "_ptr");

        // Load the field value
        return mBuilder.CreateLoad(fieldType, fieldPtr, memberName);
    }

    void CodeGen::generateIf(shared_ptr<IfBlockNode> ifNode)
    {
        llvm::BasicBlock *preheaderBlock = mBuilder.GetInsertBlock();
        llvm::Function *function = preheaderBlock->getParent();

        vector<shared_ptr<IfNode>> ifs = ifNode->getIfs();
        shared_ptr<IfNode> current = ifs[0];
        llvm::BasicBlock *currentConditionBlock = llvm::BasicBlock::Create(mContext, "if condition", function);
        llvm::BasicBlock *currentBodyBlock = llvm::BasicBlock::Create(mContext, "if body", function);
        mBuilder.SetInsertPoint(preheaderBlock);
        mBuilder.CreateBr(currentConditionBlock);
        int pos = 1;
        shared_ptr<IfNode> next = ifs.size() > pos ? ifs[pos] : nullptr;

        vector<llvm::BasicBlock *> bodyBlocks;
        bodyBlocks.push_back(currentBodyBlock);

        // Track blocks that need terminators after generateIntoBlock (for nested control flow)
        vector<llvm::BasicBlock *> exitBlocks;

        llvm::Value *cmp;
        while (next != nullptr)
        {
            llvm::BasicBlock *nextConditionBlock = llvm::BasicBlock::Create(mContext, "elif condition", function);
            llvm::BasicBlock *nextBodyBlock = llvm::BasicBlock::Create(mContext, "elif body", function);
            bodyBlocks.push_back(nextBodyBlock);

            mBuilder.SetInsertPoint(currentConditionBlock);
            cmp = generateExpression(current->getCondition());
            mBuilder.CreateCondBr(cmp, currentBodyBlock, nextConditionBlock);

            generateIntoBlock(currentBodyBlock, current->getBlock());

            // Track where we ended up after processing the block
            llvm::BasicBlock *exitBlock = mBuilder.GetInsertBlock();
            if (exitBlock && !exitBlock->getTerminator())
            {
                exitBlocks.push_back(exitBlock);
            }

            currentConditionBlock = nextConditionBlock;
            currentBodyBlock = nextBodyBlock;
            current = next;
            ++pos;
            next = ifs.size() > pos ? ifs[pos] : nullptr;
        }

        mBuilder.SetInsertPoint(currentConditionBlock);
        cmp = generateExpression(current->getCondition());

        generateIntoBlock(currentBodyBlock, current->getBlock());

        // Track where we ended up after processing the last if body
        llvm::BasicBlock *lastIfExitBlock = mBuilder.GetInsertBlock();
        if (lastIfExitBlock && !lastIfExitBlock->getTerminator())
        {
            exitBlocks.push_back(lastIfExitBlock);
        }

        shared_ptr<BlockNode> elseBlock = ifNode->getElseBlock();
        llvm::BasicBlock *end;
        if (elseBlock != nullptr)
        {
            llvm::BasicBlock *elseBasicBlock = llvm::BasicBlock::Create(mContext, "else body", function);
            bodyBlocks.push_back(elseBasicBlock);
            generateIntoBlock(elseBasicBlock, elseBlock);

            // Track where we ended up after processing else block
            llvm::BasicBlock *elseExitBlock = mBuilder.GetInsertBlock();
            if (elseExitBlock && !elseExitBlock->getTerminator())
            {
                exitBlocks.push_back(elseExitBlock);
            }

            mBuilder.SetInsertPoint(currentConditionBlock);
            mBuilder.CreateCondBr(cmp, currentBodyBlock, elseBasicBlock);

            end = llvm::BasicBlock::Create(mContext, "end of if block", function);
        }
        else
        {
            end = llvm::BasicBlock::Create(mContext, "end of if block", function);
            mBuilder.SetInsertPoint(currentConditionBlock);
            mBuilder.CreateCondBr(cmp, currentBodyBlock, end);
        }

        bool terminated = false;

        // Add terminators to all exit blocks from nested control flow
        for (auto it = exitBlocks.begin(); it != exitBlocks.end(); ++it)
        {
            if (!(*it)->getTerminator())
            {
                mBuilder.SetInsertPoint(*it);
                mBuilder.CreateBr(end);
                terminated = true;
            }
        }

        // Also check direct body blocks
        for (auto it = bodyBlocks.begin(); it != bodyBlocks.end(); ++it)
        {
            if ((*it)->getTerminator() == nullptr)
            {
                mBuilder.SetInsertPoint(*it);
                mBuilder.CreateBr(end);
                terminated = true;
            }
        }

        // Only delete end block if:
        // 1. There IS an else block (so conditional branch targets elseBasicBlock, not end)
        // 2. No body blocks branch to end (terminated is false)
        // If there's no else block, end is referenced by the conditional branch's false path
        if (elseBlock != nullptr && !terminated)
        {
            end->eraseFromParent();
        }
        else
        {
            mBuilder.SetInsertPoint(end);
        }
    }

    void CodeGen::generateWhile(shared_ptr<ast::WhileNode> whileNode)
    {
        llvm::BasicBlock *preHeaderBlock = mBuilder.GetInsertBlock();
        llvm::Function *function = preHeaderBlock->getParent();
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(mContext, "entry", function);
        llvm::BasicBlock *condition = llvm::BasicBlock::Create(mContext, "condition", function);
        llvm::BasicBlock *body = llvm::BasicBlock::Create(mContext, "body", function);
        llvm::BasicBlock *end = llvm::BasicBlock::Create(mContext, "end", function);

        mBuilder.SetInsertPoint(preHeaderBlock);
        mBuilder.CreateBr(entry);

        mBuilder.SetInsertPoint(entry);
        mBuilder.CreateBr(condition);

        generateIntoBlock(body, whileNode->getBlock());
        // Only add branch if current block doesn't have a terminator (e.g., from return)
        if (!mBuilder.GetInsertBlock()->getTerminator())
        {
            mBuilder.CreateBr(entry);
        }

        mBuilder.SetInsertPoint(condition);
        llvm::Value *cmp = generateExpression(whileNode->getCondition());
        mBuilder.CreateCondBr(cmp, body, end);

        mBuilder.SetInsertPoint(end);
    }

    llvm::Value *CodeGen::generateExpression(shared_ptr<Expression> expression)
    {
        switch (expression->getExpressionType())
        {
        case ExpressionType::IntegerLiteral:
        {
            shared_ptr<IntegerLiteralNode> i = dynamic_pointer_cast<IntegerLiteralNode>(expression);
            return llvm::ConstantInt::get(mContext, llvm::APInt(32, i->getValue(), true));
        }
        case ExpressionType::FloatLiteral:
        {
            shared_ptr<FloatLiteralNode> f = dynamic_pointer_cast<FloatLiteralNode>(expression);
            return llvm::ConstantFP::get(mContext, llvm::APFloat(f->getValue()));
        }
        case ExpressionType::StringLiteral:
        {
            shared_ptr<StringLiteralNode> str = dynamic_pointer_cast<StringLiteralNode>(expression);
            // Create a global string constant and return a pointer to it
            return mBuilder.CreateGlobalStringPtr(str->getValue(), "str");
        }
        case ExpressionType::UnaryOperator:
        {
            reportFatalError("not implemented");
            return nullptr;
        }
        case ExpressionType::BinaryOperator:
        {
            shared_ptr<BinaryExpressionNode> ben = dynamic_pointer_cast<BinaryExpressionNode>(expression);
            return generateBinaryExpression(ben);
        }
        case ExpressionType::Identifier:
        {
            shared_ptr<IdentifierNode> var = dynamic_pointer_cast<IdentifierNode>(expression);
            llvm::AllocaInst *inst = mTable.get(var->getValue());

            return mBuilder.CreateLoad(inst->getAllocatedType(), inst);
        }
        case ExpressionType::Declaration:
        {
            shared_ptr<DeclarationNode> decl = dynamic_pointer_cast<DeclarationNode>(expression);
            shared_ptr<Expression> initExpr = decl->getExpression();

            // Determine the type - either from explicit type or from alloc expression
            string typeName = decl->getTypeName();
            if (typeName.empty() && initExpr != nullptr && initExpr->getExpressionType() == ExpressionType::Alloc)
            {
                shared_ptr<AllocNode> allocNode = dynamic_pointer_cast<AllocNode>(initExpr);
                typeName = allocNode->getTypeName();
            }

            LOG("Codegen: Declaration %s type=%s\n", decl->getName().c_str(), typeName.c_str());

            ASSERT(typeName != "");
            llvm::Type *type = stringToType(typeName);

            llvm::AllocaInst *inst = mBuilder.CreateAlloca(type, 0, decl->getName());
            mTable.put(decl->getName(), inst);
            mVariableTypes.put(decl->getName(), typeName);

            // Track ref-counted variables for automatic release on scope exit
            if (isRefCountedType(typeName) && !mRefCountedVarsStack.empty())
            {
                mRefCountedVarsStack.back().push_back(decl->getName());
                LOG("Codegen: Tracking ref-counted variable %s\n", decl->getName().c_str());
            }

            // If declaration has an initializer, store the value
            if (initExpr != nullptr)
            {
                LOG("Codegen: Generating initializer for %s\n", decl->getName().c_str());
                llvm::Value *initValue = generateExpression(initExpr);
                LOG("Codegen: Storing initializer for %s\n", decl->getName().c_str());
                mBuilder.CreateStore(initValue, inst);
            }

            return inst;
        }
        case ExpressionType::Return:
        {
            shared_ptr<ReturnNode> ret = dynamic_pointer_cast<ReturnNode>(expression);
            llvm::Value *exp = generateExpression(ret->getExpression());

            // Release all ref-counted variables before returning
            releaseAllScopes();

            return mBuilder.CreateRet(exp);
        }
        case ExpressionType::Cast:
        {
            shared_ptr<CastNode> cast = dynamic_pointer_cast<CastNode>(expression);

            llvm::Value *exp = generateExpression(cast->getExpression());
            llvm::Type *type = stringToType(cast->getCastType());

            // TODO: a more generic casting mechanism
            if (type == llvm::Type::getDoubleTy(mContext))
            {
                return mBuilder.CreateSIToFP(exp, llvm::Type::getDoubleTy(mContext));
            }
            else if (type == llvm::Type::getInt32Ty(mContext))
            {
                return mBuilder.CreateFPToSI(exp, llvm::Type::getInt32Ty(mContext));
            }
            else
            {
                reportFatalError("unsupported cast");
                return nullptr;
            }
        }
        case ExpressionType::FunctionCall:
        {
            shared_ptr<FunctionCallNode> fcn = dynamic_pointer_cast<FunctionCallNode>(expression);
            return generateFunctionCall(fcn);
        }
        case ExpressionType::Empty:
            return nullptr;
        case ExpressionType::IfBlock:
        {
            shared_ptr<IfBlockNode> ifNode = dynamic_pointer_cast<IfBlockNode>(expression);
            generateIf(ifNode);
            // TODO: should refactor so this doesn't get called
            return nullptr;
        }
        break;
        case ExpressionType::While:
        {
            shared_ptr<WhileNode> whileNode = dynamic_pointer_cast<WhileNode>(expression);
            generateWhile(whileNode);
            // TODO: should refactor so this doesn't get called
            return nullptr;
        }
        break;
        case ExpressionType::Alloc:
        {
            shared_ptr<AllocNode> allocNode = dynamic_pointer_cast<AllocNode>(expression);
            return generateAlloc(allocNode);
        }
        case ExpressionType::MemberAccess:
        {
            shared_ptr<MemberAccessNode> memberNode = dynamic_pointer_cast<MemberAccessNode>(expression);
            return generateMemberAccess(memberNode);
        }
        default:
        {
            reportFatalError("Unknown expression type in codegen");
            return nullptr;
        }
        }
    }

    llvm::Value *CodeGen::generateBlock(shared_ptr<BlockNode> block, llvm::Function * llvmFunc)
    {
        llvm::BasicBlock *basicBlock;
        if (llvmFunc->empty())
        {
            basicBlock = llvm::BasicBlock::Create(mContext, "", llvmFunc, 0);
        }
        else
        {
            basicBlock = &llvmFunc->front();
        }

        return generateIntoBlock(basicBlock, block);
    }

    llvm::Value *CodeGen::generateIntoBlock(llvm::BasicBlock *basicBlock, shared_ptr<BlockNode> block)
    {
        mTable.enterContext();
        mVariableTypes.enterContext();
        enterRefCountScope();

        mBuilder.SetInsertPoint(basicBlock);

        vector<shared_ptr<Expression>> expressions = block->getExpressions();
        for (auto it = expressions.begin(); it != expressions.end(); ++it)
        {
            shared_ptr<Expression> exp = *it;
            generateExpression(exp);
        }

        leaveRefCountScope();
        mTable.leaveContext();
        mVariableTypes.leaveContext();
        return basicBlock;

    }

    void CodeGen::generate()
    {
        shared_ptr<Assembly> assembly = mTree;

        mModule = new llvm::Module(assembly->getName(), mContext);

        // Initialize native target for data layout info
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        // Set up target triple and data layout early so we can compute struct sizes
        std::string targetTriple = llvm::sys::getDefaultTargetTriple();
        mModule->setTargetTriple(llvm::Triple(targetTriple));

        std::string error;
        const llvm::Target *target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
        if (target)
        {
            llvm::TargetOptions opt;
            llvm::TargetMachine *tm = target->createTargetMachine(
                targetTriple, "generic", "", opt, llvm::Reloc::PIC_);
            if (tm)
            {
                mModule->setDataLayout(tm->createDataLayout());
                delete tm;
            }
        }

        mFpm = new llvm::legacy::FunctionPassManager(mModule);

        // Provide basic AliasAnalysis support for GVN.
        //mFpm->add(llvm::createBasicAliasAnalysisPass());
        // Promote allocas to registers.
        mFpm->add(llvm::createPromoteMemoryToRegisterPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        mFpm->add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        mFpm->add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        mFpm->add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        mFpm->add(llvm::createCFGSimplificationPass());

        mFpm->doInitialization();

        generateClassTypes(assembly);
        generateAssembly(assembly);
        if (logging::Logger::isEnabled())
        {
            LOG("Codegen: Assembly generation complete, printing module\n");
            mModule->print(llvm::errs(), nullptr);
            LOG("Codegen: Module printed, verifying\n");
        }
        llvm::verifyModule(*mModule);

        filebuf fb;
        if (mOutFile != "" && fb.open(mOutFile, ios::binary | ios::out))
        {
            ostream output(&fb);
            llvm::raw_os_ostream ls(output);
            llvm::WriteBitcodeToFile(*mModule, ls);
        }
    }

    int CodeGen::runJit()
    {
        if (mMain == nullptr)
        {
            cout << "No main() found." << endl;
            return -1;
        }

        // Initialize native target for JIT
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        // Register built-in runtime functions with LLVM
        llvm::sys::DynamicLibrary::AddSymbol("silver_print_string", (void*)silver_print_string);
        llvm::sys::DynamicLibrary::AddSymbol("silver_print_int", (void*)silver_print_int);
        llvm::sys::DynamicLibrary::AddSymbol("silver_print_float", (void*)silver_print_float);
        llvm::sys::DynamicLibrary::AddSymbol("silver_strcmp", (void*)silver_strcmp);

        std::string errStr;
        llvm::ExecutionEngine *EE = llvm::EngineBuilder(unique_ptr<llvm::Module>(mModule))
            .setErrorStr(&errStr)
            .setEngineKind(llvm::EngineKind::JIT)
            .create();

        if (!EE) {
            cerr << "Failed to create ExecutionEngine: " << errStr << endl;
            return -1;
        }

        mModule = nullptr; // now owned by unique_ptr above;

        cout << "calling main()" << endl;
        // Call the main function with no arguments:
        vector<llvm::GenericValue> noargs;
        llvm::GenericValue gv = EE->runFunction(mMain, noargs);

        // Import result of execution:
        int result = -1;
        if (mMain->getReturnType() == llvm::Type::getInt32Ty(mContext))
        {
            result = (int)gv.IntVal.getSExtValue();
        }
        else
        {
            reportFatalError("unknown main return type");
        }

        cout << "Result: " << result << endl;

        delete EE;
        llvm::llvm_shutdown();

        return result;
    }

    bool CodeGen::compileToExecutable(const std::string& outputPath)
    {
        // Initialize native target
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        // Get the target triple for this machine
        std::string targetTriple = llvm::sys::getDefaultTargetTriple();
        mModule->setTargetTriple(llvm::Triple(targetTriple));

        // Look up the target
        std::string error;
        const llvm::Target *target = llvm::TargetRegistry::lookupTarget(targetTriple, error);
        if (!target)
        {
            cerr << "Failed to lookup target: " << error << endl;
            return false;
        }

        // Create target machine
        llvm::TargetOptions opt;
        llvm::TargetMachine *targetMachine = target->createTargetMachine(
            targetTriple, "generic", "", opt, llvm::Reloc::PIC_);

        if (!targetMachine)
        {
            cerr << "Failed to create target machine" << endl;
            return false;
        }

        mModule->setDataLayout(targetMachine->createDataLayout());

        // Generate object file
        std::string objPath = outputPath + ".obj";
        std::error_code ec;
        llvm::raw_fd_ostream dest(objPath, ec, llvm::sys::fs::OF_None);
        if (ec)
        {
            cerr << "Could not open file: " << ec.message() << endl;
            return false;
        }

        llvm::legacy::PassManager passManager;
        if (targetMachine->addPassesToEmitFile(passManager, dest, nullptr,
            llvm::CodeGenFileType::ObjectFile))
        {
            cerr << "Target machine can't emit object file" << endl;
            return false;
        }

        passManager.run(*mModule);
        dest.flush();
        dest.close();

        cout << "Generated object file: " << objPath << endl;

        // Link with runtime to create executable
        std::string exePath = outputPath + ".exe";
        std::string linkCmd = "link.exe /nologo /subsystem:console ";
        linkCmd += "/out:" + exePath + " ";
        linkCmd += objPath + " ";
        linkCmd += "silver_runtime.lib ";
        linkCmd += "libcmt.lib libvcruntime.lib libucrt.lib kernel32.lib ";

        cout << "Linking: " << linkCmd << endl;
        int linkResult = system(linkCmd.c_str());

        if (linkResult != 0)
        {
            cerr << "Linking failed with code " << linkResult << endl;
            return false;
        }

        cout << "Generated executable: " << exePath << endl;

        // Clean up object file
        remove(objPath.c_str());

        delete targetMachine;
        return true;
    }

    void CodeGen::freeResources()
    {
        if (mModule)
        {
            delete mModule;
        }

        if (mFpm)
        {
            delete mFpm;
        }
    }
}
