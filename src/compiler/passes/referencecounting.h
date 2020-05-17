
#pragma once


#include <memory>
#include <vector>
#include <string>

#include "common.h"
#include "analysispass.h"
#include "ast/ast.h"

namespace analysis
{
    class ReferenceCountingPass : public Pass
    {
    public:
        ReferenceCountingPass() = default;
        virtual ~ReferenceCountingPass() = default;

        virtual void performPass(std::shared_ptr<ast::BlockNode> block, SymbolTable<std::string, std::string> &symbols) override;
    };
}
