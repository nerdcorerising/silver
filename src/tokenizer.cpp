
#include "tokenizer.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


using namespace std;

namespace tok
{
    Tokenizer::Tokenizer(void) :
        mOperators({ "+", "++", "-", "--", "*", "/", "%", "=", "<", ">", "==", ">=", "<=" }),
        mKeywords({ "if", "else", "for", "while", "module", "return" }),
        mSpecialtokens({ '[', ']', '{', '}', '(', ')' }),
        mBuffer(),
        mState(BufferState::EmptyState),
        mReady(false),
        mEscapeNextChar(false),
        mEndOfString(false)
    {

    }

    bool Tokenizer::ready()
    {
        return mReady;
    }

    void Tokenizer::insert(char ch)
    {
        bool insert = true;

        switch (mState)
        {
        case BufferState::ErrorState:
        {
            // If we are in an error token, switch to ready to
            // dispense if we encounter a token that is valid to
            // start an identifier
            if (canStartToken(ch))
            {
                mReady = true;
            }
        }
        break;
        case BufferState::SpecialTokenState:
        {
            mReady = true;
        }
        break;
        case BufferState::WhiteSpaceState:
        {
            // if we are in a white space token, only something
            // besides a white space token will end our token
            if (!isWhitespace(ch))
            {
                mReady = true;
            }
        }
        break;
        case BufferState::VariableOrKeywordState:
        {
            // If we are in an identifier, keyword, or condition,
            // only something besides a valid identifier
            // character will end the token
            if (!isIdentifierCharacter(ch))
            {
                mReady = true;
            }
        }
        break;
        case BufferState::IntConstantState:
        {
            if (ch == '.')
            {
                mState = BufferState::FloatConstantState;
            }
            else if (!isDigit(ch))
            {
                mReady = true;
            }
        }
        break;
        case BufferState::FloatConstantState:
        {
            if (!isDigit(ch))
            {
                mReady = true;
            }
        }
        break;
        case BufferState::StringConstantState:
        {
            if (mEndOfString)
            {
                mReady = true;
                mEndOfString = false;
            }
            else if (mEscapeNextChar)
            {
                switch (ch)
                {
                case 'n':
                    ch = '\n';
                    break;
                case 't':
                    ch = '\t';
                    break;
                case 'r':
                    ch = '\r';
                    break;
                case 'v':
                    ch = '\v';
                    break;
                case 'a':
                    ch = '\a';
                    break;
                case 'b':
                    ch = '\b';
                    break;
                case 'f':
                    ch = '\f';
                    break;
                case '\'':
                case '\"':
                case '?':
                case '\\':
                    // do nothing
                    break;
                default:
                    abort();
                    break;
                }
                mEscapeNextChar = false;
            }
            else if (ch == '\"')
            {
                mEndOfString = true;
            }
            else if (ch == '\\')
            {
                mEscapeNextChar = true;
                insert = false;
            }
        }
        break;
        case BufferState::CommentState:
        {
            // a comment continues until the new line character
            if (ch == '\n')
            {
                mReady = true;
            }
        }
        break;
        case BufferState::OperatorState:
        {
            if (mBuffer.size() == 1 && mBuffer[0] == '-' && isDigit(ch))
            {
                // Negative number special case
                mState = BufferState::IntConstantState;
                break;
            }

            string s = string(mBuffer.begin(), mBuffer.end());
            int before = isOperatorSubstring(s);

            s.push_back(ch);
            int after = isOperatorSubstring(s);

            if (before && !after)
            {
                mReady = true;
            }
        }
        break;
        case BufferState::EmptyState:
        {
            // make the mBuffer mState appropriate for whatever we
            // are adding.
            mState = bufferType(ch);
        }
        break;
        }

        if (insert)
        {
            mBuffer.push_back(ch);
        }
    }

    Token Tokenizer::dispense()
    {
        ASSERT(ready());

        char ch = mBuffer.at(mBuffer.size() - 1);
        mBuffer.pop_back();

        string text = string(mBuffer.begin(), mBuffer.end());
        TokenType type = tokenType(text);


        mState = bufferType(ch);
        mBuffer.clear();
        mBuffer.push_back(ch);

        mReady = false;

        return Token(type, text);
    }

    Token Tokenizer::flush()
    {
        string text = string(mBuffer.begin(), mBuffer.end());
        TokenType type = tokenType(text);

        mState = BufferState::EmptyState;

        mReady = false;
        return Token(type, text);
    }

