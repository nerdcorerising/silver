
#include "typeinferencepass.h"
#include <map>
#include <sstream>
#include "logger.h"

using namespace std;
using namespace ast;

namespace analysis
{
    string TypeInferencePass::getTypeForExpression(shared_ptr<Expression> expression, SymbolTable<string, string> &symbols)
    {
        switch (expression->getExpressionType())
        {
        case ExpressionType::IfBlock:
        case ExpressionType::If:
        {
            OPTIMIZATION_ERROR_AT(expression, "Cannot assign result of If to variable");
        }
        break;
        case ExpressionType::While:
        {
            OPTIMIZATION_ERROR_AT(expression, "Cannot assign result of loop to variable");
        }
        break;
        case ExpressionType::Block:
        {
            OPTIMIZATION_ERROR_AT(expression, "Cannot assign result of block to variable");
        }
        break;
        case ExpressionType::IntegerLiteral:
        {
            return "int";
        }
        break;
        case ExpressionType::FloatLiteral:
        {
            return "float";
        }
        break;
        case ExpressionType::StringLiteral:
        {
            return "string";
        }
        break;
        case ExpressionType::UnaryOperator:
        {
            throw std::string("not implemented");
            // return ;
        }
        break;
        case ExpressionType::BinaryOperator:
        {
            shared_ptr<BinaryExpressionNode> expr = dynamic_pointer_cast<BinaryExpressionNode>(expression);
            string lhsType = getTypeForExpression(expr->getLhs(), symbols);
            string rhsType = getTypeForExpression(expr->getRhs(), symbols);

            if (lhsType != rhsType)
            {
                stringstream error;
                error << "Types " << lhsType << " and " << rhsType << " do not match";
                OPTIMIZATION_ERROR_AT(expression, error.str());
            }

            return lhsType;
        }
        break;
        case ExpressionType::Empty:
        {
            OPTIMIZATION_ERROR_AT(expression, "Cannot assign result of empty statement to variable");
        }
        break;
        case ExpressionType::Identifier:
        {
            shared_ptr<IdentifierNode> identifier = dynamic_pointer_cast<IdentifierNode>(expression);
            string type = symbols.get(identifier->getValue());
            return type;
        }
        break;
        case ExpressionType::Declaration:
        {
            OPTIMIZATION_ERROR_AT(expression, "Cannot assign result of declaration statement to variable");
        }
        break;
        case ExpressionType::FunctionCall:
        {
            shared_ptr<FunctionCallNode> call = dynamic_pointer_cast<FunctionCallNode>(expression);
            string funcName = call->getName() + "()";
            string type = symbols.get(funcName);
            return type;
        }
        break;
        case ExpressionType::Return:
        {
            OPTIMIZATION_ERROR_AT(expression, "Cannot assign result of return statement to variable");
        }
        break;
        case ExpressionType::Cast:
        {
            shared_ptr<CastNode> cast = dynamic_pointer_cast<CastNode>(expression);
            return cast->getCastType();
        }
        break;
        case ExpressionType::Alloc:
        {
            shared_ptr<AllocNode> alloc = dynamic_pointer_cast<AllocNode>(expression);
            return alloc->getTypeName();
        }
        break;
        case ExpressionType::MemberAccess:
        {
            shared_ptr<MemberAccessNode> member = dynamic_pointer_cast<MemberAccessNode>(expression);
            // Get the type of the object being accessed
            string objectType = getTypeForExpression(member->getObject(), symbols);
            // Look up the field type using "ClassName.fieldName"
            string fieldKey = objectType + "." + member->getMemberName();
            string fieldType = symbols.get(fieldKey);
            if (fieldType.empty())
            {
                OPTIMIZATION_ERROR_AT(expression, "Unknown field: " + member->getMemberName() + " in type " + objectType);
            }
            return fieldType;
        }
        break;
        case ExpressionType::QualifiedCall:
        {
            shared_ptr<QualifiedCallNode> call = dynamic_pointer_cast<QualifiedCallNode>(expression);
            // Look up "Namespace.funcName()" in symbol table
            string funcKey = call->getFullyQualifiedName() + "()";
            string type = symbols.get(funcKey);
            if (type.empty())
            {
                // Check if this is a local function that exists but can't be called from here
                string localFuncKey = "local:" + funcKey;
                string localType = symbols.get(localFuncKey);
                if (!localType.empty())
                {
                    OPTIMIZATION_ERROR_AT(expression, "Function " + call->getFullyQualifiedName() +
                        " is local and can only be called from within its namespace");
                }
                else
                {
                    OPTIMIZATION_ERROR_AT(expression, "Unknown function: " + call->getFullyQualifiedName());
                }
            }
            return type;
        }
        break;
        case ExpressionType::MethodCall:
        {
            shared_ptr<MethodCallNode> call = dynamic_pointer_cast<MethodCallNode>(expression);

            // Get the type of the object the method is being called on
            string objectType = getTypeForExpression(call->getObject(), symbols);

            // Check if this is actually a namespace call (object is an identifier matching a namespace)
            shared_ptr<IdentifierNode> objIdent = dynamic_pointer_cast<IdentifierNode>(call->getObject());
            if (objIdent != nullptr)
            {
                string nsKey = "namespace:" + objIdent->getValue();
                if (!symbols.get(nsKey).empty())
                {
                    // It's a namespace call, look up "Namespace.funcName()"
                    string funcKey = objIdent->getValue() + "." + call->getMethodName() + "()";
                    string type = symbols.get(funcKey);
                    if (type.empty())
                    {
                        // Check for local function
                        string localFuncKey = "local:" + funcKey;
                        string localType = symbols.get(localFuncKey);
                        if (!localType.empty())
                        {
                            OPTIMIZATION_ERROR_AT(expression, "Function " + objIdent->getValue() + "." + call->getMethodName() +
                                " is local and can only be called from within its namespace");
                        }
                        else
                        {
                            OPTIMIZATION_ERROR_AT(expression, "Unknown function: " + objIdent->getValue() + "." + call->getMethodName());
                        }
                    }
                    return type;
                }
            }

            // It's a method call on an object, look up "method:ClassName.methodName()"
            string methodKey = "method:" + objectType + "." + call->getMethodName() + "()";
            string type = symbols.get(methodKey);
            if (type.empty())
            {
                OPTIMIZATION_ERROR_AT(expression, "Unknown method: " + call->getMethodName() + " on type " + objectType);
            }
            return type;
        }
        break;
        default:
        {
            OPTIMIZATION_ERROR_AT(expression, "Unknown type in getTypeForExpression");
        }
        break;
        }
    }

