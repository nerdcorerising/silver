#pragma once

#include <memory>
#include <vector>
#include <string>

#include "common.h"
#include "ast/ast.h"

#define OPTIMIZATION_ERROR(MSG) std::cout << MSG; throw "optimization error"

namespace optimization
{
    // TODO passes
    // Change DeclarationNode with expression to declaration and then assignment
    // Type inference pass
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

        virtual void performPass(std::shared_ptr<ast::Assembly> assembly) = 0;
    };

    class OptimizationPassManager
    {
        std::vector<std::shared_ptr<Pass>> mPasses;

    public:
        OptimizationPassManager(BuildType type);

        void performPasses(std::shared_ptr<ast::Assembly> assembly);
    };
}
