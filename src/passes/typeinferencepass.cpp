
#include "typeinferencepass.h"
#include <map>

using namespace std;
using namespace ast;

namespace optimization
{
    string TypeInferencePass::getTypeForExpression(shared_ptr<Expression> expression, SymbolTable<string, string> &symbols)
    {
        switch (expression->getExpressionType())
        {
        case ExpressionType::If:
        case ExpressionType::ElseIf:
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
        case IntegerLiteral:
        {
            return "int";
        }
        break;
        case FloatLiteral:
        {
            return "float";
        }
        break;
        case StringLiteral:
        {
            return "string";
        }
        break;
        case UnaryOperator:
        {
            throw "not implemented";
            // return ;
        }
        break;
        case BinaryOperator:
        {
            throw "not implemented";
            // return ;
        }
        break;
        case Empty:
        {
            OPTIMIZATION_ERROR("Cannot assign result of empty statement to variable");
        }
        break;
        case Identifier:
        {
            shared_ptr<IdentifierNode> identifier = dynamic_pointer_cast<IdentifierNode>(expression);
            string type = symbols.get(identifier->getValue());
            return type;
        }
        break;
        case Declaration:
        {
            OPTIMIZATION_ERROR("Cannot assign result of declaration statement to variable");
        }
        break;
        case FunctionCall:
        {
            shared_ptr<FunctionCallNode> call = dynamic_pointer_cast<FunctionCallNode>(expression);
            string funcName = call->getName() + "()";
            string type = symbols.get(funcName);
            return type;
        }
        break;
        case Return:
        {
            OPTIMIZATION_ERROR("Cannot assign result of return statement to variable");
        }
        break;
        case Cast:
        {
            throw "not implemented";
            // return ;
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
                if (decl->getTypeName() == "")
                {
                    untypedNodes.insert({decl->getName(), decl});
                }
            }
            break;
            case ExpressionType::BinaryOperator:
            {
                shared_ptr<BinaryExpressionNode> expr = dynamic_pointer_cast<BinaryExpressionNode>(current);
                if (expr->getOperator() == "=" && expr->getLhs()->getExpressionType() == ExpressionType::Identifier)
                {
                    shared_ptr<IdentifierNode> ident = dynamic_pointer_cast<IdentifierNode>(expr->getLhs());
                    auto it = untypedNodes.find(ident->getValue());
                    if (it != untypedNodes.end())
                    {
                        string observedType = getTypeForExpression(expr->getRhs(), symbols);
                        it->second->setTypeName(observedType);
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
