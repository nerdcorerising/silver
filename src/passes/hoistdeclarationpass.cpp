
#include "hoistdeclarationpass.h"

using namespace std;
using namespace ast;

namespace optimization
{
    void HoistDeclarationPass::performPassOnBlock(shared_ptr<BlockNode> block)
    {
        if (block == nullptr)
        {
            return;
        }

        vector<shared_ptr<Expression>> &expressions = block->getExpressions();
        for (int i = 0; i < expressions.size(); ++i)
        {
            shared_ptr<Expression> current = expressions[i];
            switch (current->getExpressionType())
            {
            case ExpressionType::If:
            {
                shared_ptr<IfNode> ifNode = dynamic_pointer_cast<IfNode>(current);
                shared_ptr<BlockNode> subBlock = ifNode->getIfBlock();
                performPassOnBlock(subBlock);

                for (auto elseIf = ifNode->getElseIfs().begin(); elseIf != ifNode->getElseIfs().end(); ++elseIf)
                {
                    subBlock = (*elseIf)->getBlock();
                    performPassOnBlock(subBlock);
                }

                subBlock = ifNode->getElseBlock();
                performPassOnBlock(subBlock);
            }
            break;
            case ExpressionType::While:
            {
                shared_ptr<WhileNode> whileNode = dynamic_pointer_cast<WhileNode>(current);
                shared_ptr<BlockNode> whileBlock = whileNode->getBlock();
                performPassOnBlock(whileBlock);
            }
            break;
            case ExpressionType::Block:
            {
                shared_ptr<BlockNode> subBlock = dynamic_pointer_cast<BlockNode>(current);
                performPassOnBlock(subBlock);
            }
            break;
            case ExpressionType::Declaration:
            {
                shared_ptr<DeclarationNode> declaration = dynamic_pointer_cast<DeclarationNode>(current);
                if (declaration->getTypeName() == "" && declaration->getExpression() != nullptr)
                {
                    shared_ptr<Expression> identifier = shared_ptr<Expression>(new IdentifierNode(declaration->getName()));
                    shared_ptr<Expression> assignment = shared_ptr<Expression>(new BinaryExpressionNode(identifier, declaration->getExpression(), "="));

                    expressions.insert(expressions.begin() + (i + 1), assignment);
                }
            }
            break;
            default:
                break;
            }
        }
    }

    void HoistDeclarationPass::performPass(shared_ptr<Assembly> assembly)
    {
        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto it = functions.begin(); it != functions.end(); ++it)
        {
            performPassOnBlock((*it)->getBlock());
        }
    }
}
