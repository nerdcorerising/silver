
#include "optimizationpass.h"
#include "hoistdeclarationpass.h"
#include "typeinferencepass.h"

using namespace std;
using namespace ast;

namespace optimization
{    
    OptimizationPassManager::OptimizationPassManager(BuildType type) :
        mPasses()
    {
        mPasses.push_back(shared_ptr<Pass>(new HoistDeclarationPass()));
        mPasses.push_back(shared_ptr<Pass>(new TypeInferencePass()));

        if (type == BuildType::Debug)
        {
            // Debug specific passes
        }
        else // Release
        {
            // Release specific passes
        }
    }

    void OptimizationPassManager::performPassOnBlock(shared_ptr<BlockNode> block, SymbolTable<string, string> &symbols)
    {
        symbols.enterContext();

        for (auto it = mPasses.begin(); it != mPasses.end(); ++it)
        {
            (*it)->performPass(block, symbols);
        }

        vector<shared_ptr<Expression>> expressions = block->getExpressions();
        for (auto current = expressions.begin(); current != expressions.end(); ++current)
        {
            switch ((*current)->getExpressionType())
            {
            case ExpressionType::IfBlock:
            {
                shared_ptr<IfBlockNode> ifNode = dynamic_pointer_cast<IfBlockNode>(*current);
                shared_ptr<BlockNode> subBlock;
                vector<shared_ptr<IfNode>> ifs = ifNode->getIfs();
                for (auto it = ifs.begin(); it != ifs.end(); ++it)
                {
                    subBlock = (*it)->getBlock();
                    performPassOnBlock(subBlock, symbols);
                }

                subBlock = ifNode->getElseBlock();
                performPassOnBlock(subBlock, symbols);
            }
            break;
            case ExpressionType::While:
            {
                shared_ptr<WhileNode> whileNode = dynamic_pointer_cast<WhileNode>(*current);
                shared_ptr<BlockNode> whileBlock = whileNode->getBlock();
                performPassOnBlock(whileBlock, symbols);
            }
            break;
            case ExpressionType::Block:
            {
                shared_ptr<BlockNode> subBlock = dynamic_pointer_cast<BlockNode>(*current);
                performPassOnBlock(subBlock, symbols);
            }
            break;
            default:
                continue;
            }
        }

        symbols.leaveContext();
    }

    void OptimizationPassManager::defineFunctions(std::shared_ptr<Assembly> assembly, SymbolTable<std::string, std::string> &symbols)
    {
        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            string name = (*func)->getName() + "()";
            symbols.put(name, (*func)->getReturnType());
        }   
    }

    void OptimizationPassManager::performPasses(shared_ptr<Assembly> assembly)
    {
        SymbolTable<string, string> symbols;
        defineFunctions(assembly, symbols);

        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            shared_ptr<BlockNode> block = (*func)->getBlock();
            performPassOnBlock(block, symbols);
        }
    }
}