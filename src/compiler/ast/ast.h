
#pragma once

#include <memory>
#include <vector>
#include <string>

#include "common.h"


namespace ast
{
    class Function;
    class Expression;
    class ClassDeclaration;
    class Field;
    class NamespaceDeclaration;

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
        IfBlock,
        If,
        While,
        Block,
        Alloc,
        MemberAccess,
        QualifiedCall,
        MethodCall
    };

    // Visibility for class fields
    enum class Visibility
    {
        Public,
        Private
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
        std::vector<std::shared_ptr<ClassDeclaration>> mClasses;
        std::vector<std::shared_ptr<NamespaceDeclaration>> mNamespaces;
        std::string mName;

    public:
        Assembly(std::string name,
                 std::vector<std::shared_ptr<Function>> functions,
                 std::vector<std::shared_ptr<ClassDeclaration>> classes = {},
                 std::vector<std::shared_ptr<NamespaceDeclaration>> namespaces = {});
        virtual ~Assembly() = default;

        size_t size();
        std::vector<std::shared_ptr<Function>> getFunctions();
        std::vector<std::shared_ptr<ClassDeclaration>> getClasses();
        std::vector<std::shared_ptr<NamespaceDeclaration>> getNamespaces();
        std::string getName();
        void prettyPrint(std::ostream &out);
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class Expression : public Node
    {
    private:
        int mLine;
        int mColumn;

    public:
        Expression(int line = 0, int column = 0) : mLine(line), mColumn(column) {}
        virtual ~Expression() = default;

        int line() const { return mLine; }
        int column() const { return mColumn; }

        virtual ExpressionType getExpressionType() = 0;
    };

    class BlockNode : public Expression
    {
    private:
        std::vector<std::shared_ptr<Expression>> mExpressions;

    public:
        BlockNode(std::vector<std::shared_ptr<Expression>> expressions, int line = 0, int col = 0);
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
        bool mIsLocal;
        Visibility mVisibility;

    public:
        Function(std::shared_ptr<BlockNode> block,
                 std::string name,
                 std::vector<std::shared_ptr<Argument>> arguments,
                 std::string returnType,
                 bool isLocal = false,
                 Visibility visibility = Visibility::Public);
        virtual ~Function() = default;

        std::shared_ptr<BlockNode> getBlock();
        std::string getName() const;
        std::string getReturnType() const;
        bool isLocal() const;
        Visibility getVisibility() const;
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
        DeclarationNode(std::string name, std::string type, std::shared_ptr<Expression> expression, int line = 0, int col = 0);
        virtual ~DeclarationNode() = default;

        virtual ExpressionType getExpressionType() override;
        std::string getName();
        std::string getTypeName();
        void setTypeName(std::string type);
        std::shared_ptr<Expression> getExpression();
        void clearExpression();
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class ReturnNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mExpression;

    public:
        ReturnNode(std::shared_ptr<Expression> expression, int line = 0, int col = 0);
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
        CastNode(std::string castType, std::shared_ptr<Expression> expression, int line = 0, int col = 0);
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
        IntegerLiteralNode(int val, int line = 0, int col = 0);
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
        StringLiteralNode(std::string val, int line = 0, int col = 0);
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
        FloatLiteralNode(double val, int line = 0, int col = 0);
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
        IdentifierNode(std::string val, int line = 0, int col = 0);
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
        BinaryExpressionNode(std::shared_ptr<Expression> lhs, std::shared_ptr<Expression> rhs, std::string op, int line = 0, int col = 0);
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
        EmptyStatementNode(int line = 0, int col = 0);
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
        FunctionCallNode(std::string name, std::vector<std::shared_ptr<Expression>> args, int line = 0, int col = 0);
        virtual ~FunctionCallNode() = default;

        std::string getName();
        size_t argCount();
        std::vector<std::shared_ptr<Expression>> getArgs();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class IfNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mCondition;
        std::shared_ptr<BlockNode> mBlock;

    public:
        IfNode(std::shared_ptr<Expression> condition, std::shared_ptr<BlockNode> block, int line = 0, int col = 0);
        virtual ~IfNode() = default;

        std::shared_ptr<Expression> getCondition();
        std::shared_ptr<BlockNode> getBlock();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    class IfBlockNode : public Expression
    {
    private:
        std::vector<std::shared_ptr<IfNode>> mIfs;
        std::shared_ptr<BlockNode> mElseBlock;

    public:
        IfBlockNode(std::vector<std::shared_ptr<IfNode>> ifs, std::shared_ptr<BlockNode> elseBlock, int line = 0, int col = 0);
        virtual ~IfBlockNode() = default;

        std::vector<std::shared_ptr<IfNode>> getIfs();
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
        WhileNode(std::shared_ptr<Expression> condition, std::shared_ptr<BlockNode> block, int line = 0, int col = 0);
        virtual ~WhileNode() = default;

        std::shared_ptr<Expression> getCondition();
        std::shared_ptr<BlockNode> getBlock();
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents a field in a class: "x: public int"
    class Field : public Node
    {
    private:
        std::string mName;
        std::string mType;
        Visibility mVisibility;

    public:
        Field(std::string name, std::string type, Visibility visibility);
        virtual ~Field() = default;

        std::string getName() const;
        std::string getType() const;
        Visibility getVisibility() const;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents a class declaration: "class MyType { ... }"
    class ClassDeclaration : public Node
    {
    private:
        std::string mName;
        std::vector<std::shared_ptr<Field>> mFields;
        std::vector<std::shared_ptr<Function>> mMethods;

    public:
        ClassDeclaration(std::string name,
                         std::vector<std::shared_ptr<Field>> fields,
                         std::vector<std::shared_ptr<Function>> methods = {});
        virtual ~ClassDeclaration() = default;

        std::string getName() const;
        std::vector<std::shared_ptr<Field>> getFields() const;
        std::vector<std::shared_ptr<Function>> getMethods() const;
        size_t getFieldIndex(const std::string& fieldName) const;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents allocation: "alloc MyType(args...)"
    class AllocNode : public Expression
    {
    private:
        std::string mTypeName;
        std::vector<std::shared_ptr<Expression>> mArgs;

    public:
        AllocNode(std::string typeName, std::vector<std::shared_ptr<Expression>> args, int line = 0, int col = 0);
        virtual ~AllocNode() = default;

        std::string getTypeName() const;
        std::vector<std::shared_ptr<Expression>> getArgs() const;
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents member access: "obj.field"
    class MemberAccessNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mObject;
        std::string mMemberName;

    public:
        MemberAccessNode(std::shared_ptr<Expression> object, std::string memberName, int line = 0, int col = 0);
        virtual ~MemberAccessNode() = default;

        std::shared_ptr<Expression> getObject() const;
        std::string getMemberName() const;
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents a method call: "obj.method(args...)"
    class MethodCallNode : public Expression
    {
    private:
        std::shared_ptr<Expression> mObject;
        std::string mMethodName;
        std::vector<std::shared_ptr<Expression>> mArgs;

    public:
        MethodCallNode(std::shared_ptr<Expression> object, std::string methodName,
                       std::vector<std::shared_ptr<Expression>> args, int line = 0, int col = 0);
        virtual ~MethodCallNode() = default;

        std::shared_ptr<Expression> getObject() const;
        std::string getMethodName() const;
        std::vector<std::shared_ptr<Expression>> getArgs() const;
        size_t argCount() const;
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents a namespace-qualified function call: "Math.add(1, 2)"
    class QualifiedCallNode : public Expression
    {
    private:
        std::string mNamespacePath;  // "Math" or "Math.Advanced"
        std::string mFunctionName;   // "add"
        std::vector<std::shared_ptr<Expression>> mArgs;

    public:
        QualifiedCallNode(std::string namespacePath, std::string functionName,
                          std::vector<std::shared_ptr<Expression>> args, int line = 0, int col = 0);
        virtual ~QualifiedCallNode() = default;

        std::string getNamespacePath() const;
        std::string getFunctionName() const;
        std::string getFullyQualifiedName() const;  // Returns "Math.add" or "Math.Advanced.add"
        std::vector<std::shared_ptr<Expression>> getArgs() const;
        size_t argCount() const;
        virtual ExpressionType getExpressionType() override;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };

    // Represents a namespace declaration: "namespace Math { ... }"
    class NamespaceDeclaration : public Node
    {
    private:
        std::string mName;  // Full dotted name, e.g., "Math.Advanced"
        std::vector<std::shared_ptr<Function>> mFunctions;
        std::vector<std::shared_ptr<ClassDeclaration>> mClasses;
        std::vector<std::shared_ptr<NamespaceDeclaration>> mNestedNamespaces;

    public:
        NamespaceDeclaration(std::string name,
                             std::vector<std::shared_ptr<Function>> functions,
                             std::vector<std::shared_ptr<ClassDeclaration>> classes,
                             std::vector<std::shared_ptr<NamespaceDeclaration>> nestedNamespaces);
        virtual ~NamespaceDeclaration() = default;

        std::string getName() const;
        std::vector<std::shared_ptr<Function>> getFunctions() const;
        std::vector<std::shared_ptr<ClassDeclaration>> getClasses() const;
        std::vector<std::shared_ptr<NamespaceDeclaration>> getNestedNamespaces() const;
        virtual void prettyPrint(std::ostream &out, size_t indent) override;
    };
}
