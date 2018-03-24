
#pragma once


#include <iostream>
#include <memory>
#include <deque>

#include "common.h"
#include "tokenizer.h"
#include "ast.h"

#define PARSE_ERROR(MSG) std::cout << MSG; throw "parse error" //; waitForKey(); exit(-1)

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

        tok::Token getNextToken();
        tok::Token getNextNonWhitespaceToken();
        void preloadLookAheads(size_t count);

    public:
        TokenManager(tok::Tokenizer tok, std::istream &in);

        void advance();
        void advanceBy(size_t count);
        tok::Token current();
        tok::Token lookAhead();
        tok::Token lookAheadBy(size_t pos);
        bool hasInput();
    };

    class Parser
    {
    private:
        TokenManager mTokens;
        tok::Token lookAhead;
        std::string mName;

        std::vector<std::shared_ptr<ast::Argument>> parseArguments();
        std::shared_ptr<ast::Function> parseFunction();
        std::shared_ptr<ast::Block> parseBlock();

        std::shared_ptr<ast::Expression> parseWhile();
        std::shared_ptr<ast::Expression> parseStatementHelper(std::shared_ptr<ast::Expression> curr, int minPrecedence);
        std::shared_ptr<ast::Expression> parseStatement();
        std::vector<std::shared_ptr<ast::Expression>> parseFunctionArgs();
        std::shared_ptr<ast::Expression> parseExpression();

        std::shared_ptr<ast::Expression> makeNode();

        int operatorPrecedence(tok::Token op);
    public:
        Parser(std::string, tok::Tokenizer tok, std::istream &in);

        std::shared_ptr<ast::Assembly> parse();
    };
}
