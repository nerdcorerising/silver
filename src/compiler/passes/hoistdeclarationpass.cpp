
#include "hoistdeclarationpass.h"

using namespace std;
using namespace ast;

namespace analysis
{
    void HoistDeclarationPass::performPass(shared_ptr<BlockNode> block, SymbolTable<string, string> &symbols)
    {
        if (block == nullptr)
        {
            return;
        }

        vector<shared_ptr<Expression>> &expressions = block->getExpressions();
        for (int i = 0; i < expressions.size(); ++i)
        {
            shared_ptr<Expression> current = expressions[i];
            if (current->getExpressionType() == ExpressionType::Declaration)
            {
                shared_ptr<DeclarationNode> declaration = dynamic_pointer_cast<DeclarationNode>(current);
                if (declaration->getTypeName() == "" && declaration->getExpression() != nullptr)
                {
                    shared_ptr<Expression> identifier = shared_ptr<Expression>(new IdentifierNode(declaration->getName()));
                    shared_ptr<Expression> assignment = shared_ptr<Expression>(new BinaryExpressionNode(identifier, declaration->getExpression(), "="));

                    // Clear the expression from declaration since we've extracted it into a separate assignment
                    declaration->clearExpression();

                    expressions.insert(expressions.begin() + (i + 1), assignment);
                }
            }
        }
    }
}
