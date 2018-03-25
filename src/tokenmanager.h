#pragma once

#include <iostream>
#include <deque>
#include <memory>

#include "tokenizer.h"

namespace parse
{
    class TokenManager
    {
    private:
        tok::Tokenizer mTokenizer;
        std::istream &mInput;
        bool mGood;
        tok::Token mCurrent;
        std::deque<tok::Token> mLookAheads;

        bool getNextToken(tok::Token &token);
        bool getNextNonWhitespaceToken(tok::Token &token);
        bool preloadLookAheads(size_t count);

    public:
        TokenManager(tok::Tokenizer tok, std::istream &in);

        void advance();
        void advanceBy(size_t count);
        bool tryGetCurrent(tok::Token &token);
        bool tryGetLookAhead(tok::Token &token);
        bool tryGetLookAheadBy(size_t pos, tok::Token &token);
        bool hasInput();
    };
}