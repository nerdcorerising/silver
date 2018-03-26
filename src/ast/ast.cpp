
#include "ast.h"

using namespace std;

namespace ast
{
    void Node::newLine(ostream &out, size_t indent)
    {
        out << endl;
        int spaces = indent * 4;
        for (int i = 0; i < spaces; ++i)
        {
            out << " ";
        }
    }

    BlockNode::BlockNode(vector<shared_ptr<Expression>> expressions) :
        mExpressions(expressions)
    {

    }

    ExpressionType BlockNode::getExpressionType()
    {
        return ExpressionType::Block;
    }

    size_t BlockNode::size()
    {
        return mExpressions.size();
    }

    vector<shared_ptr<Expression>> &BlockNode::getExpressions()
    {
        return mExpressions;
    }

    void BlockNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "{";
        ++indent;

        for (auto it = mExpressions.begin(); it < mExpressions.end(); ++it)
        {
            newLine(out, indent);
            (*it)->prettyPrint(out, indent);
            out << ";";
        }

        --indent;
        newLine(out, indent);
        out << "}";
    }

    Assembly::Assembly(string name, vector<shared_ptr<Function>> functions) :
        mName(name),
        mFunctions(functions)
    {

    }

    vector<shared_ptr<Function>> Assembly::getFunctions()
    {
        return mFunctions;
    }

    size_t Assembly::size()
    {
        return mFunctions.size();
    }

    string Assembly::getName()
    {
        return mName;
    }

    void Assembly::prettyPrint(ostream &out)
    {
        prettyPrint(out, 0);
    }

    void Assembly::prettyPrint(ostream &out, size_t indent)
    {
        out << "Assembly name: " << mName;
        ++indent;

        for (auto it = mFunctions.begin(); it < mFunctions.end(); ++it)
        {
            newLine(out, indent);
            (*it)->prettyPrint(out, indent);
        }

        newLine(out, 0);
    }

    Function::Function(shared_ptr<BlockNode> block, string name, vector<shared_ptr<Argument>> arguments, string returnType) :
        mBlock(block),
        mName(name),
        mArgs(arguments),
        mReturnType(returnType)
    {
        ASSERT(mBlock != nullptr);
    }

    shared_ptr<BlockNode> Function::getBlock()
    {
        return mBlock;
    }

    string Function::getName() const
    {
        return mName;
    }

    size_t Function::argCount()
    {
        return mArgs.size();
    }

    vector<shared_ptr<Argument>> Function::getArguments()
    {
        return mArgs;
    }

    string Function::getReturnType() const
    {
        return mReturnType;
    }

    void Function::prettyPrint(ostream &out, size_t indent)
    {
        out << "Function";
        ++indent;
        newLine(out, indent);

        out << "Name:" << mName;
        newLine(out, indent);

        out << "Return type: " << mReturnType;
        newLine(out, indent);

        out << "Arguments:";

        ++indent;
        if (mArgs.size() == 0)
        {
            newLine(out, indent);
            out << "none";
        }
        else
        {
            for (auto it = mArgs.begin(); it < mArgs.end(); ++it)
            {
                newLine(out, indent);
                out << "Name: " << (*it)->getName() << " ";
                out << "Type: " << (*it)->getType();
            }
        }
        --indent;
        newLine(out, indent);

        out << "Block: ";
        mBlock->prettyPrint(out, indent + 1);
    }

    DeclarationNode::DeclarationNode(string name, string type, shared_ptr<Expression> expression) :
        mName(name),
        mType(type),
        mExpression(expression)
    {
    }

    ExpressionType DeclarationNode::getExpressionType()
    {
        return ExpressionType::Declaration;
    }

    string DeclarationNode::getName()
    {
        return mName;
    }

    string DeclarationNode::getTypeName()
    {
        return mType;
    }

    shared_ptr<Expression> DeclarationNode::getExpression()
    {
        return mExpression;
    }

    void DeclarationNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << "Declaration type:" << mType << " name:" << mName;
    }

    ReturnNode::ReturnNode(shared_ptr<Expression> expression) :
        mExpression(expression)
    {

    }

    ExpressionType ReturnNode::getExpressionType()
    {
        return ExpressionType::Return;
    }

    shared_ptr<Expression> ReturnNode::getExpression()
    {
        return mExpression;
    }

    void ReturnNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << "return ";
        mExpression->prettyPrint(out, indent);
    }

    CastNode::CastNode(string castType, shared_ptr<Expression> expression) :
        mExpression(expression),
        mCastType(castType)
    {

    }

    ExpressionType CastNode::getExpressionType()
    {
        return ExpressionType::Cast;
    }

    void CastNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "Cast - type (" << mCastType << ") expression: (";
        mExpression->prettyPrint(out, indent);
        out << ")";
    }

    shared_ptr<Expression> CastNode::getExpression()
    {
        return mExpression;
    }

    string CastNode::getCastType()
    {
        return mCastType;
    }

    IntegerLiteralNode::IntegerLiteralNode(int val) :
        mValue(val)
    {

    }

    ExpressionType IntegerLiteralNode::getExpressionType()
    {
        return ExpressionType::IntegerLiteral;
    }

    int IntegerLiteralNode::getValue()
    {
        return mValue;
    }

    void IntegerLiteralNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << mValue;
    }


    EmptyStatementNode::EmptyStatementNode()
    {
    }

    ExpressionType EmptyStatementNode::getExpressionType()
    {
        return ExpressionType::Empty;
    }

    void EmptyStatementNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << "(Empty Statement)";
    }

    BinaryExpressionNode::BinaryExpressionNode(shared_ptr<Expression> lhs, shared_ptr<Expression> rhs, string op) :
        mLhs(lhs),
        mRhs(rhs),
        mOp(op)
    {
    }

    ExpressionType BinaryExpressionNode::getExpressionType()
    {
        return ExpressionType::BinaryOperator;
    }

    shared_ptr<Expression> BinaryExpressionNode::getLhs()
    {
        return mLhs;
    }

    shared_ptr<Expression> BinaryExpressionNode::getRhs()
    {
        return mRhs;
    }

    string BinaryExpressionNode::getOperator()
    {
        return mOp;
    }

    void BinaryExpressionNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << "(" << mOp << " ";
        mLhs->prettyPrint(out, indent);
        out << " ";
        mRhs->prettyPrint(out, indent);
        out << ")";
    }

    StringLiteralNode::StringLiteralNode(string val) :
        mValue(val)
    {
    }

    ExpressionType StringLiteralNode::getExpressionType()
    {
        return ExpressionType::StringLiteral;
    }

    string StringLiteralNode::getValue()
    {
        return mValue;
    }

    void StringLiteralNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << "\"" << mValue << "\"";
    }

    FloatLiteralNode::FloatLiteralNode(double val) :
        mValue(val)
    {
    }

    ExpressionType FloatLiteralNode::getExpressionType()
    {
        return ExpressionType::FloatLiteral;
    }

    double FloatLiteralNode::getValue()
    {
        return mValue;
    }

    void FloatLiteralNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << mValue << "f";
    }

    IdentifierNode::IdentifierNode(string val) :
        mValue(val)
    {
    }

    ExpressionType IdentifierNode::getExpressionType()
    {
        return ExpressionType::Identifier;
    }

    string IdentifierNode::getValue()
    {
        return mValue;
    }

    void IdentifierNode::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);

        out << "Identifier:" << mValue;
    }

    Argument::Argument(string type, string name) :
        mType(type),
        mName(name)
    {

    }

    string Argument::getType() const
    {
        return mType;
    }

    string Argument::getName() const
    {
        return mName;
    }

    FunctionCallNode::FunctionCallNode(string name, vector<shared_ptr<Expression>> args) :
        mName(name),
        mArgs(args)
    {

    }

    string FunctionCallNode::getName()
    {
        return mName;
    }

    size_t FunctionCallNode::argCount()
    {
        return mArgs.size();
    }

    vector<shared_ptr<Expression>> FunctionCallNode::getArgs()
    {
        return mArgs;
    }

    ExpressionType FunctionCallNode::getExpressionType()
    {
        return ExpressionType::FunctionCall;
    }

    void FunctionCallNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "Function call";
        newLine(out, indent);
        out << "name: [" << mName << "] ";
        newLine(out, indent);
        out << "Args: ";

        for (auto it = mArgs.begin(); it != mArgs.end(); ++it)
        {
            (*it)->prettyPrint(out, indent);
        }
    }

    ElseIfNode::ElseIfNode(shared_ptr<Expression> condition, shared_ptr<BlockNode> block) :
        mCondition(condition),
        mBlock(block)
    {

    }

    shared_ptr<Expression> ElseIfNode::getCondition()
    {
        return mCondition;
    }

    shared_ptr<BlockNode> ElseIfNode::getBlock()
    {
        return mBlock;
    }

    ExpressionType ElseIfNode::getExpressionType()
    {
        return ExpressionType::ElseIf;
    }

    void ElseIfNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "Else If Node";
        newLine(out, indent + 1);
        out << "Condition: ";
        newLine(out, indent);
        mCondition->prettyPrint(out, indent + 1);
        newLine(out, indent);
        out << "Block: ";
        mBlock->prettyPrint(out, indent + 1);
    }

    IfNode::IfNode(shared_ptr<Expression> condition, 
                   shared_ptr<BlockNode> ifBlock, 
                   vector<shared_ptr<ElseIfNode>> elseIfs, 
                   shared_ptr<BlockNode> elseBlock) :
        mCondition(condition),
        mIfBlock(ifBlock),
        mElseIfs(elseIfs),
        mElseBlock(elseBlock)
    {

    }

    shared_ptr<Expression> IfNode::getCondition()
    {
        return mCondition;
    }

    shared_ptr<BlockNode> IfNode::getIfBlock()
    {
        return mIfBlock;
    }

    vector<shared_ptr<ElseIfNode>> IfNode::getElseIfs()
    {
        return mElseIfs;
    }

    shared_ptr<BlockNode> IfNode::getElseBlock()
    {
        return mElseBlock;
    }

    ExpressionType IfNode::getExpressionType()
    {
        return ExpressionType::If;
    }

    void IfNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "If Node";
        newLine(out, indent);
        out << "Condition: ";
        mCondition->prettyPrint(out, indent);
        newLine(out, indent);
        out << "Block: ";
        mIfBlock->prettyPrint(out, indent);
        newLine(out, indent);

        for (auto it = mElseIfs.begin(); it != mElseIfs.end(); ++it)
        {
            (*it)->prettyPrint(out, indent);
        }

        if (mElseBlock != nullptr)
        {
            out << "Else block: ";
            mElseBlock->prettyPrint(out, indent);
        }
    }

    WhileNode::WhileNode(shared_ptr<Expression> condition, shared_ptr<BlockNode> block) :
        mCondition(condition),
        mBlock(block)
    {

    }

    shared_ptr<Expression> WhileNode::getCondition()
    {
        return mCondition;
    }

    shared_ptr<BlockNode> WhileNode::getBlock()
    {
        return mBlock;
    }

    ExpressionType WhileNode::getExpressionType()
    {
        return ExpressionType::While;
    }

    void WhileNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "While Node";
        newLine(out, indent);
        out << "Condition: ";
        mCondition->prettyPrint(out, indent);
        newLine(out, indent);
        out << "Block: ";
        mBlock->prettyPrint(out, indent);
    }

}
