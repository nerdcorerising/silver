
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
        for (auto it = expressions.begin(); it != expressions.end(); ++it)
        {
            switch ((*it)->getExpressionType())
            {
            case ExpressionType::If:
            {
                shared_ptr<IfNode> ifNode = dynamic_pointer_cast<IfNode>(*it);
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
                shared_ptr<WhileNode> whileNode = dynamic_pointer_cast<WhileNode>(*it);
                shared_ptr<BlockNode> whileBlock = whileNode->getBlock();
                performPassOnBlock(whileBlock);
            }
            break;
            case ExpressionType::Block:
            {
                shared_ptr<BlockNode> subBlock = dynamic_pointer_cast<BlockNode>(*it);
                performPassOnBlock(subBlock);
            }
            break;
            case ExpressionType::Declaration:
            {
                // Need to make the declaration with an assignment be two expressions,
                // first a declaration by itself with no type and then an assignment
                throw "not implemented";
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
