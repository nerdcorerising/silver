
#include "analysispass.h"
#include "hoistdeclarationpass.h"
#include "typeinferencepass.h"
#include "referencecounting.h"

using namespace std;
using namespace ast;

namespace analysis
{    
    AnalysisPassManager::AnalysisPassManager(BuildType type) :
        mPasses()
    {
        mPasses.push_back(shared_ptr<Pass>(new HoistDeclarationPass()));
        mPasses.push_back(shared_ptr<Pass>(new TypeInferencePass()));
        mPasses.push_back(shared_ptr<Pass>(new ReferenceCountingPass()));

        if (type == BuildType::Debug)
        {
            // Debug specific passes
        }
        else // Release
        {
            // Release specific passes
        }
    }

    void AnalysisPassManager::performPassOnBlock(shared_ptr<BlockNode> block, SymbolTable<string, string> &symbols)
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
                if (subBlock != nullptr)
                {
                    performPassOnBlock(subBlock, symbols);
                }
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

    void AnalysisPassManager::defineFunctions(shared_ptr<Assembly> assembly, SymbolTable<string, string> &symbols)
    {
        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            string name = (*func)->getName() + "()";
            symbols.put(name, (*func)->getReturnType());
        }
    }

    void AnalysisPassManager::defineClasses(shared_ptr<Assembly> assembly, SymbolTable<string, string> &symbols)
    {
        vector<shared_ptr<ClassDeclaration>> classes = assembly->getClasses();
        for (auto cls = classes.begin(); cls != classes.end(); ++cls)
        {
            string className = (*cls)->getName();
            // Register the class itself
            symbols.put("class:" + className, className);

            // Register each field as "ClassName.fieldName" -> fieldType
            vector<shared_ptr<Field>> fields = (*cls)->getFields();
            for (auto field = fields.begin(); field != fields.end(); ++field)
            {
                string fieldKey = className + "." + (*field)->getName();
                symbols.put(fieldKey, (*field)->getType());
            }
        }
    }

    void AnalysisPassManager::performPasses(shared_ptr<Assembly> assembly)
    {
        SymbolTable<string, string> symbols;
        defineFunctions(assembly, symbols);
        defineClasses(assembly, symbols);

        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            shared_ptr<BlockNode> block = (*func)->getBlock();
            performPassOnBlock(block, symbols);
        }
    }
}