    void TypeInferencePass::performPass(shared_ptr<BlockNode> block, SymbolTable<string, string> &symbols)
    {
        if (block == nullptr)
        {
            return;
        }

        map<string, shared_ptr<DeclarationNode>> untypedNodes;
        vector<shared_ptr<Expression>> &expressions = block->getExpressions();
        for (int i = 0; i < expressions.size(); ++i)
        {
            shared_ptr<Expression> current = expressions[i];
            switch (current->getExpressionType())
            {
            case ExpressionType::Declaration:
            {
                shared_ptr<DeclarationNode> decl = dynamic_pointer_cast<DeclarationNode>(current);

                if (symbols.contains(decl->getName()))
                {
                    OPTIMIZATION_ERROR_AT(current, "variable " + decl->getName() + " already declared");
                }

                if (decl->getTypeName() == "")
                {
                    // Check if declaration has an initializer expression
                    shared_ptr<Expression> initExpr = decl->getExpression();
                    if (initExpr != nullptr)
                    {
                        // Infer type from the initializer expression
                        string inferredType = getTypeForExpression(initExpr, symbols);
                        LOG("Type inference: %s -> %s\n", decl->getName().c_str(), inferredType.c_str());
                        decl->setTypeName(inferredType);
                        symbols.put(decl->getName(), inferredType);
                    }
                    else
                    {
                        // No initializer, wait for a later assignment to infer type
                        untypedNodes.insert({decl->getName(), decl});
                    }
                }
                else
                {
                    symbols.put(decl->getName(), decl->getTypeName());
                }
            }
            break;
            case ExpressionType::BinaryOperator:
            {
                shared_ptr<BinaryExpressionNode> expr = dynamic_pointer_cast<BinaryExpressionNode>(current);
                if (expr->getOperator() == "=")
                {
                    string rhsType = getTypeForExpression(expr->getRhs(), symbols);
                    if (expr->getLhs()->getExpressionType() == ExpressionType::Identifier)
                    {
                        shared_ptr<IdentifierNode> ident = dynamic_pointer_cast<IdentifierNode>(expr->getLhs());
                        auto it = untypedNodes.find(ident->getValue());
                        if (it != untypedNodes.end())
                        {
                            it->second->setTypeName(rhsType);
                            symbols.put(ident->getValue(), rhsType);
                        }
                    }

                    string lhsType = getTypeForExpression(expr->getLhs(), symbols);
                    if (lhsType != rhsType)
                    {
                        stringstream error;
                        error << "Cannot assign type " << rhsType << " to variable of type " << lhsType;
                        OPTIMIZATION_ERROR_AT(current, error.str());
                    }
                }
            }
            break;
            default:
                continue;
            }
        }
    }
}
