
#include "referencecounting.h"

using std::string;
using std::shared_ptr;

namespace analysis
{
    void ReferenceCountingPass::performPass(shared_ptr<ast::BlockNode> block, SymbolTable<string, string> &symbols)
    {
        // TODO: for any user defined type that gets created, add reference increase when it's allocated or assigned
        // and decrease when it goes out of scope
        return;
    }
}