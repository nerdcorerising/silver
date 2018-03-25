
#pragma once


#include <memory>

#include "common.h"
#include "tokenizer.h"
#include "tokenmanager.h"
#include "ast.h"

#define PARSE_ERROR(MSG) std::cout << MSG; throw "parse error" //; waitForKey(); exit(-1)

namespace parse
{
    class Parser
    {
    private:
        TokenManager mTokens;

        std::string mName;

        std::vector<std::shared_ptr<ast::Argument>> parseArgumentsForDeclaration();
        std::shared_ptr<ast::Function> parseFunction();
        std::shared_ptr<ast::Block> parseBlock();

        std::shared_ptr<ast::Expression> parseIfWhileCondition();
        std::shared_ptr<ast::Expression> parseWhile();
        std::shared_ptr<ast::Expression> parseIf();
        std::shared_ptr<ast::Expression> parseLet();
        std::shared_ptr<ast::Expression> parseStatementHelper(std::shared_ptr<ast::Expression> curr, int minPrecedence);
        std::shared_ptr<ast::Expression> parseStatement();
        std::vector<std::shared_ptr<ast::Expression>> parseFunctionArgs();
        std::shared_ptr<ast::Expression> parseExpression();

        std::shared_ptr<ast::Expression> makeNode();

        void expectCurrentTokenType(tok::TokenType type, std::string message);
        void expectCurrentTokenText(std::string text, std::string message);
        void expectCurrentTokenTypeAndText(tok::TokenType type, std::string text, std::string message);

        void expectTokenType(tok::Token token, tok::TokenType type, std::string message);
        void expectTokenText(tok::Token token, std::string text, std::string message);
        void expectTokenTypeAndText(tok::Token token, tok::TokenType type, std::string text, std::string message);

        int operatorPrecedence(tok::Token op);

        void advance();
        tok::Token current();
        tok::Token lookAhead();
        tok::Token lookAheadBy(size_t pos);
    public:
        Parser(std::string, tok::Tokenizer tok, std::istream &in);

        std::shared_ptr<ast::Assembly> parse();
    };
}
