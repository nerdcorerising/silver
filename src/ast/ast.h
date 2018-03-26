
#pragma once

#include <memory>
#include <vector>
#include <string>

#include "common.h"


namespace ast
{
    class Function;
    class Expression;

    // Valid expression types
    enum ExpressionType
    {
        IntegerLiteral,
        FloatLiteral,
        StringLiteral,
        UnaryOperator,
        BinaryOperator,
        Empty,
        Identifier,
        Declaration,
        FunctionCall,
        Return,
        Cast,
        If,
        ElseIf,
        While,
        Block
    };

    class Node
    {
    protected:
        virtual void newLine(std::ostream &out, size_t indent);

    public:
        Node() = default;
        virtual ~Node() = default;

        virtual void prettyPrint(std::ostream &out, size_t indent) = 0;
    };

    class Assembly : public Node
    {
    private:
        std::vector<std::shared_ptr<Function>> mFunctions;
        std::string mName;

    public:
        Assembly(std::string name, std::vector<std::shared_ptr<Function>> functions);
        virtual ~Assembly() = default;

        size_t size();
        std::vector<std::shared_ptr<Function>> getFunctions();
        std::string getName();
        void prettyPrint(std::ostream &out);
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class Expression : public Node
    {
    public:
        Expression() = default;
        virtual ~Expression() = default;

        virtual ExpressionType getExpressionType() = 0;
    };

    class BlockNode : public Expression
    {
    private:
        std::vector<std::shared_ptr<Expression>> mExpressions;

    public:
        BlockNode(std::vector<std::shared_ptr<Expression>> expressions);
        virtual ~BlockNode() = default;

        virtual ExpressionType getExpressionType() override;
        size_t size();
        std::vector<std::shared_ptr<Expression>> &getExpressions();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class Argument
    {
    private:
        std::string mType;
        std::string mName;

    public:
        Argument(std::string type, std::string name);
        virtual ~Argument() = default;

        std::string getType() const;
        std::string getName() const;
    };

    class Function : public Node
    {
    private:
        std::shared_ptr<BlockNode> mBlock;
        std::string mName;
        std::vector<std::shared_ptr<Argument>> mArgs;
        std::string mReturnType;

    public:
        Function(std::shared_ptr<BlockNode> block, 
                 std::string name, 
                 std::vector<std::shared_ptr<Argument>> arguments, 
                 std::string returnType);
        virtual ~Function() = default;

        std::shared_ptr<BlockNode> getBlock();
        std::string getName() const;
        std::string getReturnType() const;
        size_t argCount();
        std::vector<std::shared_ptr<Argument>> getArguments();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class DeclarationNode : public Expression
    {
    private:
        std::string mType;
        std::string mName;
        std::shared_ptr<Expression> mExpression;

    public:
        DeclarationNode(std::string name, std::string type, std::shared_ptr<Expression> expression);
        virtual ~DeclarationNode() = default;

        virtual ExpressionType getExpressionType() override;
        std::string getName();
        std::string getTypeName();
        std::shared_ptr<Expression> getExpression();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class ReturnNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mExpression;

    public:
        ReturnNode(std::shared_ptr<Expression> expression);
        virtual ~ReturnNode() = default;

        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
        std::shared_ptr<Expression> getExpression();
    };

    class CastNode : public Expression
    {
    private:
        std::string mCastType;
        std::shared_ptr<Expression> mExpression;

    public:
        CastNode(std::string castType, std::shared_ptr<Expression> expression);
        virtual ~CastNode() = default;

        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
        std::string getCastType();
        std::shared_ptr<Expression> getExpression();
    };

    class IntegerLiteralNode : public Expression
    {
    private:
        int mValue;

    public:
        IntegerLiteralNode(int val);
        virtual ~IntegerLiteralNode() = default;

        virtual ExpressionType getExpressionType() override;
        int getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class StringLiteralNode : public Expression
    {
    private:
        std::string mValue;

    public:
        StringLiteralNode(std::string val);
        virtual ~StringLiteralNode() = default;

        virtual ExpressionType getExpressionType() override;
        std::string getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class FloatLiteralNode : public Expression
    {
    private:
        double mValue;

    public:
        FloatLiteralNode(double val);
        virtual ~FloatLiteralNode() = default;

        virtual ExpressionType getExpressionType() override;
        double getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class IdentifierNode : public Expression
    {
    private:
        std::string mValue;

    public:
        IdentifierNode(std::string val);
        virtual ~IdentifierNode() = default;

        virtual ExpressionType getExpressionType() override;
        std::string getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class BinaryExpressionNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mLhs;
        std::shared_ptr<Expression> mRhs;
        std::string mOp;

    public:
        BinaryExpressionNode(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs, std::string op);
        virtual ~BinaryExpressionNode() = default;

        virtual ExpressionType getExpressionType() override;
        std::shared_ptr<Expression> getLhs();
        std::shared_ptr<Expression> getRhs();
        std::string getOperator();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class EmptyStatementNode : public Expression
    {
    public:
        EmptyStatementNode();
        virtual ~EmptyStatementNode() = default;

        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class FunctionCallNode : public Expression
    {
    private:
        std::vector<std::shared_ptr<Expression>> mArgs;
        std::string mName;

    public:
        FunctionCallNode(std::string name, std::vector<std::shared_ptr<Expression>> args);
        virtual ~FunctionCallNode() = default;

        std::string getName();
        size_t argCount();
        std::vector<std::shared_ptr<Expression>> getArgs();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class ElseIfNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mCondition;
        std::shared_ptr<BlockNode> mBlock;

    public:
        ElseIfNode(std::shared_ptr<Expression> condition, std::shared_ptr<BlockNode> block);
        virtual ~ElseIfNode() = default;

        std::shared_ptr<Expression> getCondition();
        std::shared_ptr<BlockNode> getBlock();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class IfNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mCondition;
        std::shared_ptr<BlockNode> mIfBlock;
        std::vector<std::shared_ptr<ElseIfNode>> mElseIfs;
        std::shared_ptr<BlockNode> mElseBlock;

    public:
        IfNode(std::shared_ptr<Expression> condition, 
               std::shared_ptr<BlockNode> ifBlock, 
               std::vector<std::shared_ptr<ElseIfNode>> elseIfs, 
               std::shared_ptr<BlockNode> elseBlock);
        virtual ~IfNode() = default;

        std::shared_ptr<Expression> getCondition();
        std::shared_ptr<BlockNode> getIfBlock();
        std::vector<std::shared_ptr<ElseIfNode>> getElseIfs();
        std::shared_ptr<BlockNode> getElseBlock();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class WhileNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mCondition;
        std::shared_ptr<BlockNode> mBlock;

    public:
        WhileNode(std::shared_ptr<Expression> condition, std::shared_ptr<BlockNode> block);
        virtual ~WhileNode() = default;

        std::shared_ptr<Expression> getCondition();
        std::shared_ptr<BlockNode> getBlock();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };
}
