#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ops.h"
#include <vector>
#include <map>
#include <stack>
#include <exception>

class ParserError : public std::exception {
private:
    std::string message;
public:
    int line, column;
    ParserError(const std::string& msg, int l, int c)
        : message(msg), line(l), column(c) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// Нетерминалы грамматики
enum NonTerm {
    NT_P = 0,   // программа
    NT_V,       // объявление переменных
    NT_W,       // продолжение объявления (хвост)
    NT_Q,       // список операторов (хвост блока)
    NT_A,       // оператор
    NT_H,       // индексация массива
    NT_E,       // ветка else
    NT_C,       // условие
    NT_D,       // правый операнд сравнения
    NT_S,       // выражение (сложение / вычитание)
    NT_U,       // хвост выражения
    NT_T,       // терм (умножение / деление)
    NT_VM,      // хвост терма (V в грамматике)
    NT_F,       // фактор
    NT_G,       // операнд унарных операций
    NT_Z,       // вспомогательный нетерминал для семантических действий
    NT_COUNT
};

// Семантические действия 
enum SemAction {
    ACT_NONE = 0,
    ACT_OPERAND,    // записать операнд (переменную / константу) в ОПС
    ACT_ASSIGN,     // операция :=
    ACT_ADD,        // операция +
    ACT_SUB,        // операция -
    ACT_MUL,        // операция *
    ACT_DIV,        // операция /
    ACT_NEG,        // унарный минус
    ACT_INDEX,      // индексирование массива
    ACT_LT, ACT_GT, ACT_LE, ACT_GE, ACT_EQ, ACT_NE,
    ACT_READ,       // read
    ACT_WRITE,      // write
    // Семантические программы 1–5
    ACT_PROG1,  // создать метку-заглушку и JF (начало условия if/while)
    ACT_PROG2,  // обработать else (создать J, заполнить предыдущую JF-метку)
    ACT_PROG3,  // заполнить последнюю открытую метку (конец if)
    ACT_PROG4,  // запомнить позицию начала цикла (while)
    ACT_PROG5,  // создать J назад, заполнить JF-метку (конец while)
    // Вспомогательные действия для объявлений
    ACT_SET_INT,     // переключить тип объявления на int
    ACT_SET_REAL,    // переключить тип объявления на real
    ACT_SAVE_NAME,   // сохранить имя объявляемой переменной
    ACT_DECL_SCALAR, // объявить скалярную переменную
    ACT_DECL_ARRAY   // объявить массив (размер из lastToken)
};

// Элемент магазина (стека) анализатора
struct StackSym {
    enum Kind { TERM, NONTERM, ACTION } kind;
    int id; // TokenType / NonTerm / SemAction
};

using Rule = std::vector<StackSym>;

// Синтаксический анализатор — магазинный автомат + табличный
// генератор ОПС (LL(1) на основе НФ Грейбах)
class Parser {
private:
    std::vector<Token> tokens;
    size_t current;

    OPS ops;
    SymbolTable symbols;

    // Таблица разбора LL(1): (нетерминал, тип_токена) → правило (список элементов магазина)
    std::map<std::pair<int,int>, Rule> parseTable;

    // Вспомогательное состояние для семантических действий
    Token lastToken;             // последний сопоставленный токен
    std::string  pendingName;    // имя объявляемой переменной
    int pendingLine{0}, pendingCol{0};
    enum VarType { TYPE_INT, TYPE_REAL } currentVarType{TYPE_INT};
    std::stack<int> labelStack;  // стек открытых JF/J меток (backpatching)
    std::stack<int> loopStack;   // стек позиций начала while-циклов

    // Инициализация таблицы разбора
    void initTable();

    // Вспомогательная функция: добавить одно правило для нескольких lookahead-токенов
    void addRule(int nt, const std::vector<int>& tokens, const Rule& rule);

    // Текущий токен
    Token curTok() const;

    // Сопоставить и потребить токен; при несовпадении — ошибка
    void matchToken(int expectedType);

    // Выполнить семантическое действие
    void executeAction(int action);

    // Имя токена для сообщений об ошибках
    static std::string tokenName(int type);

public:
    explicit Parser(const std::vector<Token>& tokens);

    OPS parse();
    const SymbolTable& getSymbolTable() const { return symbols; }
};

#endif // PARSER_H
