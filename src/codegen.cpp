

#include "codegen.h"


using namespace std;
using namespace ast;

SymbolTable::SymbolTable() :
    mStack(),
    mCurrent(),
    mFunctions()
{

}

void SymbolTable::putVar(string name, llvm::AllocaInst *inst)
{
    mCurrent.insert({name, inst});
}

llvm::AllocaInst *SymbolTable::getVar(string name)
{
    auto it = mCurrent.find(name);
    if (it == mCurrent.end())
    {
        bool found = false;
        for (auto revIt = mStack.rbegin(); revIt != mStack.rend(); ++revIt)
        {
            it = (*revIt).find(name);
            if (it != (*revIt).end())
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            return nullptr;
        }
    }

    return (*it).second;
}

void SymbolTable::putFunc(string name, llvm::Function *func)
{
    mFunctions.insert({ name, func });
}

llvm::Function *SymbolTable::getFunc(string name)
{
    auto it = mFunctions.find(name);
    if (it == mFunctions.end())
    {
        return nullptr;
    }

    return it->second;
}

void SymbolTable::enterContext()
{
    mStack.push_back(mCurrent);
    mCurrent.clear();
}

void SymbolTable::leaveContext()
{
    mCurrent = mStack.at(mStack.size() - 1);
    mStack.pop_back();
}

CodeGen::CodeGen(shared_ptr<Assembly> tree, string outFile) :
    mTree(tree),
    mOutFile(outFile),
    mTable(),
    mContext(),
    mModule(),
    mBuilder(mContext),
    mFpm()
{

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

void CodeGen::GenerateAssembly(shared_ptr<Assembly> assembly)
{
    vector<shared_ptr<Function>> functions = assembly->getFunctions();
    for (auto it = functions.begin(); it != functions.end(); ++it)
    {
        shared_ptr<Function> function = *it;
        GenerateFunction(function);
    }
}

llvm::Value *CodeGen::GenerateFunction(shared_ptr<Function> function)
{
    mTable.enterContext();

    llvm::Type *retType = stringToType(function->getReturnType());
    vector<llvm::Type *> argTypes = getFunctionArgumentTypes(function);
    llvm::FunctionType *type = llvm::FunctionType::get(retType, argTypes, false);

    llvm::Function *llvmFunc = llvm::cast<llvm::Function>(mModule->getOrInsertFunction(function->getName(), type));
    mTable.putFunc(function->getName(), llvmFunc);

    if (function->getName() == "main")
    {
        if (mMain != nullptr)
        {
            throw "Multiple entry points defined.";
        }

        mMain = llvmFunc;
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

        mTable.putVar(a->getName(), inst);
    }

    GenerateBlock(function->getBlock(), llvmFunc);

    mFpm->run(*llvmFunc);

    mTable.leaveContext();
    return llvmFunc;
}

llvm::Value *CodeGen::GenerateBinaryExpression(shared_ptr<BinaryExpressionNode> expression)
{
    string op = expression->getOperator();

    llvm::Value *rhs = GenerateExpression(expression->getRhs());

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
            inst = mTable.getVar(castLhs->getValue());
        }
        else // if(binLhs->getType() == Declaration)
        {
            llvm::Value *exp = GenerateExpression(binLhs);
            inst = exp;
        }
        return mBuilder.CreateStore(rhs, inst);
    }

    llvm::Value *lhs = GenerateExpression(expression->getLhs());

    if (lhs->getType() == llvm::Type::getDoubleTy(mContext) || rhs->getType() == llvm::Type::getDoubleTy(mContext))
    {
        return GenerateFloatingPointMath(op, lhs, rhs);
    }
    else if (lhs->getType() == llvm::Type::getInt32Ty(mContext) && rhs->getType() == llvm::Type::getInt32Ty(mContext))
    {
        return GenerateIntegerMath(op, lhs, rhs);
    }
    else
    {
        throw "need to support strings and such here eventually";
    }

}

llvm::Value *CodeGen::GenerateIntegerMath(string op, llvm::Value *lhs, llvm::Value *rhs)
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

llvm::Value *CodeGen::GenerateFloatingPointMath(string op, llvm::Value *lhs, llvm::Value *rhs)
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