    TokenType Tokenizer::tokenType(string text)
    {
        switch (mState)
        {
        case BufferState::VariableOrKeywordState:
            // mBuffer holding an identifier, keyword, or condition token:
            // determine which
            return variableOrKeyword(text);
        case BufferState::SpecialTokenState:
            return specialTokenKind(text);
        case BufferState::OperatorState:
            return TokenType::Operator;
        case BufferState::WhiteSpaceState:
            // mBuffer holding white space token
            return TokenType::WhiteSpace;
        case BufferState::CommentState:
            // mBuffer holding comment token
            return TokenType::Comment;
        case BufferState::IntConstantState:
            return TokenType::IntLiteral;
        case BufferState::FloatConstantState:
            return TokenType::FloatLiteral;
        case BufferState::StringConstantState:
            return TokenType::StringLiteral;
        case BufferState::EmptyState:
        case BufferState::ErrorState:
        default:
            // mBuffer holding an error token
            return TokenType::Error;
        }
    }


    bool Tokenizer::isKeyword(string str)
    {
        unsigned int i = 0;
        for (i = 0; i < mKeywords.size(); ++i)
        {
            if (mKeywords[i] == str)
            {
                return true;
            }
        }

        return false;
    }

    TokenType Tokenizer::specialTokenKind(string text)
    {
        ASSERT(text.size() == 1);

        switch (text[0])
        {
        case '[':
            return TokenType::LeftBracket;
        case ']':
            return TokenType::RightBracket;
        case '{':
            return TokenType::LeftBrace;
        case '}':
            return TokenType::RightBrace;
        case '(':
            return TokenType::OpenParens;
        case ')':
            return TokenType::CloseParens;
        case ',':
            return TokenType::Comma;
        case ';':
            return TokenType::SemiColon;
        default:
            ASSERT(false);
        }

        return TokenType::Error;
    }

    TokenType Tokenizer::variableOrKeyword(string text)
    {
        if (isKeyword(text))
        {
            return TokenType::Keyword;
        }

        return TokenType::Identifier;
    }

    bool Tokenizer::canStartToken(char ch)
    {
        string chStr = string(1, ch);
        if (isOperatorSubstring(chStr) || isLetter(ch) || isWhitespace(ch) || (ch == '#'))
        {
            return true;
        }

        return false;
    }

    bool Tokenizer::isWhitespace(char ch)
    {
        if (ch == ' ' || ch == '\t' || ch == '\n')
        {
            return true;
        }

        return false;
    }

    bool Tokenizer::isIdentifierCharacter(char ch)
    {
        if (isLetter(ch) || isDigit(ch) || ch == '_')
        {
            return true;
        }

        return false;
    }

    bool Tokenizer::isDigit(char ch)
    {
        if (ch >= '0' && ch <= '9')
        {
            return true;
        }

        return false;
    }

    bool Tokenizer::isLetter(char ch)
    {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
        {
            return true;
        }

        return false;
    }

    bool Tokenizer::isOperatorSubstring(string chp)
    {
        unsigned int i = 0;
        for (i = 0; i < mOperators.size(); ++i)
        {
            if (StartsWith(mOperators[i], chp))
            {
                return true;
            }
        }

        return false;
    }

    bool Tokenizer::isOperator(string chp)
    {
        unsigned int i = 0;
        for (i = 0; i < mOperators.size(); ++i)
        {
            if (mOperators[i] == chp)
            {
                return true;
            }
        }

        return false;
    }

    bool Tokenizer::isSpecialToken(char ch)
    {
        switch (ch)
        {
        case '[':
        case ']':
        case '{':
        case '}':
        case '(':
        case ')':
        case ',':
        case ';':
            return true;
        default:
            return false;
        }
    }

    BufferState Tokenizer::bufferType(char ch)
    {
        string chStr = string(1, ch);
        if (isLetter(ch))
        {
            return BufferState::VariableOrKeywordState;
        }
        else if (isSpecialToken(ch))
        {
            return BufferState::SpecialTokenState;
        }
        else if (isDigit(ch))
        {
            return BufferState::IntConstantState;
        }
        else if (ch == '\"')
        {
            return BufferState::StringConstantState;
        }
        else if (isWhitespace(ch))
        {
            return BufferState::WhiteSpaceState;
        }
        else if (ch == '#')
        {
            return BufferState::CommentState;
        }
        else if (isOperatorSubstring(chStr))
        {
            return BufferState::OperatorState;
        }
        else
        {
            return BufferState::ErrorState;
        }
    }
}
