
#include "ast.h"

using namespace std;

namespace ast
{
    void Node::newLine(ostream &out, size_t indent)
    {
        out << endl;
        size_t spaces = indent * 4;
        for (size_t i = 0; i < spaces; ++i)
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

    Assembly::Assembly(string name, vector<shared_ptr<Function>> functions, vector<shared_ptr<ClassDeclaration>> classes) :
        mName(name),
        mFunctions(functions),
        mClasses(classes)
    {

    }

    vector<shared_ptr<Function>> Assembly::getFunctions()
    {
        return mFunctions;
    }

    vector<shared_ptr<ClassDeclaration>> Assembly::getClasses()
    {
        return mClasses;
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

        for (auto it = mClasses.begin(); it < mClasses.end(); ++it)
        {
            newLine(out, indent);
            (*it)->prettyPrint(out, indent);
        }

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

    void DeclarationNode::setTypeName(string type)
    {
        mType = type;
    }

    shared_ptr<Expression> DeclarationNode::getExpression()
    {
        return mExpression;
    }

    void DeclarationNode::clearExpression()
    {
        mExpression = nullptr;
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

    IfNode::IfNode(shared_ptr<Expression> condition, shared_ptr<BlockNode> block) :
        mCondition(condition),
        mBlock(block)
    {

    }

    shared_ptr<Expression> IfNode::getCondition()
    {
        return mCondition;
    }

    shared_ptr<BlockNode> IfNode::getBlock()
    {
        return mBlock;
    }

    ExpressionType IfNode::getExpressionType()
    {
        return ExpressionType::If;
    }

    void IfNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "If Node";
        newLine(out, indent + 1);
        out << "Condition: ";
        mCondition->prettyPrint(out, indent + 1);
        newLine(out, indent);
        out << "Block: ";
        mBlock->prettyPrint(out, indent + 1);
    }

    IfBlockNode::IfBlockNode(vector<shared_ptr<IfNode>> ifs, shared_ptr<BlockNode> elseBlock) :
        mIfs(ifs),
        mElseBlock(elseBlock)
    {

    }

    vector<shared_ptr<IfNode>> IfBlockNode::getIfs()
    {
        return mIfs;
    }

    shared_ptr<BlockNode> IfBlockNode::getElseBlock()
    {
        return mElseBlock;
    }

    ExpressionType IfBlockNode::getExpressionType()
    {
        return ExpressionType::IfBlock;
    }

    void IfBlockNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "If Block Node";
        newLine(out, indent);

        for (auto it = mIfs.begin(); it != mIfs.end(); ++it)
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

    // Field implementation
    Field::Field(string name, string type, Visibility visibility) :
        mName(name),
        mType(type),
        mVisibility(visibility)
    {
    }

    string Field::getName() const
    {
        return mName;
    }

    string Field::getType() const
    {
        return mType;
    }

    Visibility Field::getVisibility() const
    {
        return mVisibility;
    }

    void Field::prettyPrint(ostream &out, size_t indent)
    {
        UNREFERENCED(indent);
        out << mName << ": ";
        out << (mVisibility == Visibility::Public ? "public " : "private ");
        out << mType;
    }

    // ClassDeclaration implementation
    ClassDeclaration::ClassDeclaration(string name, vector<shared_ptr<Field>> fields) :
        mName(name),
        mFields(fields)
    {
    }

    string ClassDeclaration::getName() const
    {
        return mName;
    }

    vector<shared_ptr<Field>> ClassDeclaration::getFields() const
    {
        return mFields;
    }

    size_t ClassDeclaration::getFieldIndex(const string& fieldName) const
    {
        for (size_t i = 0; i < mFields.size(); ++i)
        {
            if (mFields[i]->getName() == fieldName)
            {
                return i;
            }
        }
        return (size_t)-1; // Not found
    }

    void ClassDeclaration::prettyPrint(ostream &out, size_t indent)
    {
        out << "Class: " << mName;
        ++indent;
        newLine(out, indent);
        out << "Fields:";
        ++indent;
        for (auto it = mFields.begin(); it != mFields.end(); ++it)
        {
            newLine(out, indent);
            (*it)->prettyPrint(out, indent);
        }
    }

    // AllocNode implementation
    AllocNode::AllocNode(string typeName, vector<shared_ptr<Expression>> args) :
        mTypeName(typeName),
        mArgs(args)
    {
    }

    string AllocNode::getTypeName() const
    {
        return mTypeName;
    }

    vector<shared_ptr<Expression>> AllocNode::getArgs() const
    {
        return mArgs;
    }

    ExpressionType AllocNode::getExpressionType()
    {
        return ExpressionType::Alloc;
    }

    void AllocNode::prettyPrint(ostream &out, size_t indent)
    {
        out << "alloc " << mTypeName << "(";
        bool first = true;
        for (auto it = mArgs.begin(); it != mArgs.end(); ++it)
        {
            if (!first) out << ", ";
            first = false;
            (*it)->prettyPrint(out, indent);
        }
        out << ")";
    }

    // MemberAccessNode implementation
    MemberAccessNode::MemberAccessNode(shared_ptr<Expression> object, string memberName) :
        mObject(object),
        mMemberName(memberName)
    {
    }

    shared_ptr<Expression> MemberAccessNode::getObject() const
    {
        return mObject;
    }

    string MemberAccessNode::getMemberName() const
    {
        return mMemberName;
    }

    ExpressionType MemberAccessNode::getExpressionType()
    {
        return ExpressionType::MemberAccess;
    }

    void MemberAccessNode::prettyPrint(ostream &out, size_t indent)
    {
        mObject->prettyPrint(out, indent);
        out << "." << mMemberName;
    }

}
