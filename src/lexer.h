#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <exception>
#include <map>

// Типы токенов (лексем)
enum TokenType {
    TOKEN_ID = 1,
    TOKEN_INT_CONST,
    TOKEN_REAL_CONST,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULT,
    TOKEN_DIV,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_ASSIGN,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_BEGIN,
    TOKEN_END,
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_INT,
    TOKEN_REAL,
    TOKEN_SQRT,
    TOKEN_ABS,
    TOKEN_EOF,
    TOKEN_UNKNOWN
};

struct Token {
    TokenType   type;
    std::string value;     // исходная строка (для идентификаторов и диагностики)
    int         int_val;   // числовое значение для TOKEN_INT_CONST
    double      real_val;  // числовое значение для TOKEN_REAL_CONST
    int         line;
    int         column;

    Token(TokenType t = TOKEN_UNKNOWN, const std::string& v = "", int l = 0, int c = 0)
        : type(t), value(v), int_val(0), real_val(0.0), line(l), column(c) {}
};

class LexerError : public std::exception {
private:
    std::string message;
public:
    int line, column;
    LexerError(const std::string& msg, int l, int c)
        : message(msg), line(l), column(c) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class Lexer {
private:
    std::string source;
    size_t pos;
    int line, column;
    std::map<std::string, TokenType> keywords;

    // Продвинуть позицию на один символ
    void advanceChar();

    // Определить класс входного символа
    int charClass(char c) const;

    // Пропустить пробелы и комментарии { ... }
    void skipWhitespaceAndComments();

    // Построить токен по завершающему состоянию автомата и накопленному буферу
    Token buildToken(int finalState, const std::string& buf, int sl, int sc) const;

public:
    explicit Lexer(const std::string& filename);

    std::vector<Token> tokenize();
    Token getNextToken();
};

#endif // LEXER_H
