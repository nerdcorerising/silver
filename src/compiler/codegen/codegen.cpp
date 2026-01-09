

#include "codegen.h"

#include <string>
#include <cstring>
#include <cstdio>
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"

using namespace std;
using namespace ast;

// TODO: type inference
// TODO: The call to printf causes a segfault, see what you're doing wrong

namespace codegen
{
    CodeGen::CodeGen(shared_ptr<Assembly> tree, string outFile) :
        mOutFile(outFile),
        mTree(tree),
        mTable(),
        mFunctions(),
        mContext(),
        mModule(nullptr),
        mMain(nullptr),
        mBuilder(mContext),
        mFpm(nullptr)
    {

    }

    void CodeGen::addSystemCalls()
    {
        // Add printf as a valid target: int printf(const char* format, ...)
        std::vector<llvm::Type*> printfParams;
        printfParams.push_back(llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0));
        llvm::FunctionType *printFType = llvm::FunctionType::get(
            llvm::IntegerType::getInt32Ty(mContext),  // return type: int
            printfParams,                              // params: const char*
            true);                                     // isVarArg: true
        llvm::FunctionCallee printfCallee = mModule->getOrInsertFunction("printf", printFType);
        llvm::Value *printfVal = printfCallee.getCallee();
        llvm::Function *printfFunc = llvm::cast<llvm::Function>(printfVal);
        putFunc("printf", printfFunc);

