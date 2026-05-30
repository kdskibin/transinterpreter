#pragma once
#include <vector>
#include <memory>
#include "Terminal.h"
#include "RPNSymbol.h"

// Транслятор в обратную польскую нотацию (RPN).
// Реализует алгоритм сортировочной станции (алгоритм Дейкстры).
class RPNTranslator {
public:
    // Преобразует список терминалов в RPN.
    static std::vector<std::shared_ptr<RPNSymbol>>
    convertToRPN(const std::vector<std::shared_ptr<Terminal>>& inputTerminals);

private:
    using TermVec  = std::vector<std::shared_ptr<Terminal>>;
    using RPNVec   = std::vector<std::shared_ptr<RPNSymbol>>;
    using MarkVec  = std::vector<std::shared_ptr<RPNMark>>;

    static TermVec  _input;
    static RPNVec   _output;
    static MarkVec  _tempMarks;
    static MarkVec  _constMarks;
    static RPNVec   _operationStack;

    // Перевод терминала-операции в символ RPN
    static std::shared_ptr<RPNSymbol> translateToRPNSymbol(const std::shared_ptr<Terminal>& t);
    // Перевод терминала-операнда в символ RPN
    static std::shared_ptr<RPNSymbol> translateOperand(const std::shared_ptr<Terminal>& t);

    // Приоритет символа RPN
    static int getRPNSymbolPriority(const std::shared_ptr<RPNSymbol>& s);

    // Перевод операции объявления переменной в операцию объявления массива
    static ERPNType toArrayInit(const std::shared_ptr<RPNSymbol>& s);

    // Помещение символа в стек операций
    static void toStack(std::shared_ptr<RPNSymbol> s);

    // Запись финальных позиций меток
    static void writeMarks();

    // Предикаты
    static bool isOpeningParenthesis(const std::shared_ptr<Terminal>& t);
    static bool isOperationOrParenthesis(const std::shared_ptr<Terminal>& t);
    static bool isOperand(const std::shared_ptr<Terminal>& t);
    static bool isWritableInOutput(const std::shared_ptr<RPNSymbol>& s);
    static bool isWritableInOperationStack(const std::shared_ptr<RPNSymbol>& s);
    static bool isVariableInitialization(const std::shared_ptr<RPNSymbol>& s);
};
