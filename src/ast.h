
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
        Cast
    };

    class Argument
    {
    private:
        std::string mType;
        std::string mName;

    public:
        Argument(std::string type, std::string name);

        std::string getType() const;
        std::string getName() const;
    };

    class Node
    {
    protected:
        virtual void newLine(std::ostream &out, size_t indent);

    public:
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) = 0;
    };

    class Assembly : public Node
    {
    private:
        std::vector<std::shared_ptr<Function>> mFunctions;
        std::string mName;

    public:
        Assembly(std::string name, std::vector<std::shared_ptr<Function>> functions);
        size_t size();
        std::vector<std::shared_ptr<Function>> getFunctions();
        std::string getName();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class Block : public Node
    {
    private:
        std::vector<std::shared_ptr<Expression>> mExpressions;

    public:
        Block(std::vector<std::shared_ptr<Expression>> expressions);
        size_t size();
        std::vector<std::shared_ptr<Expression>> getExpressions();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class Function : public Node
    {
    private:
        std::shared_ptr<Block> mBlock;
        std::string mName;
        std::vector<std::shared_ptr<Argument>> mArgs;
        std::string mReturnType;

    public:
        Function(std::shared_ptr<Block> block, std::string name, std::vector<std::shared_ptr<Argument>> arguments, std::string returnType);
        std::shared_ptr<Block> getBlock();
        std::string getName() const;
        std::string getReturnType() const;
        size_t argCount();
        std::vector<std::shared_ptr<Argument>> getArguments();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class Expression : public Node
    {
    public:
        virtual ExpressionType getExpressionType() = 0;
    };

    class DeclarationNode : public Expression
    {
    private:
        std::string mType;
        std::string mName;

    public:
        DeclarationNode(std::string type, std::string name);
        virtual ExpressionType getExpressionType() override;
        std::string getName();
        std::string getTypeName();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class ReturnNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mExpression;

    public:
        ReturnNode(std::shared_ptr<Expression> expression);
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
        std::shared_ptr<Expression> getExpression();
    };

    class CastNode : public Expression
    {
    private:
        std::string mCastType;
        std::shared_ptr<Expression> mExpression;

    public:
        CastNode(std::string castType, std::shared_ptr<Expression> expression);
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
        std::string getCastType();
        std::shared_ptr<Expression> getExpression();
    };

    class IntegerLiteralNode : public Expression
    {
    private:
        int mValue;

    public:
        IntegerLiteralNode(int val);
        virtual ExpressionType getExpressionType() override;
        int getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class StringLiteralNode : public Expression
    {
    private:
        std::string mValue;

    public:
        StringLiteralNode(std::string val);
        virtual ExpressionType getExpressionType() override;
        std::string getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class FloatLiteralNode : public Expression
    {
    private:
        double mValue;

    public:
        FloatLiteralNode(double val);
        virtual ExpressionType getExpressionType() override;
        double getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class IdentifierNode : public Expression
    {
    private:
        std::string mValue;

    public:
        IdentifierNode(std::string val);
        virtual ExpressionType getExpressionType() override;
        std::string getValue();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class BinaryExpressionNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mLhs;
        std::shared_ptr<Expression> mRhs;
        std::string mOp;

    public:
        BinaryExpressionNode(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs, std::string op);
        virtual ExpressionType getExpressionType() override;
        std::shared_ptr<Expression> getLhs();
        std::shared_ptr<Expression> getRhs();
        std::string getOperator();
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class EmptyStatementNode : public Expression
    {
    public:
        EmptyStatementNode();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };

    class FunctionCallNode : public Expression
    {
    private:
        std::vector<std::shared_ptr<Expression>> mArgs;
        std::string mName;

    public:
        FunctionCallNode(std::string name, std::vector<std::shared_ptr<Expression>> args);
        std::string getName();
        size_t argCount();
        std::vector<std::shared_ptr<Expression>> getArgs();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent = 0) override;
    };
}
