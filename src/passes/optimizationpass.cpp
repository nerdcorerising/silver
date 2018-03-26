
#include "optimizationpass.h"
#include "hoistdeclarationpass.h"

using namespace std;
using namespace ast;

namespace optimization
{    
    OptimizationPassManager::OptimizationPassManager(BuildType type) :
        mPasses()
    {
        mPasses.push_back(shared_ptr<Pass>(new HoistDeclarationPass()));

        if (type == BuildType::Debug)
        {
            // Debug specific passes
        }
        else // Release
        {
            // Release specific passes
        }
    }

    void OptimizationPassManager::performPasses(shared_ptr<Assembly> assembly)
    {
        for (auto it = mPasses.begin(); it != mPasses.end(); ++it)
        {
            (*it)->performPass(assembly);
        }
    }
}