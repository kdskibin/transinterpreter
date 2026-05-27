#pragma once
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "RPNSymbol.h"

// Интерпретатор RPN: выполняет список инструкций обратной польской нотации.
class RPNInterpreter {
public:
    // Выполняет список RPN-инструкций.
    // Выбрасывает std::runtime_error при ошибке выполнения.
    static void executeInstructions(const std::vector<std::shared_ptr<RPNSymbol>>& rpn);

private:
    // Вспомогательный метод: извлекает числовое значение из операнда (число, переменная, элемент массива)
    static int resolveNumberOperand(std::shared_ptr<RPNSymbol> sym,
                                     const std::map<std::string, std::vector<std::string>>& variables);
};
