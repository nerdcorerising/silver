

#include "parser.h"

#include <stack>

using namespace std;
using namespace tok;
using namespace ast;

namespace parse
{
    Parser::Parser(string name, Tokenizer tok, istream &in) :
        mName(name),
        mTokens(tok, in)
    {

    }

    void Parser::expectCurrentTokenType(TokenType type, string message)
    {
        expectTokenType(current(), type, message);
    }
    
    void Parser::expectCurrentTokenText(string text, string message)
    {
        expectTokenText(current(), text, message);
    }
    
    void Parser::expectCurrentTokenTypeAndText(TokenType type, string text, string message)
    {
        expectCurrentTokenType(type, message);
        expectCurrentTokenText(text, message);
    }

    void Parser::expectTokenType(Token token, TokenType type, string message)
    {
        if (token.type() != type)
        {
            PARSE_ERROR(message);
        }
    }
    
    void Parser::expectTokenText(Token token, string text, string message)
    {
        if (token.text() != text)
        {
            PARSE_ERROR(message);
        }
    }
    
    void Parser::expectTokenTypeAndText(Token token, TokenType type, string text, string message)
    {
        expectTokenType(token, type, message);
        expectTokenText(token, text, message);
    }

    void Parser::advance()
    {
        mTokens.advance();
    }

    Token Parser::current()
    {
        Token token;
        if (!mTokens.tryGetCurrent(token))
        {
            PARSE_ERROR("Unexpected end of input");
        }

        return token;
    }

    Token Parser::lookAhead()
    {
        Token token;
        if (!mTokens.tryGetLookAhead(token))
        {
            PARSE_ERROR("Unexpected end of input");
        }

        return token;
    }

    Token Parser::lookAheadBy(size_t pos)
    {
        Token token;
        if (!mTokens.tryGetLookAheadBy(pos, token))
        {
            PARSE_ERROR("Unexpected end of input");
        }

        return token;
    }

    vector<shared_ptr<Argument>> Parser::parseArgumentsForDeclaration()
    {
        expectCurrentTokenType(TokenType::OpenParens, "Unexpected token after function name");

        advance();

        vector<shared_ptr<Argument>> args;
        if (current().type() == TokenType::CloseParens)
        {
            advance();
            return args;
        }

        for (;;)
        {
            expectCurrentTokenType(TokenType::Identifier, "Invalid name for function argument.");

            string name = current().text();
            advance();

            expectCurrentTokenType(TokenType::Colon, "Expected colon after function argument.");

            advance();

            expectCurrentTokenType(TokenType::Identifier, "Type for function argument.");

            string type = current().text();
            advance();

            args.push_back(shared_ptr<Argument>(new Argument(type, name)));

            TokenType curType = current().type();
            if (curType == TokenType::CloseParens)
            {
                break;
            }

            expectCurrentTokenType(TokenType::Comma, "Expected comma or close of parentheses in argument list");

            // Skip comma
            advance();
        }

        expectCurrentTokenType(TokenType::CloseParens, "Expected closing parentheses.");

        advance();
        return args;
    }

    shared_ptr<Function> Parser::parseFunction()
    {
        expectCurrentTokenTypeAndText(TokenType::Keyword, "fn", "Missing function keyword");

        advance();

        expectCurrentTokenType(TokenType::Identifier, "Invalid function name.");

        string name = current().text();
        advance();

        vector<shared_ptr<Argument>> args = parseArgumentsForDeclaration();

        string returnType;
        if (current().type() == TokenType::Operator && current().text() == "->")
        {
            advance();

            expectCurrentTokenType(TokenType::Identifier, "Invalid return type");

            // TODO: if multiple return types are wanted, need to implement here
            returnType = current().text();
            advance();
        }

        shared_ptr<Block> block = parseBlock();

        return shared_ptr<Function>(new Function(block, name, args, returnType));
    }

