
#pragma once


#include <memory>

#include "common.h"
#include "tokenizer.h"
#include "tokenmanager.h"
#include "ast/ast.h"

namespace parse
{
    class Parser
    {
    private:
        TokenManager mTokens;

        std::string mName;

        void reportFatalError(std::string message);
        void reportFatalError(std::string message, tok::Token token);

        std::vector<std::shared_ptr<ast::Argument>> parseArgumentsForDeclaration();
        std::vector<std::shared_ptr<ast::Function>> parseImport();
        std::shared_ptr<ast::Function> parseFunction(bool isLocal = false);
        std::shared_ptr<ast::ClassDeclaration> parseClass();
        std::shared_ptr<ast::NamespaceDeclaration> parseNamespace();
        std::shared_ptr<ast::Field> parseField();
        std::shared_ptr<ast::BlockNode> parseBlock();

        std::shared_ptr<ast::Expression> parseIfWhileCondition();
        std::shared_ptr<ast::Expression> parseWhile();
        std::shared_ptr<ast::Expression> parseIf();
        std::shared_ptr<ast::Expression> parseLet();
        std::shared_ptr<ast::Expression> parseStatementHelper(std::shared_ptr<ast::Expression> curr, int minPrecedence);
        std::shared_ptr<ast::Expression> parseStatement();
        std::vector<std::shared_ptr<ast::Expression>> parseFunctionArgs();
        std::shared_ptr<ast::Expression> parseExpression();

        std::shared_ptr<ast::Expression> makeNode();

        bool expectCurrentTokenType(tok::TokenType type, std::string message);
        bool expectCurrentTokenText(std::string text, std::string message);
        bool expectCurrentTokenTypeAndText(tok::TokenType type, std::string text, std::string message);

        bool expectTokenType(tok::Token token, tok::TokenType type, std::string message);
        bool expectTokenText(tok::Token token, std::string text, std::string message);
        bool expectTokenTypeAndText(tok::Token token, tok::TokenType type, std::string text, std::string message);

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
