#include "lexer.h"
#include <fstream>
#include <sstream>

// ============================================================
// Конечный автомат лексического анализатора
// Состояния, классы символов и таблица переходов
// соответствуют разделу 1 описания.
// ============================================================

// Состояния автомата
enum LexState {
    ST_S  = 0,  // начальное
    ST_I  = 1,  // идентификатор
    ST_C  = 2,  // целое число
    ST_D  = 3,  // после точки (ожидание дробной части)
    ST_G  = 4,  // вещественное число
    ST_A  = 5,  // двоеточие (:)
    ST_B  = 6,  // присваивание (:=)
    ST_E  = 7,  // меньше (<)
    ST_H  = 8,  // меньше или равно (<=)
    ST_M  = 9,  // не равно (<>)
    ST_F  = 10, // больше (>)
    ST_K  = 11  // больше или равно (>=)
};

// Классы входных символов
enum CharClass {
    CC_LETTER = 0,  // буква или _
    CC_DIGIT  = 1,  // цифра
    CC_DOT    = 2,  // точка
    CC_SPACE  = 3,  // пробельный символ
    CC_COLON  = 4,  // :
    CC_EQ     = 5,  // =
    CC_LT     = 6,  // <
    CC_GT     = 7,  // >
    CC_OTHER  = 8,  // прочие символы (+, -, *, / и т.д.)
    CC_EOF    = 9   // конец файла
};

// Специальные значения ячеек таблицы переходов
const int TR_Z   = 50;  // Z  — лексема распознана, текущий символ запоминаем
const int TR_ZS  = 51;  // Z* — лексема распознана, текущий символ НЕ запоминаем (откат)
const int TR_ERR = -1;  // ошибка — недопустимый символ в данном состоянии

