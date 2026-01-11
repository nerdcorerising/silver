

#include "parser.h"

#include <stack>
#include <filesystem>
#include <fstream>
#include <iostream>

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

    void Parser::reportFatalError(string message)
    {
        // Try to get current token for line info
        Token token;
        if (mTokens.tryGetCurrent(token) && token.line() > 0)
        {
            reportFatalError(message, token);
        }
        else
        {
            fprintf(stderr, "Error: %s\n", message.c_str());
            throw message;
        }
    }

    void Parser::reportFatalError(string message, Token token)
    {
        if (token.line() > 0)
        {
            fprintf(stderr, "Error at line %d, column %d: %s\n",
                    token.line(), token.column(), message.c_str());
        }
        else
        {
            fprintf(stderr, "Error: %s\n", message.c_str());
        }
        throw message;
    }

    bool Parser::expectCurrentTokenType(TokenType type, string message)
    {
        return expectTokenType(current(), type, message);
    }

    bool Parser::expectCurrentTokenText(string text, string message)
    {
        return expectTokenText(current(), text, message);
    }

    bool Parser::expectCurrentTokenTypeAndText(TokenType type, string text, string message)
    {
        return expectCurrentTokenType(type, message)
               && expectCurrentTokenText(text, message);
    }

    bool Parser::expectTokenType(Token token, TokenType type, string message)
    {
        if (token.type() != type)
        {
            reportFatalError(message);
            return false;
        }

        return true;
    }

    bool Parser::expectTokenText(Token token, string text, string message)
    {
        if (token.text() != text)
        {
            reportFatalError(message);
            return false;
        }

        return true;
    }

    bool Parser::expectTokenTypeAndText(Token token, TokenType type, string text, string message)
    {
        return expectTokenType(token, type, message)
               && expectTokenText(token, text, message);
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
            reportFatalError("Unexpected end of input");
        }

        return token;
    }

    Token Parser::lookAhead()
    {
        Token token;
        if (!mTokens.tryGetLookAhead(token))
        {
            reportFatalError("Unexpected end of input");
        }

        return token;
    }

    Token Parser::lookAheadBy(size_t pos)
    {
        Token token;
        if (!mTokens.tryGetLookAheadBy(pos, token))
        {
            reportFatalError("Unexpected end of input");
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

    vector<shared_ptr<Function>> Parser::parseImport()
    {
        // TODO: better search, user defined imports
        expectCurrentTokenTypeAndText(TokenType::Keyword, "import", "Missing import keyword");
        advance();

        expectCurrentTokenType(TokenType::Identifier, "Invalid import name.");
        filesystem::path fileName = filesystem::path(current().text() + ".sl");
        advance();

        expectCurrentTokenType(TokenType::SemiColon, "Expected semicolon after import.");
        advance();

        filebuf fb;
        // Now we know what they're trying to import, let's find it and parse it
        if (!filesystem::exists(fileName))
        {
            fileName = filesystem::path("framework") / fileName;
        }

        if (fb.open(fileName.string().c_str(), ios::in))
        {
            istream input = istream(&fb);

            Tokenizer tok;
            Parser parser(fileName.string(), tok, input);
            shared_ptr<ast::Assembly> importAssembly = parser.parse();
            return importAssembly->getFunctions();
        }
        else
        {
            reportFatalError("Could not find import file " + fileName.string());
            return vector<shared_ptr<Function>>();
        }
    }

    shared_ptr<Function> Parser::parseFunction(bool isLocal)
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

        shared_ptr<BlockNode> block = parseBlock();

        return shared_ptr<Function>(new Function(block, name, args, returnType, isLocal));
    }

    shared_ptr<Field> Parser::parseField()
    {
        // Parse: "name: visibility type;"
        expectCurrentTokenType(TokenType::Identifier, "Expected field name");
        string name = current().text();
        advance();

        expectCurrentTokenType(TokenType::Colon, "Expected ':' after field name");
        advance();

        // Parse visibility (public or private)
        expectCurrentTokenType(TokenType::Keyword, "Expected 'public' or 'private'");
        Visibility visibility;
        if (current().text() == "public")
        {
            visibility = Visibility::Public;
        }
        else if (current().text() == "private")
        {
            visibility = Visibility::Private;
        }
        else
        {
            reportFatalError("Expected 'public' or 'private' for field visibility");
            return nullptr;
        }
        advance();

        // Parse type
        expectCurrentTokenType(TokenType::Identifier, "Expected field type");
        string type = current().text();
        advance();

        expectCurrentTokenType(TokenType::SemiColon, "Expected ';' after field declaration");
        advance();

        return shared_ptr<Field>(new Field(name, type, visibility));
    }

    shared_ptr<ClassDeclaration> Parser::parseClass()
    {
        expectCurrentTokenTypeAndText(TokenType::Keyword, "class", "Expected 'class' keyword");
        advance();

        expectCurrentTokenType(TokenType::Identifier, "Expected class name");
        string name = current().text();
        advance();

        expectCurrentTokenType(TokenType::LeftBrace, "Expected '{' after class name");
        advance();

        vector<shared_ptr<Field>> fields;
        while (current().type() != TokenType::RightBrace)
        {
            shared_ptr<Field> field = parseField();
            fields.push_back(field);
        }

        expectCurrentTokenType(TokenType::RightBrace, "Expected '}' to close class");
        advance();

        return shared_ptr<ClassDeclaration>(new ClassDeclaration(name, fields));
    }

    shared_ptr<NamespaceDeclaration> Parser::parseNamespace()
    {
        expectCurrentTokenTypeAndText(TokenType::Keyword, "namespace", "Expected 'namespace' keyword");
        advance();

        // Parse namespace name (possibly dotted: "Foo.Bar")
        expectCurrentTokenType(TokenType::Identifier, "Expected namespace name");
        string fullName = current().text();
        advance();

        // Handle dotted namespace names: namespace Foo.Bar { }
        while (current().type() == TokenType::Operator && current().text() == ".")
        {
            advance();  // skip dot
            expectCurrentTokenType(TokenType::Identifier, "Expected namespace name after '.'");
            fullName += "." + current().text();
            advance();
        }

        expectCurrentTokenType(TokenType::LeftBrace, "Expected '{' after namespace name");
        advance();

        vector<shared_ptr<Function>> functions;
        vector<shared_ptr<ClassDeclaration>> classes;
        vector<shared_ptr<NamespaceDeclaration>> nestedNamespaces;

        while (current().type() != TokenType::RightBrace)
        {
            if (current().type() == TokenType::Keyword)
            {
                if (current().text() == "local")
                {
                    advance();
                    expectCurrentTokenTypeAndText(TokenType::Keyword, "fn", "Expected 'fn' after 'local'");
                    shared_ptr<Function> func = parseFunction(true);
                    functions.push_back(func);
                }
                else if (current().text() == "fn")
                {
                    shared_ptr<Function> func = parseFunction(false);
                    functions.push_back(func);
                }
                else if (current().text() == "class")
                {
                    shared_ptr<ClassDeclaration> cls = parseClass();
                    classes.push_back(cls);
                }
                else if (current().text() == "namespace")
                {
                    shared_ptr<NamespaceDeclaration> nested = parseNamespace();
                    nestedNamespaces.push_back(nested);
                }
                else
                {
                    reportFatalError("Unexpected keyword in namespace: " + current().text());
                }
            }
            else
            {
                reportFatalError("Expected function, class, or namespace in namespace body");
            }
        }

        expectCurrentTokenType(TokenType::RightBrace, "Expected '}' to close namespace");
        advance();

        return shared_ptr<NamespaceDeclaration>(
            new NamespaceDeclaration(fullName, functions, classes, nestedNamespaces));
    }

    shared_ptr<BlockNode> Parser::parseBlock()
    {
        expectCurrentTokenType(TokenType::LeftBrace, "Expected curly brace to open block");
        int line = current().line();
        int col = current().column();
        advance();

        vector<shared_ptr<Expression>> expressions;

        while (current().type() != TokenType::RightBrace)
        {
            shared_ptr<Expression> exp = parseExpression();
            expressions.push_back(exp);
        }

        advance();

        return shared_ptr<BlockNode>(new BlockNode(expressions, line, col));
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
        int line = current().line();
        int col = current().column();

        // Handle alloc keyword
        if (current().type() == TokenType::Keyword && current().text() == "alloc")
        {
            advance();
            expectCurrentTokenType(TokenType::Identifier, "Expected type name after 'alloc'");
            string typeName = current().text();
            advance();
            vector<shared_ptr<Expression>> args = parseFunctionArgs();
            return shared_ptr<Expression>(new AllocNode(typeName, args, line, col));
        }

        switch (current().type())
        {
        case TokenType::IntLiteral:
        {
            iVal = stoi(current().text());
            node = shared_ptr<Expression>(new IntegerLiteralNode(iVal, line, col));
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
                node = shared_ptr<Expression>(new CastNode(type, exp, line, col));
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
            node = shared_ptr<Expression>(new StringLiteralNode(current().text(), line, col));
            advance();
        }
            break;
        case TokenType::FloatLiteral:
        {
            dVal = stod(current().text());
            node = shared_ptr<Expression>(new FloatLiteralNode(dVal, line, col));
            advance();
        }
            break;
        case TokenType::Identifier:
        {
            // Check for qualified call: Identifier.something.func()
            if (lookAhead().type() == TokenType::Operator && lookAhead().text() == ".")
            {
                // Collect the dotted path
                string path = current().text();
                advance();  // skip first identifier

                while (current().type() == TokenType::Operator && current().text() == ".")
                {
                    advance();  // skip dot
                    expectCurrentTokenType(TokenType::Identifier, "Expected identifier after '.'");
                    path += "." + current().text();
                    advance();

                    // Check if this might be a function call
                    if (current().type() == TokenType::OpenParens)
                    {
                        // It's a qualified call: path ends with function name
                        // Split path into namespace path and function name
                        size_t lastDot = path.rfind('.');
                        string namespacePath = path.substr(0, lastDot);
                        string funcName = path.substr(lastDot + 1);

                        vector<shared_ptr<Expression>> args = parseFunctionArgs();
                        node = shared_ptr<Expression>(new QualifiedCallNode(namespacePath, funcName, args, line, col));
                        break;
                    }
                }

                // If we didn't find a function call, it's member access
                // Create chained MemberAccessNode
                if (node == nullptr)
                {
                    // Parse the path into chained member access
                    size_t pos = 0;
                    size_t dotPos = path.find('.');
                    string firstPart = path.substr(0, dotPos);
                    node = shared_ptr<Expression>(new IdentifierNode(firstPart, line, col));

                    pos = dotPos + 1;
                    while (pos < path.size())
                    {
                        dotPos = path.find('.', pos);
                        string part;
                        if (dotPos == string::npos)
                        {
                            part = path.substr(pos);
                            pos = path.size();
                        }
                        else
                        {
                            part = path.substr(pos, dotPos - pos);
                            pos = dotPos + 1;
                        }
                        node = shared_ptr<Expression>(new MemberAccessNode(node, part, line, col));
                    }
                }
            }
            else if (lookAhead().type() == TokenType::OpenParens)
            {
                string name = current().text();
                advance();
                vector<shared_ptr<Expression>> args = parseFunctionArgs();
                node = shared_ptr<Expression>(new FunctionCallNode(name, args, line, col));
            }
            else
            {
                node = shared_ptr<Expression>(new IdentifierNode(current().text(), line, col));
                advance();
            }
        }
            break;
        default:
        {
            CONSISTENCY_CHECK(false, "Unknown token type in Parser::makeNode: '" + current().text() + "' (type " + std::to_string((int)current().type()) + ")");
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
        else if (opStr == "||")
        {
            return 1;
        }
        else if (opStr == "&&")
        {
            return 2;
        }
        else if (opStr == "<" || opStr == ">" || opStr == "==" || opStr == "!=" || opStr == ">=" || opStr == "<=")
        {
            return 3;
        }
        else if (opStr == "+" || opStr == "-")
        {
            return 4;
        }
        else if (opStr == "*" || opStr == "/" || opStr == "%")
        {
            return 5;
        }
        else if (opStr == "++" || opStr == "--")
        {
            return 6;
        }
        else if (opStr == ".")
        {
            return 7;  // Highest precedence for member access
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
            int opLine = op.line();
            int opCol = op.column();
            advance();

            // Special handling for member access operator
            if (op.text() == ".")
            {
                expectCurrentTokenType(TokenType::Identifier, "Expected member name after '.'");
                string memberName = current().text();
                advance();
                curr = shared_ptr<Expression>(new MemberAccessNode(curr, memberName, opLine, opCol));
                continue;
            }

            shared_ptr<Expression> next = makeNode();

            while (current().type() == TokenType::Operator && operatorPrecedence(current()) > operatorPrecedence(op))
            {
                next = parseStatementHelper(next, operatorPrecedence(current()));
            }

            curr = shared_ptr<Expression>(new BinaryExpressionNode(curr, next, op.text(), opLine, opCol));
        }

        return curr;
    }

    shared_ptr<Expression> Parser::parseStatement()
    {
        Token curr = current();

        if (curr.type() == TokenType::SemiColon)
        {
            int line = curr.line();
            int col = curr.column();
            advance();
            return shared_ptr<Expression>(new EmptyStatementNode(line, col));
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
                int line = curr.line();
                int col = curr.column();
                advance();
                shared_ptr<Expression> ret = parseStatement();

                shared_ptr<Expression> returnStatement = shared_ptr<Expression>(new ReturnNode(ret, line, col));

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
        int line = current().line();
        int col = current().column();
        // skip while
        advance();

        shared_ptr<Expression> condition = parseIfWhileCondition();

        shared_ptr<BlockNode> block = parseBlock();

        return shared_ptr<Expression>(new WhileNode(condition, block, line, col));
    }

    shared_ptr<Expression> Parser::parseIf()
    {
        CONSISTENCY_CHECK(current().text() == "if", "parseIf called without if keyword");
        int ifBlockLine = current().line();
        int ifBlockCol = current().column();
        // skip if
        advance();

        vector<shared_ptr<IfNode>> ifs;

        shared_ptr<Expression> condition = parseIfWhileCondition();
        shared_ptr<BlockNode> block = parseBlock();
        ifs.push_back(shared_ptr<IfNode>(new IfNode(condition, block, ifBlockLine, ifBlockCol)));

        while (current().type() == TokenType::Keyword && current().text() == "elif")
        {
            int elifLine = current().line();
            int elifCol = current().column();
            advance();

            shared_ptr<Expression> elseIfCondition = parseIfWhileCondition();
            shared_ptr<BlockNode> elseIfBlock = parseBlock();

            shared_ptr<IfNode> elseIfNode = shared_ptr<IfNode>(new IfNode(elseIfCondition, elseIfBlock, elifLine, elifCol));
            ifs.push_back(elseIfNode);
        }

        // Codegen relies on there being a block for the else node, even if it's empty
        shared_ptr<BlockNode> elseBlock(new BlockNode(vector<shared_ptr<Expression>>()));
        if (current().type() == TokenType::Keyword && current().text() == "else")
        {
            advance();

            elseBlock = parseBlock();
        }

        return shared_ptr<Expression>(new IfBlockNode(ifs, elseBlock, ifBlockLine, ifBlockCol));
    }

    shared_ptr<Expression> Parser::parseLet()
    {
        CONSISTENCY_CHECK(current().text() == "let", "parseLet called without let keyword");
        int line = current().line();
        int col = current().column();

        // skip let keyword
        advance();

        expectCurrentTokenType(TokenType::Identifier, "Invalid identifier name");

        string name = current().text();
        advance();

        shared_ptr<Expression> expression;
        string type;

        if (current().type() == TokenType::Colon)
        {
            advance();

            expectCurrentTokenType(TokenType::Identifier, "Expected type in declaration.");

            type = current().text();
            advance();

            // Check for optional initializer after type annotation
            if (current().type() == TokenType::Operator && current().text() == "=")
            {
                advance();
                expression = parseStatement();
            }
        }
        else if (current().type() == TokenType::Operator && current().text() == "=")
        {
            advance();
            expression = parseStatement();
        }
        else
        {
            reportFatalError("Expected : or = after let");
        }

        return shared_ptr<Expression>(new DeclarationNode(name, type, expression, line, col));
    }

    shared_ptr<Assembly> Parser::parse()
    {
        vector<shared_ptr<Function>> functions;
        vector<shared_ptr<ClassDeclaration>> classes;
        vector<shared_ptr<NamespaceDeclaration>> namespaces;

        while (mTokens.hasInput())
        {
            if (current().type() == TokenType::Keyword && current().text() == "import")
            {
                vector<shared_ptr<Function>> expressions = parseImport();
                functions.insert(functions.end(), expressions.begin(), expressions.end());
            }
            else if (current().type() == TokenType::Keyword && current().text() == "class")
            {
                shared_ptr<ClassDeclaration> classDecl = parseClass();
                classes.push_back(classDecl);
            }
            else if (current().type() == TokenType::Keyword && current().text() == "namespace")
            {
                shared_ptr<NamespaceDeclaration> ns = parseNamespace();
                namespaces.push_back(ns);
            }
            else
            {
                // Top-level functions cannot be local (no namespace to restrict to)
                shared_ptr<Function> function = parseFunction(false);
                functions.push_back(function);
            }
        }

        return shared_ptr<Assembly>(new Assembly(mName, functions, classes, namespaces));
    }
}