        // Create __silver_strcmp function: i1 __silver_strcmp(i8* s1, i8* s2)
        // Returns true (1) if strings are equal, false (0) otherwise
        std::vector<llvm::Type*> strcmpParams;
        strcmpParams.push_back(llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0));
        strcmpParams.push_back(llvm::PointerType::get(llvm::Type::getInt8Ty(mContext), 0));
        llvm::FunctionType *strcmpFType = llvm::FunctionType::get(
            llvm::Type::getInt1Ty(mContext),  // return type: i1 (bool)
            strcmpParams,                      // params: i8*, i8*
            false);                            // isVarArg: false

        llvm::Function *strcmpFunc = llvm::Function::Create(
            strcmpFType, llvm::Function::InternalLinkage, "__silver_strcmp", mModule);

        // Mark as noinline to avoid optimization issues
        strcmpFunc->addFnAttr(llvm::Attribute::NoInline);

        // Create the function body using alloca instead of PHI to be more robust
        llvm::BasicBlock *entryBB = llvm::BasicBlock::Create(mContext, "entry", strcmpFunc);
        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(mContext, "loop", strcmpFunc);
        llvm::BasicBlock *equalBB = llvm::BasicBlock::Create(mContext, "equal", strcmpFunc);
        llvm::BasicBlock *notEqualBB = llvm::BasicBlock::Create(mContext, "notequal", strcmpFunc);

        llvm::IRBuilder<> builder(mContext);

        // Get function arguments
        auto argIt = strcmpFunc->arg_begin();
        llvm::Value *s1 = &(*argIt++);
        llvm::Value *s2 = &(*argIt);
        s1->setName("s1");
        s2->setName("s2");

        // Entry block - allocate index, initialize to 0, jump to loop
        builder.SetInsertPoint(entryBB);
        llvm::AllocaInst *idxPtr = builder.CreateAlloca(llvm::Type::getInt64Ty(mContext), nullptr, "idx_ptr");
        builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(mContext), 0), idxPtr);
        builder.CreateBr(loopBB);

        // Loop block - load index, compare characters
        builder.SetInsertPoint(loopBB);
        llvm::Value *idx = builder.CreateLoad(llvm::Type::getInt64Ty(mContext), idxPtr, "idx");

        // Load characters from both strings
        llvm::Value *ptr1 = builder.CreateGEP(llvm::Type::getInt8Ty(mContext), s1, idx, "ptr1");
        llvm::Value *ptr2 = builder.CreateGEP(llvm::Type::getInt8Ty(mContext), s2, idx, "ptr2");
        llvm::Value *char1 = builder.CreateLoad(llvm::Type::getInt8Ty(mContext), ptr1, "char1");
        llvm::Value *char2 = builder.CreateLoad(llvm::Type::getInt8Ty(mContext), ptr2, "char2");

        // Compare characters - if not equal, return false
        llvm::Value *charsEqual = builder.CreateICmpEQ(char1, char2, "chars_eq");

        // Create blocks for after-comparison logic
        llvm::BasicBlock *checkNullBB = llvm::BasicBlock::Create(mContext, "check_null", strcmpFunc);
        builder.CreateCondBr(charsEqual, checkNullBB, notEqualBB);

        // Check null block - if chars are equal, check if we hit null terminator
        builder.SetInsertPoint(checkNullBB);
        llvm::Value *isNull = builder.CreateICmpEQ(char1,
            llvm::ConstantInt::get(llvm::Type::getInt8Ty(mContext), 0), "is_null");

        // Create continue block for incrementing and looping
        llvm::BasicBlock *continueBB = llvm::BasicBlock::Create(mContext, "continue", strcmpFunc);
        builder.CreateCondBr(isNull, equalBB, continueBB);

        // Continue block - increment index and loop back
        builder.SetInsertPoint(continueBB);
        llvm::Value *nextIdx = builder.CreateAdd(idx,
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(mContext), 1), "next_idx");
        builder.CreateStore(nextIdx, idxPtr);
        builder.CreateBr(loopBB);

        // Equal block - return true
        builder.SetInsertPoint(equalBB);
        builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt1Ty(mContext), 1));

        // Not equal block - return false
        builder.SetInsertPoint(notEqualBB);
        builder.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt1Ty(mContext), 0));

        putFunc("__silver_strcmp", strcmpFunc);
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

            fprintf(stderr, "Codegen: Created struct type for class %s with %zu fields\n",
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
        fprintf(stderr, "Codegen: Generating function %s\n", function->getName().c_str());
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

        fprintf(stderr, "Codegen: Running optimization passes on %s\n", function->getName().c_str());

        mFpm->run(*llvmFunc);
        fprintf(stderr, "Codegen: Finished function %s\n", function->getName().c_str());

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
            // String comparison - call our generated strcmp function
            if (op == "==" || op == "!=")
            {
                llvm::Function *strcmpFunc = getFunc("__silver_strcmp");
                if (strcmpFunc == nullptr)
                {
                    reportFatalError("__silver_strcmp function not found");
                    return nullptr;
                }

                // Call __silver_strcmp(lhs, rhs) - returns i1 (true if equal)
                std::vector<llvm::Value*> args;
                args.push_back(lhs);
                args.push_back(rhs);
                llvm::Value *result = mBuilder.CreateCall(strcmpFunc, args, "streq");

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

        // Allocate memory for the struct (stack allocation for now)
        llvm::AllocaInst *structPtr = mBuilder.CreateAlloca(structType, nullptr, typeName + "_instance");

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

            currentConditionBlock = nextConditionBlock;
            currentBodyBlock = nextBodyBlock;
            current = next;
            ++pos;
            next = ifs.size() > pos ? ifs[pos] : nullptr;
        }

        mBuilder.SetInsertPoint(currentConditionBlock);
        cmp = generateExpression(current->getCondition());

        generateIntoBlock(currentBodyBlock, current->getBlock());

        shared_ptr<BlockNode> elseBlock = ifNode->getElseBlock();
        llvm::BasicBlock *end;
        if (elseBlock != nullptr)
        {
            llvm::BasicBlock *elseBasicBlock = llvm::BasicBlock::Create(mContext, "else body", function);
            bodyBlocks.push_back(elseBasicBlock);
            generateIntoBlock(elseBasicBlock, elseBlock);

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
        mBuilder.CreateBr(entry);

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

            fprintf(stderr, "Codegen: Declaration %s type=%s\n", decl->getName().c_str(), typeName.c_str());

            ASSERT(typeName != "");
            llvm::Type *type = stringToType(typeName);

            llvm::AllocaInst *inst = mBuilder.CreateAlloca(type, 0, decl->getName());
            mTable.put(decl->getName(), inst);
            mVariableTypes.put(decl->getName(), typeName);

            // If declaration has an initializer, store the value
            if (initExpr != nullptr)
            {
                fprintf(stderr, "Codegen: Generating initializer for %s\n", decl->getName().c_str());
                llvm::Value *initValue = generateExpression(initExpr);
                fprintf(stderr, "Codegen: Storing initializer for %s\n", decl->getName().c_str());
                mBuilder.CreateStore(initValue, inst);
            }

            return inst;
        }
        case ExpressionType::Return:
        {
            shared_ptr<ReturnNode> ret = dynamic_pointer_cast<ReturnNode>(expression);
            llvm::Value *exp = generateExpression(ret->getExpression());

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

        mBuilder.SetInsertPoint(basicBlock);

        vector<shared_ptr<Expression>> expressions = block->getExpressions();
        for (auto it = expressions.begin(); it != expressions.end(); ++it)
        {
            shared_ptr<Expression> exp = *it;
            generateExpression(exp);
        }

        mTable.leaveContext();
        mVariableTypes.leaveContext();
        return basicBlock;

    }

    void CodeGen::generate()
    {
        shared_ptr<Assembly> assembly = mTree;

        mModule = new llvm::Module(assembly->getName(), mContext);

        mFpm = new llvm::legacy::FunctionPassManager(mModule);

        // Provide basic AliasAnalysis support for GVN.
        //mFpm->add(llvm::createBasicAliasAnalysisPass());
        // Promote allocas to registers.
        mFpm->add(llvm::createPromoteMemoryToRegisterPass());
        // Do simple "peephole" optimizations and bit-twiddling optzns.
        // NOTE: Disabled because it converts printf to puts which JIT can't resolve
        // mFpm->add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        mFpm->add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        mFpm->add(llvm::createGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        mFpm->add(llvm::createCFGSimplificationPass());

        mFpm->doInitialization();

        generateClassTypes(assembly);
        generateAssembly(assembly);
        fprintf(stderr, "Codegen: Assembly generation complete, printing module\n");
        mModule->print(llvm::errs(), nullptr);
        fprintf(stderr, "Codegen: Module printed, verifying\n");
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

        std::string errStr;
        llvm::ExecutionEngine *EE = llvm::EngineBuilder(unique_ptr<llvm::Module>(mModule))
            .setErrorStr(&errStr)
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
