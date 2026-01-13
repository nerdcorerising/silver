
#include "analysispass.h"
#include "hoistdeclarationpass.h"
#include "typeinferencepass.h"

using namespace std;
using namespace ast;

namespace analysis
{    
    AnalysisPassManager::AnalysisPassManager(BuildType type) :
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
        // Register built-in functions and their parameter types
        symbols.put("strlen_utf8()", "int");
        symbols.put("funcargs:strlen_utf8", "string");
        symbols.put("string_bytes()", "int");
        symbols.put("funcargs:string_bytes", "string");
        symbols.put("print()", "void");
        symbols.put("funcargs:print", "string");
        symbols.put("print_string()", "void");
        symbols.put("funcargs:print_string", "string");
        symbols.put("print_int()", "void");
        symbols.put("funcargs:print_int", "int");
        symbols.put("print_float()", "void");
        symbols.put("funcargs:print_float", "float");
        symbols.put("strcmp()", "int");
        symbols.put("funcargs:strcmp", "string,string");
        symbols.put("refcount()", "int");
        // refcount accepts any reference type, so we don't register specific arg types

        // Register user-defined functions
        // Also register "funcargs:functionName" -> comma-separated parameter types
        vector<shared_ptr<Function>> functions = assembly->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            string name = (*func)->getName() + "()";
            symbols.put(name, (*func)->getReturnType());

            // Store parameter types for argument validation
            string argsKey = "funcargs:" + (*func)->getName();
            string argTypes = "";
            vector<shared_ptr<Argument>> args = (*func)->getArguments();
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0) argTypes += ",";
                argTypes += args[i]->getType();
            }
            symbols.put(argsKey, argTypes);
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
            // Also register "fieldvis:ClassName.fieldName" -> "public" or "private"
            vector<shared_ptr<Field>> fields = (*cls)->getFields();
            for (auto field = fields.begin(); field != fields.end(); ++field)
            {
                string fieldKey = className + "." + (*field)->getName();
                symbols.put(fieldKey, (*field)->getType());

                // Store visibility for access control
                string visKey = "fieldvis:" + className + "." + (*field)->getName();
                string visibility = (*field)->getVisibility() == Visibility::Public ? "public" : "private";
                symbols.put(visKey, visibility);
            }

            // Register each method as "method:ClassName.methodName()" -> returnType
            // Also register "methodargs:ClassName.methodName" -> comma-separated parameter types
            // Also register "methodvis:ClassName.methodName" -> "public" or "private"
            vector<shared_ptr<Function>> methods = (*cls)->getMethods();
            for (auto method = methods.begin(); method != methods.end(); ++method)
            {
                string methodKey = "method:" + className + "." + (*method)->getName() + "()";
                symbols.put(methodKey, (*method)->getReturnType());

                // Store parameter types for argument validation
                string argsKey = "methodargs:" + className + "." + (*method)->getName();
                string argTypes = "";
                vector<shared_ptr<Argument>> args = (*method)->getArguments();
                for (size_t i = 0; i < args.size(); ++i)
                {
                    if (i > 0) argTypes += ",";
                    argTypes += args[i]->getType();
                }
                symbols.put(argsKey, argTypes);

                // Store visibility for access control
                string visKey = "methodvis:" + className + "." + (*method)->getName();
                string visibility = (*method)->getVisibility() == Visibility::Public ? "public" : "private";
                symbols.put(visKey, visibility);
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

        // Register functions with qualified names
        // Also register their parameter types for argument validation
        vector<shared_ptr<Function>> functions = ns->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            string qualifiedName = fullPath + "." + (*func)->getName() + "()";
            if ((*func)->isLocal())
            {
                // Register local functions with a "local:" prefix so we can check for them
                // and give a better error message when they're called from outside
                symbols.put("local:" + qualifiedName, (*func)->getReturnType());
            }
            else
            {
                symbols.put(qualifiedName, (*func)->getReturnType());
            }

            // Store parameter types for argument validation
            string argsKey = "funcargs:" + fullPath + "." + (*func)->getName();
            string argTypes = "";
            vector<shared_ptr<Argument>> args = (*func)->getArguments();
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0) argTypes += ",";
                argTypes += args[i]->getType();
            }
            symbols.put(argsKey, argTypes);
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
        // Also register their argument types for validation
        vector<shared_ptr<Function>> functions = ns->getFunctions();
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            string shortName = (*func)->getName() + "()";
            symbols.put(shortName, (*func)->getReturnType());

            // Store short argument types for validation within namespace
            string shortArgsKey = "funcargs:" + (*func)->getName();
            string argTypes = "";
            vector<shared_ptr<Argument>> args = (*func)->getArguments();
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0) argTypes += ",";
                argTypes += args[i]->getType();
            }
            symbols.put(shortArgsKey, argTypes);
        }

        // Process functions in this namespace
        for (auto func = functions.begin(); func != functions.end(); ++func)
        {
            // Enter a new context for this function with parameters defined
            symbols.enterContext();

            // Register function parameters
            vector<shared_ptr<Argument>> args = (*func)->getArguments();
            for (auto arg = args.begin(); arg != args.end(); ++arg)
            {
                symbols.put((*arg)->getName(), (*arg)->getType());
            }

            shared_ptr<BlockNode> block = (*func)->getBlock();
            performPassOnBlock(block, symbols);

            symbols.leaveContext();
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
            // Enter a new context for this function with parameters defined
            symbols.enterContext();

            // Register function parameters
            vector<shared_ptr<Argument>> args = (*func)->getArguments();
            for (auto arg = args.begin(); arg != args.end(); ++arg)
            {
                symbols.put((*arg)->getName(), (*arg)->getType());
            }

            shared_ptr<BlockNode> block = (*func)->getBlock();
            performPassOnBlock(block, symbols);

            symbols.leaveContext();
        }

        // Process class methods
        vector<shared_ptr<ClassDeclaration>> classes = assembly->getClasses();
        for (auto cls = classes.begin(); cls != classes.end(); ++cls)
        {
            string className = (*cls)->getName();
            vector<shared_ptr<Function>> methods = (*cls)->getMethods();
            for (auto method = methods.begin(); method != methods.end(); ++method)
            {
                // Enter a new context for this method with 'this' and parameters defined
                symbols.enterContext();
                symbols.put("this", className);

                // Register method parameters
                vector<shared_ptr<Argument>> args = (*method)->getArguments();
                for (auto arg = args.begin(); arg != args.end(); ++arg)
                {
                    symbols.put((*arg)->getName(), (*arg)->getType());
                }

                shared_ptr<BlockNode> block = (*method)->getBlock();
                performPassOnBlock(block, symbols);

                symbols.leaveContext();
            }
        }

        // Process namespace functions
        vector<shared_ptr<NamespaceDeclaration>> namespaces = assembly->getNamespaces();
        for (auto ns = namespaces.begin(); ns != namespaces.end(); ++ns)
        {
            performPassesOnNamespace(*ns, symbols, "");
        }
    }
}