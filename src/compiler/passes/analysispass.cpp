
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

    void AnalysisPassManager::defineNamespaces(shared_ptr<Assembly> assembly, SymbolTable<string, string> &symbols)
    {
        vector<shared_ptr<NamespaceDeclaration>> namespaces = assembly->getNamespaces();
        for (auto ns = namespaces.begin(); ns != namespaces.end(); ++ns)
        {
            defineNamespaceContents(*ns, symbols, "");
        }
    }

    void AnalysisPassManager::defineNamespaceContents(shared_ptr<NamespaceDeclaration> ns, SymbolTable<string, string> &symbols, string parentPath)
    {
        string fullPath = parentPath.empty() ? ns->getName() : parentPath + "." + ns->getName();

        // Register namespace exists
        symbols.put("namespace:" + fullPath, "namespace");

        // Register non-local functions with qualified names (local functions are only accessible within the namespace)
        vector<shared_ptr<Function>> functions = ns->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            if (!(*func)->isLocal())
            {
                string qualifiedName = fullPath + "." + (*func)->getName() + "()";
                symbols.put(qualifiedName, (*func)->getReturnType());
            }
        }

        // Register classes with qualified names
        vector<shared_ptr<ClassDeclaration>> classes = ns->getClasses();
        for (auto cls = classes.begin(); cls != classes.end(); ++cls)
        {
            string qualifiedClassName = fullPath + "." + (*cls)->getName();
            symbols.put("class:" + qualifiedClassName, qualifiedClassName);

            // Register fields
            vector<shared_ptr<Field>> fields = (*cls)->getFields();
            for (auto field = fields.begin(); field != fields.end(); ++field)
            {
                string fieldKey = qualifiedClassName + "." + (*field)->getName();
                symbols.put(fieldKey, (*field)->getType());
            }
        }

        // Recurse into nested namespaces
        vector<shared_ptr<NamespaceDeclaration>> nested = ns->getNestedNamespaces();
        for (auto nestedNs = nested.begin(); nestedNs != nested.end(); ++nestedNs)
        {
            defineNamespaceContents(*nestedNs, symbols, fullPath);
        }
    }

    void AnalysisPassManager::performPassesOnNamespace(shared_ptr<NamespaceDeclaration> ns, SymbolTable<string, string> &symbols, string parentPath)
    {
        string currentPath = parentPath.empty() ? ns->getName() : parentPath + "." + ns->getName();

        // Enter a new scope and register local function aliases
        symbols.enterContext();

        // Register short function names as aliases to qualified names
        vector<shared_ptr<Function>> functions = ns->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            string shortName = (*func)->getName() + "()";
            symbols.put(shortName, (*func)->getReturnType());
        }

        // Process functions in this namespace
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            shared_ptr<BlockNode> block = (*func)->getBlock();
            performPassOnBlock(block, symbols);
        }

        // Recurse into nested namespaces
        vector<shared_ptr<NamespaceDeclaration>> nested = ns->getNestedNamespaces();
        for (auto nestedNs = nested.begin(); nestedNs != nested.end(); ++nestedNs)
        {
            performPassesOnNamespace(*nestedNs, symbols, currentPath);
        }

        symbols.leaveContext();
    }

    void AnalysisPassManager::performPasses(shared_ptr<Assembly> assembly)
    {
        SymbolTable<string, string> symbols;
        defineFunctions(assembly, symbols);
        defineClasses(assembly, symbols);
        defineNamespaces(assembly, symbols);

        // Process top-level functions
        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            shared_ptr<BlockNode> block = (*func)->getBlock();
            performPassOnBlock(block, symbols);
        }

        // Process namespace functions
        vector<shared_ptr<NamespaceDeclaration>> namespaces = assembly->getNamespaces();
        for (auto ns = namespaces.begin(); ns != namespaces.end(); ++ns)
        {
            performPassesOnNamespace(*ns, symbols, "");
        }
    }
}