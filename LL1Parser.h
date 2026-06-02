#pragma once
#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include <memory>
#include "LexicalAnalyzer.h" // Подключаем ваш адаптированный лексер

// Нетерминалы вашей грамматики (согласно Report.md)
enum class ENonTerminal {
    Program,       // P
    StmtList,      // L
    Statement,     // A
    Expression,    // S
    ExpressionPrime, // U
    Term,          // T
    TermPrime,     // Y
    Factor,        // F
    Condition,     // C
    ConditionPrime // D
};

// Тип элемента, находящегося в управляющем магазине парсера
enum class EStackItemType {
    Terminal,
    NonTerminal,
    SemanticAction  // Маркер вызова семантической программы генератора ОПС
};

// Структура ячейки главного магазина
struct StackItem {
    EStackItemType type;
    int id; // Хранит целочисленное значение из ETerminalType, ENonTerminal или ID программы
};

// Элемент выходной ленты ОПС
struct RpnElement {
    std::string value;
    bool isLabel;      // Флаг: является ли элемент адресом/меткой перехода
};

class LL1Parser {
public:
    LL1Parser();
    
    // Главная точка входа для синтаксического анализа и генерации ОПС
    void parse(const std::string& sourceCode);
    
    // Вывод полученной ОПС в консоль
    void printRpn() const;

    const std::vector<RpnElement>& getRpn() const { return _rpn; }

private:
    std::stack<StackItem> _parseStack;  // Главный магазин синтаксического анализатора
    std::stack<int> _labelStack;        // Магазин меток (семантический стек для вложенных переходов)
    std::vector<RpnElement> _rpn;       // Результирующий массив ОПС

    // Управляющая таблица LL(1): [Нетерминал][Тип_Терминала] -> Правая часть правила (цепочка символов)
    std::unordered_map<ENonTerminal, std::unordered_map<ETerminalType, std::vector<StackItem>>> _parsingTable;

    void initParsingTable();
    void executeSemanticAction(int actionId);
};