// Таблица переходов [состояние][класс_символа]
// <б>     <ц>     .   пробел    :      =     <      >     <с>     ┴
const int TRANS[12][10] = {
 { ST_I,  ST_C,  TR_Z,  ST_S,  ST_A,  TR_Z,  ST_E,  ST_F,  TR_Z,  TR_Z  }, // S
 { ST_I,  ST_I,  TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // I
 { TR_ZS, ST_C,  ST_D,  TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // C
 { TR_ZS, ST_G,  TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // D
 { TR_ZS, ST_G,  TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // G
 { TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, ST_B,  TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // A
 { TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // B (:=)
 { TR_ZS, TR_ZS, TR_ZS, TR_Z,  TR_ZS, ST_H,  TR_ZS, ST_M,  TR_ZS, TR_ZS }, // E (<)
 { TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // H (<=)
 { TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // M (<>)
 { TR_ZS, TR_ZS, TR_ZS, TR_Z,  TR_ZS, ST_K,  TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // F (>)
 { TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS, TR_ZS }, // K (>=)
};


Lexer::Lexer(const std::string& filename) : pos(0), line(1), column(1) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw LexerError("Не удалось открыть файл: " + filename, 0, 0);

    std::stringstream buf;
    buf << file.rdbuf();
    source = buf.str();

    keywords["if"]    = TOKEN_IF;    keywords["then"]  = TOKEN_THEN;
    keywords["else"]  = TOKEN_ELSE;  keywords["while"] = TOKEN_WHILE;
    keywords["do"]    = TOKEN_DO;    keywords["begin"] = TOKEN_BEGIN;
    keywords["end"]   = TOKEN_END;   keywords["read"]  = TOKEN_READ;
    keywords["write"] = TOKEN_WRITE; keywords["int"]   = TOKEN_INT;
    keywords["real"]  = TOKEN_REAL;
}

void Lexer::advanceChar() {
    if (pos < source.size()) {
        if (source[pos] == '\n') { line++; column = 1; }
        else 
            column++;
        pos++;
    }
}

int Lexer::charClass(char c) const {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
        return CC_LETTER;
    if (c >= '0' && c <= '9')
        return CC_DIGIT;
    if (c == '.') return CC_DOT;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return CC_SPACE;
    if (c == ':') return CC_COLON;
    if (c == '=') return CC_EQ;
    if (c == '<') return CC_LT;
    if (c == '>') return CC_GT;
    return CC_OTHER;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos < source.size()) {
        char c = source[pos];
        if (c == '{') {
            // Комментарий вида { ... }
            advanceChar();
            while (pos < source.size() && source[pos] != '}')
                advanceChar();
            if (pos < source.size()) advanceChar(); // закрывающая }
        } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advanceChar();
        } else {
            break;
        }
    }
}

Token Lexer::buildToken(int finalState, const std::string& buf,
                        int sl, int sc) const {
    Token tok;
    tok.value = buf;
    tok.line = sl;
    tok.column = sc;

    switch (finalState) {
        // Состояние S: односимвольные лексемы
        case ST_S:
            if (buf.empty()) { tok.type = TOKEN_EOF; break; }
            switch (buf[0]) {
                case '+': tok.type = TOKEN_PLUS;      break;
                case '-': tok.type = TOKEN_MINUS;     break;
                case '*': tok.type = TOKEN_MULT;      break;
                case '/': tok.type = TOKEN_DIV;       break;
                case '(': tok.type = TOKEN_LPAREN;    break;
                case ')': tok.type = TOKEN_RPAREN;    break;
                case '[': tok.type = TOKEN_LBRACKET;  break;
                case ']': tok.type = TOKEN_RBRACKET;  break;
                case ',': tok.type = TOKEN_COMMA;     break;
                case ';': tok.type = TOKEN_SEMICOLON; break;
                case '=': tok.type = TOKEN_EQ;        break;
                default:
                    throw LexerError(std::string("Неизвестный символ: ") + buf[0], sl, sc);
            }
            break;

        // Идентификатор или ключевое слово
        case ST_I: {
            auto it = keywords.find(buf);
            tok.type = (it != keywords.end()) ? it->second : TOKEN_ID;
            break;
        }

        // Целая константа: преобразуем строку в число сразу в лексере
        case ST_C:
            tok.type    = TOKEN_INT_CONST;
            tok.int_val = std::stoi(buf);
            break;

        // Незавершённое вещественное число (вида "3." без цифр после точки)
        case ST_D:
            throw LexerError(std::string("Неизвестный символ: ."),
                             sl, sc + (int)buf.size() - 1);

        // Вещественная константа: преобразуем строку в число сразу в лексере
        case ST_G:
            tok.type     = TOKEN_REAL_CONST;
            tok.real_val = std::stod(buf);
            break;

        // Одиночное двоеточие — всегда ошибка, допустимо только :=
        case ST_A:
            throw LexerError("Ожидалось ':='", sl, sc);

        case ST_B: tok.type = TOKEN_ASSIGN; break;
        case ST_E: tok.type = TOKEN_LT;     break;
        case ST_H: tok.type = TOKEN_LE;     break;
        case ST_M: tok.type = TOKEN_NE;     break;
        case ST_F: tok.type = TOKEN_GT;     break;
        case ST_K: tok.type = TOKEN_GE;     break;

        default:
            throw LexerError("Внутренняя ошибка лексера", sl, sc);
    }
    return tok;
}

Token Lexer::getNextToken() {
    // Пропускаем пробелы и комментарии до начала следующей лексемы
    skipWhitespaceAndComments();

    int startLine = line, startCol = column;
    int state = ST_S;
    std::string buf;

    while (true) {
        bool atEOF = (pos >= source.size());
        char c = atEOF ? '\0' : source[pos];
        int cc = atEOF ? CC_EOF : charClass(c);
        int ns = TRANS[state][cc];

        if (ns == TR_ERR) {
            throw LexerError(std::string("Неизвестный символ: ") + c, line, column);
        }

        if (ns == TR_Z) {
            // Z: запоминаем символ и завершаем лексему.
            // Пробел в состоянии E или F используется как разделитель (<пробел, >пробел),
            // в буфер не добавляем, чтобы он не входил в значение токена.
            if (!atEOF) {
                if (cc != CC_SPACE) buf += c;
                advanceChar();
            }
            return buildToken(state, buf, startLine, startCol);
        }

        if (ns == TR_ZS) {
            // Z*: откат — символ не запоминаем, возвращаем накопленную лексему
            return buildToken(state, buf, startLine, startCol);
        }

        // Обычный переход: запоминаем символ, пробелы в буфер не включаем
        if (cc != CC_SPACE) buf += c;
        advanceChar();

        // Пока находимся в начальном состоянии (пропуск пробелов через S->S),
        // сдвигаем стартовую позицию токена
        if (state == ST_S && ns == ST_S) {
            startLine = line;
            startCol  = column;
        }

        state = ns;

        // Состояния B, H, M, K — завершающие сразу при входе:
        // лексема полностью набрана (например := после A+= или <= после E+=)
        if (state == ST_B || state == ST_H ||
            state == ST_M || state == ST_K) {
            return buildToken(state, buf, startLine, startCol);
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    Token tok;
    do {
        tok = getNextToken();
        tokens.push_back(tok);
    } while (tok.type != TOKEN_EOF);
    return tokens;
}