llvm::Value *CodeGen::GenerateFunctionCall(shared_ptr<FunctionCallNode> expression)
{
    shared_ptr<FunctionCallNode> call = dynamic_pointer_cast<FunctionCallNode>(expression);

    llvm::Function *func = mTable.getFunc(call->getName());

    vector<llvm::Value *> args;
    for (size_t i = 0; i < call->argCount(); ++i)
    {
        shared_ptr<Expression> argNode = dynamic_pointer_cast<Expression>(call->getArgs()[i]);
        llvm::Value *arg = GenerateExpression(argNode);

        args.push_back(arg);
    }

    return mBuilder.CreateCall(func, args);
}

llvm::Value *CodeGen::GenerateExpression(shared_ptr<Expression> expression)
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
        return GenerateBinaryExpression(ben);
    }
    case ExpressionType::Identifier:
    {
        shared_ptr<IdentifierNode> var = dynamic_pointer_cast<IdentifierNode>(expression);
        llvm::AllocaInst *inst = mTable.getVar(var->getValue());

        return mBuilder.CreateLoad(inst);
    }
    case ExpressionType::Declaration:
    {
        shared_ptr<DeclarationNode> decl = dynamic_pointer_cast<DeclarationNode>(expression);

        llvm::Type *type = stringToType(decl->getTypeName());

        llvm::AllocaInst *inst = mBuilder.CreateAlloca(type, 0, decl->getName());
        mTable.putVar(decl->getName(), inst);
        return inst;
    }
    case ExpressionType::Return:
    {
        shared_ptr<ReturnNode> ret = dynamic_pointer_cast<ReturnNode>(expression);
        llvm::Value *exp = GenerateExpression(ret->getExpression());

        return mBuilder.CreateRet(exp);
    }
    case ExpressionType::Cast:
    {
        shared_ptr<CastNode> cast = dynamic_pointer_cast<CastNode>(expression);
        
        llvm::Value *exp = GenerateExpression(cast->getExpression());
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
        return GenerateFunctionCall(fcn);
    }
    case ExpressionType::Empty:
        return nullptr;
    default:
        throw "Unknown expression type in codegen";
    }
}

llvm::Value *CodeGen::GenerateBlock(shared_ptr<Block> block, llvm::Function * llvmFunc)
{
    mTable.enterContext();

    llvm::BasicBlock *basicBlock;
    if (llvmFunc->getBasicBlockList().size() == 0)
    {
        basicBlock = llvm::BasicBlock::Create(mContext, "", llvmFunc, 0);
    }
    else
    {
        basicBlock = &llvmFunc->getBasicBlockList().front();
    }

    mBuilder.SetInsertPoint(basicBlock);

    vector<shared_ptr<Expression>> expressions = block->getExpressions();
    for (auto it = expressions.begin(); it != expressions.end(); ++it)
    {
        shared_ptr<Expression> exp = *it;
        GenerateExpression(exp);
    }

    mTable.leaveContext();
    return basicBlock;
}

void CodeGen::Generate()
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
    
    GenerateAssembly(assembly);
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

void CodeGen::RunJit()
{
    if (mMain == nullptr)
    {
        cout << "No main() found." << endl;
        return;
    }

    llvm::ExecutionEngine *EE = llvm::EngineBuilder(unique_ptr<llvm::Module>(mModule)).create();
    mModule = nullptr; // now owned by unique_ptr above;
    
    cout << "calling main()" << endl;
    // Call the main function with no arguments:
    vector<llvm::GenericValue> noargs;
    llvm::GenericValue gv = EE->runFunction(mMain, noargs);

    // Import result of execution:
    if (mMain->getReturnType() == llvm::Type::getInt32Ty(mContext))
    {
        cout << "Result: " << gv.IntVal.getSExtValue() << endl;
    }
    else if (mMain->getReturnType() == llvm::Type::getDoubleTy(mContext))
    {
        cout << "Result: " << gv.DoubleVal << endl;
    }
    else
    {
        throw "unknown main return type";
    }

    delete EE;
    llvm::llvm_shutdown();
}

void CodeGen::FreeResources()
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