    shared_ptr<Block> Parser::parseBlock()
    {
        expectCurrentTokenType(TokenType::LeftBrace, "Expected curly brace to open block");

        advance();
        vector<shared_ptr<Expression>> expressions;

        while (current().type() != TokenType::RightBrace)
        {
            shared_ptr<Expression> exp = parseExpression();
            expressions.push_back(exp);
        }

        advance();
        return shared_ptr<Block>(new Block(expressions));
    }

    vector<shared_ptr<Expression>> Parser::parseFunctionArgs()
    {
        expectCurrentTokenType(TokenType::OpenParens, "Expected open parens to open argument list");

        advance();

        vector<shared_ptr<Expression>> args;
        while (current().type() != TokenType::CloseParens)
        {
            shared_ptr<Expression> arg = makeNode();
            if (current().type() != TokenType::Comma && current().type() != TokenType::CloseParens)
            {
                arg = parseStatementHelper(arg, 0);
            }

            args.push_back(arg);

            if (current().type() == TokenType::Comma)
            {
                advance();
            }
        }

        expectCurrentTokenType(TokenType::CloseParens, "Expected close parens to close argument list");

        advance();
        return args;
    }

    shared_ptr<Expression> Parser::makeNode()
    {
        shared_ptr<Expression> node;
        double dVal;
        int iVal;

        switch (current().type())
        {
        case TokenType::IntLiteral:
        {
            iVal = stoi(current().text());
            node = shared_ptr<Expression>(new IntegerLiteralNode(iVal));
            advance();
        }
            break;
        case TokenType::OpenParens:
        {
            advance();

            if (current().type() == TokenType::Identifier 
                && lookAhead().type() == TokenType::CloseParens)
            {
                string type = current().text();
                mTokens.advanceBy(2);
                shared_ptr<Expression> exp = makeNode();
                node = shared_ptr<Expression>(new CastNode(type, exp));
            }
            else
            {
                node = parseStatementHelper(makeNode(), 0);
                expectCurrentTokenType(TokenType::CloseParens, "Expected closing parens.");

                advance();
            }
        }
            break;
        case TokenType::StringLiteral:
        {
            node = shared_ptr<Expression>(new StringLiteralNode(current().text()));
            advance();
        }
            break;
        case TokenType::FloatLiteral:
        {
            dVal = stod(current().text());
            node = shared_ptr<Expression>(new FloatLiteralNode(dVal));
            advance();
        }
            break;
        case TokenType::Identifier:
        {
            if (lookAhead().type() == TokenType::OpenParens)
            {
                string name = current().text();
                advance();
                vector<shared_ptr<Expression>> args = parseFunctionArgs();
                node = shared_ptr<Expression>(new FunctionCallNode(name, args));
            }
            else
            {
                node = shared_ptr<Expression>(new IdentifierNode(current().text()));
                advance();
            }
        }
            break;
        default:
        {
            CONSISTENCY_CHECK(false, "Unknown token type in Parser::makeNode");
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
            CONSISTENCY_CHECK(false, "Unrecognized operator in parser.");
        }
    }

    shared_ptr<Expression> Parser::parseStatementHelper(shared_ptr<Expression> curr, int minPrecedence)
    {
        //TODO: figure out unary operators (increment, decrement, function call, etc.)
        expectCurrentTokenType(TokenType::Operator, "Expected operator in expression.");

        while (current().type() == TokenType::Operator && operatorPrecedence(current()) >= minPrecedence)
        {
            Token op = current();
            advance();
            shared_ptr<Expression> next = makeNode();

            while (current().type() == TokenType::Operator && operatorPrecedence(current()) > operatorPrecedence(op))
            {
                next = parseStatementHelper(next, operatorPrecedence(current()));
            }

            curr = shared_ptr<Expression>(new BinaryExpressionNode(curr, next, op.text()));
        }

        return curr;
    }

    shared_ptr<Expression> Parser::parseStatement()
    {
        Token curr = current();

        if (curr.type() == TokenType::SemiColon)
        {
            advance();
            return shared_ptr<Expression>(new EmptyStatementNode());
        }

        shared_ptr<Expression> node = makeNode();
        shared_ptr<Expression> exp;
        if (current().type() != TokenType::SemiColon)
        {
            exp = parseStatementHelper(node, 0);
        }
        else
        {
            exp = node;
        }

        return exp;
    }

    shared_ptr<Expression> Parser::parseExpression()
    {
        Token curr = current();

        if (curr.type() == TokenType::Keyword)
        {
            if (curr.text() == "while")
            {
                return parseWhile();
            }
            else if (curr.text() == "return")
            {
                advance();
                shared_ptr<Expression> ret = parseStatement();

                shared_ptr<Expression> returnStatement = shared_ptr<Expression>(new ReturnNode(ret));

                expectCurrentTokenType(TokenType::SemiColon, "Expected semicolon after statement.");
                advance();

                return returnStatement;
            }
            else if (curr.text() == "let")
            {
                shared_ptr<Expression> let = parseLet();

                expectCurrentTokenType(TokenType::SemiColon, "Expected semicolon after statement.");
                advance();

                return let;
            }
            else if (curr.text() == "if")
            {
                return parseIf();
            }
            else
            {
                CONSISTENCY_CHECK(false, "Unhandled keyword in parser.");
            }
        }
        else
        {
            shared_ptr<Expression> statement = parseStatement();
            expectCurrentTokenType(TokenType::SemiColon, "Expected semicolon after statement.");
            advance();
            return statement;
        }
    }

    shared_ptr<Expression> Parser::parseIfWhileCondition()
    {        
        expectCurrentTokenType(TokenType::OpenParens, "Missing open parentheses for if statement");
        advance();

        shared_ptr<Expression> condition = parseStatement();

        expectCurrentTokenType(TokenType::CloseParens, "Missing closing parentheses after if statement.");
        advance();

        return condition;
    }

    shared_ptr<Expression> Parser::parseWhile()
    {
        CONSISTENCY_CHECK(current().text() == "while", "parseWhile called without while keyword");
        // skip while
        advance();

        shared_ptr<Expression> condition = parseIfWhileCondition();

        shared_ptr<Block> block = parseBlock();

        return shared_ptr<Expression>(new WhileNode(condition, block));
    }

    shared_ptr<Expression> Parser::parseIf()
    {
        CONSISTENCY_CHECK(current().text() == "if", "parseIf called without if keyword");
        // skip if
        advance();

        shared_ptr<Expression> condition = parseIfWhileCondition();

        shared_ptr<Block> block = parseBlock();

        vector<shared_ptr<ElseIfNode>> elseIfs;
        while (current().type() == TokenType::Keyword && current().text() == "elif")
        {
            advance();

            shared_ptr<Expression> elseIfCondition = parseIfWhileCondition();
            shared_ptr<Block> elseIfBlock = parseBlock();

            shared_ptr<ElseIfNode> elseIfNode = shared_ptr<ElseIfNode>(new ElseIfNode(elseIfCondition, elseIfBlock));
            elseIfs.push_back(elseIfNode);
        }

        shared_ptr<Block> elseBlock;
        if (current().type() == TokenType::Keyword && current().text() == "else")
        {
            advance();

            elseBlock = parseBlock();
        }

        return shared_ptr<Expression>(new IfNode(condition, block, elseIfs, elseBlock));
    }

    shared_ptr<Expression> Parser::parseLet()
    {
        CONSISTENCY_CHECK(current().text() == "let", "parseLet called without let keyword");

        // skip let keyword
        advance();

        expectCurrentTokenType(TokenType::Identifier, "Invalid identifier name");

        string name = current().text();
        advance();

        if (current().type() == TokenType::Colon)
        {
            advance();

            expectCurrentTokenType(TokenType::Identifier, "Expected type in declaration.");

            string type = current().text();
            advance();

            return shared_ptr<Expression>(new DeclarationNode(type, name));
        }
        else if (current().type() == TokenType::Operator && current().text() == "=")
        {
            advance();
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
