

#include "codegen.h"


using namespace std;
using namespace ast;

// TODO: type inference

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

    void CodeGen::putFunc(string name, llvm::Function *func)
    {
        mFunctions.insert({name, func});
    }

    llvm::Function *CodeGen::getFunc(string name)
    {
        auto it = mFunctions.find(name);
        if (it == mFunctions.end())
        {
            throw "uh oh!";
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
            return llvm::Type::getInt8PtrTy(mContext);
        }
        else if (str == "void")
        {
            return llvm::Type::getVoidTy(mContext);
        }
        else
        {
            throw "not implemented";
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

    void CodeGen::generateAssembly(shared_ptr<Assembly> assembly)
    {
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

        llvm::Function *llvmFunc = llvm::cast<llvm::Function>(mModule->getOrInsertFunction(function->getName(), type));
        putFunc(function->getName(), llvmFunc);

        if (function->getName() == "main")
        {
            if (mMain != nullptr)
            {
                throw "Multiple entry points defined.";
            }

            mMain = llvmFunc;
        }

        return llvmFunc;
    }

    llvm::Value *CodeGen::generateFunction(shared_ptr<Function> function)
    {
        mTable.enterContext();
        llvm::Function *llvmFunc = getFunc(function->getName());
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
        }

        generateBlock(function->getBlock(), llvmFunc);

        mFpm->run(*llvmFunc);

        mTable.leaveContext();
        return llvmFunc;
    }

    llvm::Value *CodeGen::generateBinaryExpression(shared_ptr<BinaryExpressionNode> expression)
    {
        string op = expression->getOperator();

        llvm::Value *rhs = generateExpression(expression->getRhs());

        if (op == "=")
        {
            shared_ptr<Expression> binLhs = expression->getLhs();

            if (binLhs == nullptr || (binLhs->getExpressionType() != Identifier && binLhs->getExpressionType() != Declaration))
            {
                // only can store to variable.
                throw "invalid, need to decide what to do for error stuff.";
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

        if (lhs->getType() == llvm::Type::getDoubleTy(mContext) || rhs->getType() == llvm::Type::getDoubleTy(mContext))
        {
            return generateFloatingPointMath(op, lhs, rhs);
        }
        else if (lhs->getType() == llvm::Type::getInt32Ty(mContext) && rhs->getType() == llvm::Type::getInt32Ty(mContext))
        {
            return generateIntegerMath(op, lhs, rhs);
        }
        else
        {
            throw "need to support strings and such here eventually";
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
            throw "unknown operator";
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
            throw "unknown operator";
        }
    }

    llvm::Value *CodeGen::generateFunctionCall(shared_ptr<FunctionCallNode> expression)
    {
        shared_ptr<FunctionCallNode> call = dynamic_pointer_cast<FunctionCallNode>(expression);

        llvm::Function *func = getFunc(call->getName());

        vector<llvm::Value *> args;
        for (size_t i = 0; i < call->argCount(); ++i)
        {
            shared_ptr<Expression> argNode = dynamic_pointer_cast<Expression>(call->getArgs()[i]);
            llvm::Value *arg = generateExpression(argNode);

            args.push_back(arg);
        }

        return mBuilder.CreateCall(func, args);
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

        bool didNotTerminateAny = true;
        for (auto it = bodyBlocks.begin(); it != bodyBlocks.end(); ++it)
        {
            if ((*it)->getTerminator() == nullptr)
            {
                mBuilder.SetInsertPoint(*it);
                mBuilder.CreateBr(end);
                didNotTerminateAny = false;
            }
        }

        if (didNotTerminateAny)
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
            return llvm::ConstantDataArray::getString(mContext, str->getValue());
        }
        case ExpressionType::UnaryOperator:
        {
            throw "not implemented";
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

            return mBuilder.CreateLoad(inst);
        }
        case ExpressionType::Declaration:
        {
            shared_ptr<DeclarationNode> decl = dynamic_pointer_cast<DeclarationNode>(expression);

            ASSERT(decl->getTypeName() != "");
            llvm::Type *type = stringToType(decl->getTypeName());

            llvm::AllocaInst *inst = mBuilder.CreateAlloca(type, 0, decl->getName());
            mTable.put(decl->getName(), inst);
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
                throw "unsupported cast";
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
        default:
            throw "Unknown expression type in codegen";
        }
    }

    llvm::Value *CodeGen::generateBlock(shared_ptr<BlockNode> block, llvm::Function * llvmFunc)
    {
        llvm::BasicBlock *basicBlock;
        if (llvmFunc->getBasicBlockList().size() == 0)
        {
            basicBlock = llvm::BasicBlock::Create(mContext, "", llvmFunc, 0);
        }
        else
        {
            basicBlock = &llvmFunc->getBasicBlockList().front();
        }

        return generateIntoBlock(basicBlock, block);
    }

    llvm::Value *CodeGen::generateIntoBlock(llvm::BasicBlock *basicBlock, shared_ptr<BlockNode> block)
    {
        mTable.enterContext();

        mBuilder.SetInsertPoint(basicBlock);

        vector<shared_ptr<Expression>> expressions = block->getExpressions();
        for (auto it = expressions.begin(); it != expressions.end(); ++it)
        {
            shared_ptr<Expression> exp = *it;
            generateExpression(exp);
        }

        mTable.leaveContext();
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
        mFpm->add(llvm::createInstructionCombiningPass());
        // Reassociate expressions.
        mFpm->add(llvm::createReassociatePass());
        // Eliminate Common SubExpressions.
        mFpm->add(llvm::createNewGVNPass());
        // Simplify the control flow graph (deleting unreachable blocks, etc).
        mFpm->add(llvm::createCFGSimplificationPass());

        mFpm->doInitialization();

        generateAssembly(assembly);
        mModule->print(llvm::errs(), nullptr);
        llvm::verifyModule(*mModule);

        filebuf fb;
        if (mOutFile != "" && fb.open(mOutFile, ios::binary | ios::out))
        {
            ostream output(&fb);
            llvm::raw_os_ostream ls(output);
            llvm::WriteBitcodeToFile(mModule, ls);
        }
    }

    int CodeGen::runJit()
    {
        if (mMain == nullptr)
        {
            cout << "No main() found." << endl;
            return -1;
        }

        llvm::ExecutionEngine *EE = llvm::EngineBuilder(unique_ptr<llvm::Module>(mModule)).create();
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
            throw "unknown main return type";
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
