
#include "tokenmanager.h"

using namespace std;
using namespace tok;

namespace parse
{
    TokenManager::TokenManager(Tokenizer tok, istream &in) :
        mTokenizer(tok),
        mInput(in),
        mCurrent(),
        mLookAheads(),
        mGood(true)
    {
        advance();
    }

    bool TokenManager::hasInput()
    {
        return mInput.good();
    }

    bool TokenManager::getNextToken(Token &token)
    {
        while (!mTokenizer.ready())
        {
            int rawCh = mInput.get();
            if (!mInput.good())
            {
                break;
            }

            char ch = (char)rawCh;
            mTokenizer.insert(ch);
        }

        if (!mTokenizer.ready())
        {
            token = mTokenizer.flush();
        }
        else
        {
            token = mTokenizer.dispense();
        }

        return (token.type() != TokenType::Error) || mInput.good();
    }

    bool TokenManager::getNextNonWhitespaceToken(Token &token)
    {
        do
        {
            if (!getNextToken(token))
            {
                return false;
            }
        } while (token.type() == TokenType::WhiteSpace || token.type() == TokenType::Comment);

        return true;
    }

    bool TokenManager::preloadLookAheads(size_t count)
    {
        while (mLookAheads.size() < count)
        {
            Token tok;
            if (!getNextNonWhitespaceToken(tok))
            {
                return false;
            }

            mLookAheads.push_back(tok);
        }

        return true;
    }

    void TokenManager::advance()
    {
        if (mLookAheads.size() > 0)
        {
            mCurrent = mLookAheads.front();
            mLookAheads.pop_front();
        }
        else
        {
            if (!getNextNonWhitespaceToken(mCurrent))
            {
                mGood = false;
            }
        }
    }

    void TokenManager::advanceBy(size_t count)
    {
        preloadLookAheads(count);

        for (size_t i = 0; i < count; ++i)
        {
            advance();
        }
    }

    bool TokenManager::tryGetCurrent(Token &token)
    {
        if (mGood)
        {
            token = mCurrent;
            return true;
        }

        // TokenType::Error
        token = Token();
        return false;
    }

    bool TokenManager::tryGetLookAhead(Token &token)
    {
        return tryGetLookAheadBy(0, token);
    }

    bool TokenManager::tryGetLookAheadBy(size_t pos, Token &token)
    {
        size_t sz = pos + 1;

        if (mLookAheads.size() < sz)
        {
            if (!preloadLookAheads(sz))
            {
                // TokenType::Error
                token = Token();
                return false;
            }
        }

        token = mLookAheads[pos];
        return true;
    }
}