#pragma once


#include <memory>
#include <vector>
#include <string>

#include "common.h"
#include "analysispass.h"
#include "ast/ast.h"

namespace analysis
{
    class TypeInferencePass : public Pass
    {
    private:
        std::string getTypeForExpression(std::shared_ptr<ast::Expression> expression, SymbolTable<std::string, std::string> &symbols);

    public:
        TypeInferencePass() = default;
        virtual ~TypeInferencePass() = default;

        virtual void performPass(std::shared_ptr<ast::BlockNode> block, SymbolTable<std::string, std::string> &symbols) override;
    };
}
