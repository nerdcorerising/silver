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

    bool tryGet(key name, value &out)
    {
        bool found = false;
        auto it = mCurrent.find(name);
        if (it == mCurrent.end())
        {
            for (auto revIt = mStack.rbegin(); revIt != mStack.rend(); ++revIt)
            {
                it = (*revIt).find(name);
                if (it != (*revIt).end())
                {
                    found = true;
                    break;
                }
            }
        }
        else
        {
            found = true;
        }

        if (found)
        {
            out = it->second;
        }
        else
        {
            out = value();
        }

        return found;
    }

    value get(key name)
    {
        value v;
        if (!tryGet(name, v))
        {
            return value();
        }

        return v;
    }

    bool contains(key name)
    {
        value v;
        return tryGet(name, v);
    }

    bool containsInCurrentScope(key name)
    {
        return mCurrent.find(name) != mCurrent.end();
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
