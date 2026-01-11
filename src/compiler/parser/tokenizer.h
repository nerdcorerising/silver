
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <cstdint>

namespace tok
{
    typedef enum
    {
        Keyword,
        Identifier,
        IntLiteral,
        FloatLiteral,
        StringLiteral,
        Operator,
        WhiteSpace,
        Comment,
        LeftBracket,
        RightBracket,
        LeftBrace,
        RightBrace,
        OpenParens,
        CloseParens,
        Comma,
        SemiColon,
        Colon,
        Error
    } TokenType;

    typedef enum
    {
        EmptyState,
        SpecialTokenState,
        VariableOrKeywordState,
        IntConstantState,
        FloatConstantState,
        StringConstantState,
        OperatorState,
        WhiteSpaceState,
        CommentState,
        ErrorState
    } BufferState;

    class Token
    {
    private:
        TokenType mType; // What kind of token it is.
        std::string mText; // The text of the token, taken directly from the input
        int mLine;   // Line number where token starts (1-based)
        int mColumn; // Column number where token starts (1-based)

    public:
        Token() :
            mType(TokenType::Error),
            mText(),
            mLine(0),
            mColumn(0)
        {
        }

        Token(TokenType ty, std::string txt) :
            mType(ty),
            mText(txt),
            mLine(0),
            mColumn(0)
        {
        }

        Token(TokenType ty, std::string txt, int line, int column) :
            mType(ty),
            mText(txt),
            mLine(line),
            mColumn(column)
        {
        }

        Token(const Token& other) = default;
        Token(Token&& other) = default;
        Token& operator=(const Token& other) = default;
        Token& operator=(Token&& other) = default;

        inline TokenType type()
        {
            return mType;
        }

        inline std::string text()
        {
            return mText;
        }

        inline int line() const
        {
            return mLine;
        }

        inline int column() const
        {
            return mColumn;
        }

        inline bool isLiteral()
        {
            return mType == IntLiteral || mType == FloatLiteral || mType == StringLiteral;
        }
    };

    class Tokenizer
    {
    public:
        Tokenizer(void);

        void insert(char);
        bool ready(void);
        Token flush(void);
        Token dispense(void);

    private:
        BufferState bufferType(char);

        // Returns true if the char provided is a letter, whitespace operator or the comment sign ('#').
        bool canStartToken(char);

        // Returns true if the char provided is a space, tab, or newline.
        bool isWhitespace(char);

        // Returns true if the char provided is a letter, digit, or underscore character.
        bool isIdentifierCharacter(char);

        // Returns true if the char is a digit (0-9).
        bool isDigit(char);

        // Returns true if the char is a letter (a-z, A-Z).
        bool isLetter(char);

        // Returns true if the string is a valid operator (language defined).
        bool isOperator(std::string);

        // Returns true if the string is a valid keyword (language defined).
        bool isKeyword(std::string);

        // Returns true if the string is a prefix of one of the valid operators (language defined).
        bool isOperatorSubstring(std::string);

        // Return true if the char is one of the special tokens (languaged defined).
        bool isSpecialToken(char ch);

        // Returns the correct TokenType for the text.
        TokenType tokenType(std::string);

        // Returns whether the identifier is a variable or a language keyword.
        TokenType variableOrKeyword(std::string);

        // Returns the correct TokenType for the special token given by the text.
        TokenType specialTokenKind(std::string);

        template<typename T>
        bool StartsWith(T left, T right)
        {
            if (left.size() < right.size())
            {
                return false;
            }

            for (unsigned int i = 0; i < right.size(); ++i)
            {
                if (left[i] != right[i])
                {
                    return false;
                }
            }

            return true;
        }

        std::vector<std::string> mOperators;
        std::vector<std::string> mKeywords;
        std::vector<char> mSpecialtokens;
        // State of the tokenizer.
        std::vector<char> mBuffer;
        BufferState mState;
        bool mReady;
        bool mEscapeNextChar;
        bool mEndOfString;

        // Unicode escape sequence handling
        int mUnicodeEscapeDigits;      // Number of hex digits expected (4 for \u, 8 for \U)
        int mUnicodeEscapeCollected;   // Number of hex digits collected so far
        uint32_t mUnicodeCodepoint;    // Accumulated codepoint value

        // Line tracking
        int mCurrentLine;      // Current line number (1-based)
        int mCurrentColumn;    // Current column number (1-based)
        int mTokenStartLine;   // Line where current token started
        int mTokenStartColumn; // Column where current token started

        // Helper methods for Unicode
        bool isHexDigit(char ch);
        int hexDigitValue(char ch);
        void appendUtf8(uint32_t codepoint);
    };

}