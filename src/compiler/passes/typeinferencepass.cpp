
#include "typeinferencepass.h"
#include <map>
#include <sstream>

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
            OPTIMIZATION_ERROR("Cannot assign result of If to variable");
        }
        break;
        case ExpressionType::While:
        {
            OPTIMIZATION_ERROR("Cannot assign result of loop to variable");
        }
        break;
        case ExpressionType::Block:
        {
            OPTIMIZATION_ERROR("Cannot assign result of block to variable");
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
            throw "not implemented";
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
                error << "Types " << lhsType << " and " << lhsType << " do not match" << endl;
                OPTIMIZATION_ERROR(error.str());
            }

            return lhsType;
        }
        break;
        case ExpressionType::Empty:
        {
            OPTIMIZATION_ERROR("Cannot assign result of empty statement to variable");
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
            OPTIMIZATION_ERROR("Cannot assign result of declaration statement to variable");
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
            OPTIMIZATION_ERROR("Cannot assign result of return statement to variable");
        }
        break;
        case ExpressionType::Cast:
        {
            shared_ptr<CastNode> cast = dynamic_pointer_cast<CastNode>(expression);
            return cast->getCastType();
        }
        break;
        default:
        {
            OPTIMIZATION_ERROR("Unknown type");
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
                    OPTIMIZATION_ERROR("variable " + decl->getName() + " already declared");
                }

                if (decl->getTypeName() == "")
                {
                    // Check if declaration has an initializer expression
                    shared_ptr<Expression> initExpr = decl->getExpression();
                    if (initExpr != nullptr)
                    {
                        // Infer type from the initializer expression
                        string inferredType = getTypeForExpression(initExpr, symbols);
                        fprintf(stderr, "Type inference: %s -> %s\n", decl->getName().c_str(), inferredType.c_str());
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
                        error << "Cannot assign type " << rhsType << "to variable of type " << lhsType << endl;
                        OPTIMIZATION_ERROR(error.str());
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
