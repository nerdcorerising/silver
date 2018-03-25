

#include "parser.h"

#include <stack>

using namespace std;
using namespace tok;
using namespace ast;

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

    Token TokenManager::getNextToken()
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

        Token tok;
        if (!mTokenizer.ready())
        {
            tok = mTokenizer.flush();
        }
        else
        {
            tok = mTokenizer.dispense();
        }

        return tok;
    }

    Token TokenManager::getNextNonWhitespaceToken()
    {
        Token tok = getNextToken();
        while (tok.type() == TokenType::WhiteSpace || tok.type() == TokenType::Comment)
        {
            tok = getNextToken();
        }

        return tok;
    }

    void TokenManager::preloadLookAheads(size_t count)
    {
        while (mLookAheads.size() < count)
        {
            Token tok = getNextNonWhitespaceToken();
            mLookAheads.push_back(tok);
        }
    }

    void TokenManager::advance()
    {
        if (!hasInput())
        {
            mGood = false;
            return;
        }

        if (mLookAheads.size() > 0)
        {
            mCurrent = mLookAheads.front();
            mLookAheads.pop_front();
        }
        else
        {
            mCurrent = getNextNonWhitespaceToken();
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

    Token TokenManager::current()
    {
        if (mGood)
        {
            return mCurrent;
        }

        PARSE_ERROR("Unexpected end of input.");
    }

    Token TokenManager::lookAhead()
    {
        return lookAheadBy(0);
    }

    Token TokenManager::lookAheadBy(size_t pos)
    {
        size_t sz = pos + 1;

        if (mLookAheads.size() < sz)
        {
            preloadLookAheads(sz);
        }

        return mLookAheads[pos];
    }

    Parser::Parser(string name, Tokenizer tok, istream &in) :
        mName(name),
        mTokens(tok, in)
    {

    }

    vector<shared_ptr<Argument>> Parser::parseArgumentsForDeclaration()
    {
        if (mTokens.current().type() != TokenType::OpenParens)
        {
            PARSE_ERROR("Unexpected token after function name");
        }

        mTokens.advance();

        vector<shared_ptr<Argument>> args;
        if (mTokens.current().type() == TokenType::CloseParens)
        {
            mTokens.advance();
            return args;
        }

        for (;;)
        {
            if (mTokens.current().type() != TokenType::Identifier)
            {
                PARSE_ERROR("Invalid name for function argument.");
            }

            string name = mTokens.current().text();
            mTokens.advance();

            if (mTokens.current().type() != TokenType::Colon)
            {
                PARSE_ERROR("Expected colon after function argument.");
            }

            mTokens.advance();


            if (mTokens.current().type() != TokenType::Identifier)
            {
                PARSE_ERROR("Type for function argument.");
            }

            string type = mTokens.current().text();
            mTokens.advance();

            args.push_back(shared_ptr<Argument>(new Argument(type, name)));

            TokenType curType = mTokens.current().type();
            if (curType == TokenType::CloseParens)
            {
                break;
            }
            else if (curType != TokenType::Comma)
            {
                PARSE_ERROR("Expected comma or close of parentheses in argument list");
            }

            // Skip comma
            mTokens.advance();
        }

        if (mTokens.current().type() != TokenType::CloseParens)
        {
            PARSE_ERROR("Expected closing parentheses.");
        }

        mTokens.advance();
        return args;
    }

    shared_ptr<Function> Parser::parseFunction()
    {
        if (mTokens.current().type() != TokenType::Keyword || mTokens.current().text() != "fn")
        {
            PARSE_ERROR("Missing function keyword");
        }

        mTokens.advance();

        if (mTokens.current().type() != TokenType::Identifier)
        {
            PARSE_ERROR("Invalid function name.");
        }

        string name = mTokens.current().text();
        mTokens.advance();

        vector<shared_ptr<Argument>> args = parseArgumentsForDeclaration();

        string returnType;
        if (mTokens.current().type() == TokenType::Operator && mTokens.current().text() == "->")
        {
            mTokens.advance();

            if (mTokens.current().type() != TokenType::Identifier)
            {
                PARSE_ERROR("Invalid return type");
            }

            // TODO: if multiple return types are wanted, need to implement here
            returnType = mTokens.current().text();
            mTokens.advance();
        }

        shared_ptr<Block> block = parseBlock();

        return shared_ptr<Function>(new Function(block, name, args, returnType));
    }

    shared_ptr<Block> Parser::parseBlock()
    {
        if (mTokens.current().type() != TokenType::LeftBrace)
        {
            PARSE_ERROR("Expected curly brace to open block");
        }

        mTokens.advance();
        vector<shared_ptr<Expression>> expressions;

        while (mTokens.current().type() != TokenType::RightBrace)
        {
            shared_ptr<Expression> exp = parseExpression();
            expressions.push_back(exp);
        }

        mTokens.advance();
        return shared_ptr<Block>(new Block(expressions));
    }

    vector<shared_ptr<Expression>> Parser::parseFunctionArgs()
    {
        if (mTokens.current().type() != TokenType::OpenParens)
        {
            PARSE_ERROR("Expected open parens to open argument list");
        }

        mTokens.advance();

        vector<shared_ptr<Expression>> args;
        for (;;)
        {
            shared_ptr<Expression> arg = makeNode();
            if (mTokens.current().type() != TokenType::Comma && mTokens.current().type() != TokenType::CloseParens)
            {
                arg = parseStatementHelper(arg, 0);
            }

            args.push_back(arg);

            if (mTokens.current().type() == TokenType::CloseParens)
            {
                break;
            }

            ASSERT(mTokens.current().type() == TokenType::Comma);
            mTokens.advance();
        }

        if (mTokens.current().type() != TokenType::CloseParens)
        {
            PARSE_ERROR("Expected close parens to close argument list");
        }

        mTokens.advance();
        return args;
    }

    shared_ptr<Expression> Parser::makeNode()
    {
        shared_ptr<Expression> node;
        double dVal;
        int iVal;

        switch (mTokens.current().type())
        {
        case TokenType::IntLiteral:
        {
            iVal = stoi(mTokens.current().text());
            node = shared_ptr<Expression>(new IntegerLiteralNode(iVal));
            mTokens.advance();
        }
            break;
        case TokenType::OpenParens:
        {
            mTokens.advance();

            if (mTokens.current().type() == TokenType::Identifier 
                && mTokens.lookAhead().type() == TokenType::CloseParens)
            {
                string type = mTokens.current().text();
                mTokens.advanceBy(2);
                shared_ptr<Expression> exp = makeNode();
                node = shared_ptr<Expression>(new CastNode(type, exp));
            }
            else
            {
                node = parseStatementHelper(makeNode(), 0);
                if (mTokens.current().type() != TokenType::CloseParens)
                {
                    PARSE_ERROR("Expected closing parens.");
                }

                mTokens.advance();
            }
        }
            break;
        case TokenType::StringLiteral:
        {
            node = shared_ptr<Expression>(new StringLiteralNode(mTokens.current().text()));
            mTokens.advance();
        }
            break;
        case TokenType::FloatLiteral:
        {
            dVal = stod(mTokens.current().text());
            node = shared_ptr<Expression>(new FloatLiteralNode(dVal));
            mTokens.advance();
        }
            break;
        case TokenType::Identifier:
        {
            if (mTokens.lookAhead().type() == TokenType::OpenParens)
            {
                string name = mTokens.current().text();
                mTokens.advance();
                vector<shared_ptr<Expression>> args = parseFunctionArgs();
                node = shared_ptr<Expression>(new FunctionCallNode(name, args));
            }
            else
            {
                node = shared_ptr<Expression>(new IdentifierNode(mTokens.current().text()));
                mTokens.advance();
            }
        }
            break;
        default:
        {
            INTERNAL_ERROR("Unknown token type in Parser::makeNode");
        }
        }

        return node;
    }

    int Parser::operatorPrecedence(Token op)
    {
        string opStr = op.text();

        if (opStr == "=")
        {
            return 0;
        }
        else if (opStr == "<" || opStr == ">" || opStr == "==" || opStr == ">=" || opStr == "<=")
        {
            return 1;
        }
        else if (opStr == "+" || opStr == "-")
        {
            return 2;
        }
        else if (opStr == "*" || opStr == "/" || opStr == "%")
        {
            return 3;
        }
        else if (opStr == "++" || opStr == "--")
        {
            return 4;
        }
        else
        {
            PARSE_ERROR("Unrecognized operator in parser.");
        }
    }

    shared_ptr<Expression> Parser::parseStatementHelper(shared_ptr<Expression> curr, int minPrecedence)
    {
        //TODO: figure out unary operators (increment, decrement, function call, etc.)
        if (mTokens.current().type() != TokenType::Operator)
        {
            PARSE_ERROR("Expected operator in expression.");
        }

        while (mTokens.current().type() == TokenType::Operator && operatorPrecedence(mTokens.current()) >= minPrecedence)
        {
            Token op = mTokens.current();
            mTokens.advance();
            shared_ptr<Expression> next = makeNode();

            while (mTokens.current().type() == TokenType::Operator && operatorPrecedence(mTokens.current()) > operatorPrecedence(op))
            {
                next = parseStatementHelper(next, operatorPrecedence(mTokens.current()));
            }

            curr = shared_ptr<Expression>(new BinaryExpressionNode(curr, next, op.text()));
        }

        return curr;
    }

    shared_ptr<Expression> Parser::parseStatement()
    {
        Token curr = mTokens.current();

        if (curr.type() == TokenType::SemiColon)
        {
            mTokens.advance();
            return shared_ptr<Expression>(new EmptyStatementNode());
        }

        shared_ptr<Expression> node = makeNode();
        shared_ptr<Expression> exp;
        if (mTokens.current().type() != TokenType::SemiColon)
        {
            exp = parseStatementHelper(node, 0);
        }
        else
        {
            exp = node;
        }

        if (mTokens.current().type() != TokenType::SemiColon)
        {
            PARSE_ERROR("Expected semicolon after statement.");
        }

        mTokens.advance();
        return exp;
    }

    shared_ptr<Expression> Parser::parseExpression()
    {
        Token curr = mTokens.current();

        if (curr.type() == TokenType::Keyword)
        {
            if (curr.text() == "while")
            {
                return parseWhile();
            }
            else if (curr.text() == "return")
            {
                mTokens.advance();
                shared_ptr<Expression> ret = parseStatement();

                return shared_ptr<Expression>(new ReturnNode(ret));
            }
            else if (curr.text() == "let")
            {
                return parseLet();
            }
            else if (curr.text() == "if")
            {
                return parseIf();
            }
            else
            {
                INTERNAL_ERROR("Unhandled keyword in parser.");
            }
        }
        else
        {
            return parseStatement();
        }
    }

    shared_ptr<Expression> Parser::parseWhile()
    {
        throw "Not implemented";
    }

    shared_ptr<Expression> Parser::parseIf()
    {
        throw "Not implemented";
    }

    shared_ptr<Expression> Parser::parseLet()
    {
        // skip let keyword
        mTokens.advance();

        if (mTokens.current().type() != TokenType::Identifier)
        {
            PARSE_ERROR("Invalid identifier name");
        }

        string name = mTokens.current().text();
        mTokens.advance();

        if (mTokens.current().type() == TokenType::Colon)
        {
            mTokens.advance();

            if (mTokens.current().type() != TokenType::Identifier)
            {
                // Do case where it's just the type
                PARSE_ERROR("Expected type in declaration.");
            }

            string type = mTokens.current().text();
            mTokens.advance();

            return shared_ptr<Expression>(new DeclarationNode(type, name));
        }
        else if (mTokens.current().type() == TokenType::Operator && mTokens.current().text() == "=")
        {
            mTokens.advance();
            shared_ptr<Expression> expression = parseStatement();

            return shared_ptr<Expression>(new DeclarationNode(expression, name));
        }
        else
        {
            PARSE_ERROR("Expected : or = after let");
        }
    }

    shared_ptr<Assembly> Parser::parse()
    {
        vector<shared_ptr<Function>> functions;

        while (mTokens.hasInput())
        {
            shared_ptr<Function> function = parseFunction();
            functions.push_back(function);
        }

        return shared_ptr<Assembly>(new Assembly(mName, functions));
    }
}
