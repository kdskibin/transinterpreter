#include "Interpreter.h"
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <stdexcept>

// Вспомогательная функция для проверки, является ли строка числом
bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0; // Обработка отрицательных чисел
    if (start == 1 && s.length() == 1) return false;
    return s.find_first_not_of("0123456789", start) == std::string::npos;
}

void Interpreter::execute(const std::vector<RpnElement>& rpn) {
    std::stack<std::string> evalStack;
    std::unordered_map<std::string, int> variables; // Наша оперативная память (таблица переменных)

    // Лямбда-функция для извлечения значения (числа или переменной из памяти)
    auto getValue = [&](const std::string& s) -> int {
        if (isNumber(s)) {
            return std::stoi(s);
        }
        // Если переменной еще нет в памяти, инициализируем ее нулем
        if (variables.find(s) == variables.end()) {
            variables[s] = 0; 
        }
        return variables[s];
    };

    // Главный цикл прохода по ленте ОПС (Instruction Pointer)
    for (size_t ip = 0; ip < rpn.size(); ++ip) {
        const std::string& op = rpn[ip].value;

        // Если это метка адреса (из семантических программ), просто кладем в стек
        if (rpn[ip].isLabel) {
            evalStack.push(op);
            continue;
        }

        // 1. Арифметические и логические операции (Бинарные)
        if (op == "+" || op == "-" || op == "*" || op == "/" || 
            op == "<" || op == ">" || op == "<=" || op == ">=" || 
            op == "==" || op == "!=") {
            
            int right = getValue(evalStack.top()); evalStack.pop();
            int left = getValue(evalStack.top()); evalStack.pop();
            
            int result = 0;
            if (op == "+") result = left + right;
            else if (op == "-") result = left - right;
            else if (op == "*") result = left * right;
            else if (op == "/") {
                if (right == 0) throw std::runtime_error("Деление на ноль!");
                result = left / right;
            }
            else if (op == "<") result = (left < right) ? 1 : 0;
            else if (op == ">") result = (left > right) ? 1 : 0;
            else if (op == "<=") result = (left <= right) ? 1 : 0;
            else if (op == ">=") result = (left >= right) ? 1 : 0;
            else if (op == "==") result = (left == right) ? 1 : 0;
            else if (op == "!=") result = (left != right) ? 1 : 0;

            // Кладем результат вычисления обратно в стек
            evalStack.push(std::to_string(result));
        }
        // 2. Операция присваивания
        else if (op == "=" || op == ":=") {
            int val = getValue(evalStack.top()); evalStack.pop();
            std::string varName = evalStack.top(); evalStack.pop();
            variables[varName] = val; // Записываем значение в память
        }
        // 3. Безусловный переход
        else if (op == "j") {
            int addr = std::stoi(evalStack.top()); evalStack.pop();
            ip = addr - 1; // Делаем -1, так как цикл for сделает ++ip
        }
        // 4. Условный переход по лжи (jump if false)
        else if (op == "jf") {
            int addr = std::stoi(evalStack.top()); evalStack.pop();
            int cond = getValue(evalStack.top()); evalStack.pop();
            if (cond == 0) { // Если условие ложно, прыгаем
                ip = addr - 1; 
            }
        }
        // 5. Вывод переменной на экран (опционально)
        else if (op == "Output") {
            int val = getValue(evalStack.top()); evalStack.pop();
            std::cout << ">> " << val << std::endl;
        }
        // 6. Любой другой операнд (число, имя переменной) просто кидаем в стек
        else {
            evalStack.push(op);
        }
    }
}