
#include "optimizationpass.h"

using namespace std;
using namespace ast;

namespace optimization
{    
    OptimizationPassManager::OptimizationPassManager(BuildType type) :
        mPasses()
    {
        throw "add passes here?";

        if (type == BuildType::Debug)
        {
            // Debug specific passes
        }
        else // Release
        {
            // Release specific passes
        }
    }

    void OptimizationPassManager::performPasses(std::shared_ptr<Assembly> assembly)
    {
        for (auto it = mPasses.begin(); it != mPasses.end(); ++it)
        {
            (*it)->performPass(assembly);
        }
    }
}