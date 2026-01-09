#pragma once

#include <memory>
#include <vector>
#include <string>

#include "common.h"
#include "symboltable.h"
#include "ast/ast.h"

#define OPTIMIZATION_ERROR(MSG) std::cout << MSG; throw "optimization error"

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

    public:
        AnalysisPassManager(BuildType type);

        void performPasses(std::shared_ptr<ast::Assembly> assembly);
    };
}
