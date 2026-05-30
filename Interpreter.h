#pragma once
#include <vector>
#include "LL1parser.h" // Подключаем, чтобы компилятор знал структуру RpnElement

class Interpreter {
public:
    // Главная функция выполнения ОПС
    static void execute(const std::vector<RpnElement>& rpn);
};