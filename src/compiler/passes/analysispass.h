#pragma once

#include <memory>
#include <vector>
#include <string>

#include "common.h"
#include "symboltable.h"
#include "ast/ast.h"

#define OPTIMIZATION_ERROR(MSG) do { \
    std::cerr << "Error: " << MSG << std::endl; \
    throw std::string("optimization error"); \
} while(0)

#define OPTIMIZATION_ERROR_AT(EXPR, MSG) do { \
    if ((EXPR)->line() > 0) { \
        std::cerr << "Error at line " << (EXPR)->line() << ", column " << (EXPR)->column() << ": " << MSG << std::endl; \
    } else { \
        std::cerr << "Error: " << MSG << std::endl; \
    } \
    throw std::string("optimization error"); \
} while(0)

// Also need to make a fill out symbol table pass

namespace analysis
{
    // TODO passes
    // Type checking pass (assignments, function calls, etc)
    // Maybe ARC pass? Probably ARC pass!

    enum BuildType
    {
        Debug,
        Release
    };

    class Pass
    {
    public:
        Pass() = default;
        virtual ~Pass() = default;

        virtual void performPass(std::shared_ptr<ast::BlockNode> block, SymbolTable<std::string, std::string> &symbols) = 0;
    };

    class AnalysisPassManager
    {
    private:
        std::vector<std::shared_ptr<Pass>> mPasses;

        void performPassOnBlock(std::shared_ptr<ast::BlockNode> block, SymbolTable<std::string, std::string> &symbols);
        void defineFunctions(std::shared_ptr<ast::Assembly> assembly, SymbolTable<std::string, std::string> &symbols);
        void defineClasses(std::shared_ptr<ast::Assembly> assembly, SymbolTable<std::string, std::string> &symbols);
        void defineNamespaces(std::shared_ptr<ast::Assembly> assembly, SymbolTable<std::string, std::string> &symbols);
        void defineNamespaceContents(std::shared_ptr<ast::NamespaceDeclaration> ns, SymbolTable<std::string, std::string> &symbols, std::string parentPath);
        void performPassesOnNamespace(std::shared_ptr<ast::NamespaceDeclaration> ns, SymbolTable<std::string, std::string> &symbols, std::string parentPath);

    public:
        AnalysisPassManager(BuildType type);

        void performPasses(std::shared_ptr<ast::Assembly> assembly);
    };
}
