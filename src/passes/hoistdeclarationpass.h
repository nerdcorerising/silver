#pragma once


#include <memory>
#include <vector>
#include <string>

#include "common.h"
#include "optimizationpass.h"
#include "ast/ast.h"

namespace optimization
{
    class HoistDeclarationPass : public Pass
    {
    public:
        HoistDeclarationPass() = default;
        virtual ~HoistDeclarationPass() = default;

        virtual void performPass(std::shared_ptr<ast::BlockNode> block, SymbolTable<std::string, std::string> &symbols) override;
    };
}
