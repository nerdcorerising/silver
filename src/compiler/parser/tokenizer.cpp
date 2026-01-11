
#include "tokenizer.h"
#include "common.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


using namespace std;

namespace tok
{
    Tokenizer::Tokenizer(void) :
        mOperators({ "+", "++", "-", "--", "*", "/", "%", "=", "!=", "<", ">", "==", ">=", "<=", "->", ".", "&&", "||" }),
        mKeywords({ "if", "elif", "else", "for", "while", "module", "return", "fn", "let", "import", "class", "public", "private", "alloc", "namespace", "local" }),
        mSpecialtokens({ '[', ']', '{', '}', '(', ')', ',', ';', ':' }),
        mBuffer(),
        mState(BufferState::EmptyState),
        mReady(false),
        mEscapeNextChar(false),
        mEndOfString(false),
        mUnicodeEscapeDigits(0),
        mUnicodeEscapeCollected(0),
        mUnicodeCodepoint(0),
        mCurrentLine(1),
        mCurrentColumn(1),
        mTokenStartLine(1),
        mTokenStartColumn(1)
    {

    }

    bool Tokenizer::isHexDigit(char ch)
    {
        return (ch >= '0' && ch <= '9') ||
               (ch >= 'a' && ch <= 'f') ||
               (ch >= 'A' && ch <= 'F');
    }

    int Tokenizer::hexDigitValue(char ch)
    {
        if (ch >= '0' && ch <= '9') return ch - '0';
        if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
        if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
        return 0;
    }

    void Tokenizer::appendUtf8(uint32_t codepoint)
    {
        // Convert Unicode codepoint to UTF-8 bytes and append to buffer
        if (codepoint <= 0x7F)
        {
            // 1-byte sequence: 0xxxxxxx
            mBuffer.push_back(static_cast<char>(codepoint));
        }
        else if (codepoint <= 0x7FF)
        {
            // 2-byte sequence: 110xxxxx 10xxxxxx
            mBuffer.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            mBuffer.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint <= 0xFFFF)
        {
            // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
            mBuffer.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            mBuffer.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            mBuffer.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        else if (codepoint <= 0x10FFFF)
        {
            // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            mBuffer.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            mBuffer.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            mBuffer.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            mBuffer.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        // Invalid codepoints (> 0x10FFFF) are silently ignored
    }

    bool Tokenizer::ready()
    {
        return mReady;
    }

    void Tokenizer::insert(char ch)
    {
        bool insert = true;

        // Record position when starting a new token
        if (mState == BufferState::EmptyState)
        {
            mTokenStartLine = mCurrentLine;
            mTokenStartColumn = mCurrentColumn;
        }

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
            else if (mUnicodeEscapeDigits > 0)
            {
                // We're collecting hex digits for a Unicode escape
                if (isHexDigit(ch))
                {
                    mUnicodeCodepoint = (mUnicodeCodepoint << 4) | static_cast<uint32_t>(hexDigitValue(ch));
                    mUnicodeEscapeCollected++;
                    insert = false;

                    if (mUnicodeEscapeCollected == mUnicodeEscapeDigits)
                    {
                        // We have all the digits, convert to UTF-8
                        appendUtf8(mUnicodeCodepoint);
                        mUnicodeEscapeDigits = 0;
                        mUnicodeEscapeCollected = 0;
                        mUnicodeCodepoint = 0;
                    }
                }
                else
                {
                    // Invalid character in Unicode escape - abort
                    abort();
                }
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
                case 'u':
                    // \uXXXX - 4 hex digits
                    mUnicodeEscapeDigits = 4;
                    mUnicodeEscapeCollected = 0;
                    mUnicodeCodepoint = 0;
                    insert = false;
                    break;
                case 'U':
                    // \UXXXXXXXX - 8 hex digits
                    mUnicodeEscapeDigits = 8;
                    mUnicodeEscapeCollected = 0;
                    mUnicodeCodepoint = 0;
                    insert = false;
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
                insert = false;  // Don't include closing quote in string value
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
            // Don't include opening quote in string value
            if (mState == BufferState::StringConstantState)
            {
                insert = false;
            }
        }
        break;
        }

        if (insert)
        {
            mBuffer.push_back(ch);
        }

        // Update line/column tracking
        if (ch == '\n')
        {
            mCurrentLine++;
            mCurrentColumn = 1;
        }
        else
        {
            mCurrentColumn++;
        }
    }

    Token Tokenizer::dispense()
    {
        ASSERT(ready());

        char ch = mBuffer.at(mBuffer.size() - 1);
        mBuffer.pop_back();

        string text = string(mBuffer.begin(), mBuffer.end());
        TokenType type = tokenType(text);

        // Save the position of the token we're about to return
        int tokenLine = mTokenStartLine;
        int tokenColumn = mTokenStartColumn;

        mState = bufferType(ch);
        mBuffer.clear();
        // Don't include opening quote in string buffer
        if (mState != BufferState::StringConstantState)
        {
            mBuffer.push_back(ch);
        }

        // Record start of the next token (which begins with ch)
        mTokenStartLine = mCurrentLine;
        mTokenStartColumn = mCurrentColumn - 1; // ch was already counted

        mReady = false;

        return Token(type, text, tokenLine, tokenColumn);
    }

    Token Tokenizer::flush()
    {
        string text = string(mBuffer.begin(), mBuffer.end());
        TokenType type = tokenType(text);

        // Save the position of the token we're about to return
        int tokenLine = mTokenStartLine;
        int tokenColumn = mTokenStartColumn;

        mState = BufferState::EmptyState;

        mReady = false;
        return Token(type, text, tokenLine, tokenColumn);
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
        return std::find(mKeywords.begin(), mKeywords.end(), str) != mKeywords.end();
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
        case ':':
            return TokenType::Colon;
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
        return std::find(mOperators.begin(), mOperators.end(), chp) != mOperators.end();
    }

    bool Tokenizer::isSpecialToken(char ch)
    {
        return std::find(mSpecialtokens.begin(), mSpecialtokens.end(), ch) != mSpecialtokens.end();
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
