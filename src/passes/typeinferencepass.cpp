
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
            throw "not implemented";
            // return ;
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
