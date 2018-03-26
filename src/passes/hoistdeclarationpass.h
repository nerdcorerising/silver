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
    private:
        void performPassOnBlock(std::shared_ptr<ast::BlockNode> block);

    public:
        HoistDeclarationPass() = default;
        virtual ~HoistDeclarationPass() = default;

        virtual void performPass(std::shared_ptr<ast::Assembly> assembly) override;
    };
}
