#pragma once

#include <vector>
#include <map>

template<typename key, typename value>
class SymbolTable
{
private:
    std::vector<std::map<key, value>> mStack;
    std::map<key, value> mCurrent;

public:
    SymbolTable():
        mStack(),
        mCurrent()
    {

    }

    void put(key name, value inst)
    {
        mCurrent.insert({name, inst});
    }

    value get(key name)
    {
        auto it = mCurrent.find(name);
        if (it == mCurrent.end())
        {
            bool found = false;
            for (auto revIt = mStack.rbegin(); revIt != mStack.rend(); ++revIt)
            {
                it = (*revIt).find(name);
                if (it != (*revIt).end())
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                throw "uh oh!";
                // return value();
            }
        }

        return (*it).second;
    }


    void enterContext()
    {
        mStack.push_back(mCurrent);
        mCurrent.clear();
    }

    void leaveContext()
    {
        mCurrent = mStack.at(mStack.size() - 1);
        mStack.pop_back();
    }
};
