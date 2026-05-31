#include "Interpreter.h"
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <stdexcept>

bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start == 1 && s.length() == 1) return false;
    return s.find_first_not_of("0123456789", start) == std::string::npos;
}

void Interpreter::execute(const std::vector<RpnElement>& rpn) {
    std::stack<std::string> evalStack;
    std::unordered_map<std::string, int> variables;

    // Улучшенный getValue
    auto getValue = [&](const std::string& s) -> int {
        if (isNumber(s)) {
            return std::stoi(s);
        }
        // ЕСЛИ ПЕРЕМЕННОЙ НЕТ В ПАМЯТИ — БЬЕМ ПО РУКАМ!
        if (variables.find(s) == variables.end()) {
            throw std::runtime_error("Ошибка выполнения: Переменная '" + s + "' не объявлена!");
        }
        return variables[s];
    };

    for (size_t ip = 0; ip < rpn.size(); ++ip) {
        const std::string& op = rpn[ip].value;

        if (rpn[ip].isLabel) {
            evalStack.push(op);
            continue;
        }

        // 1. Арифметические и логические операции
        if (op == "+" || op == "-" || op == "*" || op == "/" ||
            op == "<" || op == ">" || op == "<=" || op == ">=" || 
            op == "==" || op == "!=" || op == "&&" || op == "||") {
            
            int right = getValue(evalStack.top()); evalStack.pop();
            int left = getValue(evalStack.top()); evalStack.pop();
            
            int result = 0;
            if (op == "+") result = left + right;
            else if (op == "-") result = left - right;
            else if (op == "*") result = left * right;
            else if (op == "/") {
                if (right == 0) throw std::runtime_error("Ошибка времени выполнения: Деление на ноль!");
                result = left / right;
            }
            else if (op == "<") result = (left < right) ? 1 : 0;
            else if (op == ">") result = (left > right) ? 1 : 0;
            else if (op == "<=") result = (left <= right) ? 1 : 0;
            else if (op == ">=") result = (left >= right) ? 1 : 0;
            else if (op == "==") result = (left == right) ? 1 : 0;
            else if (op == "!=") result = (left != right) ? 1 : 0;
            else if (op == "&&") result = (left && right) ? 1 : 0;
            else if (op == "||") result = (left || right) ? 1 : 0;

            evalStack.push(std::to_string(result));
        }
        // 2. Операция логического НЕ
        else if (op == "!") {
            int val = getValue(evalStack.top()); evalStack.pop();
            evalStack.push(std::to_string(!val));
        }
        // 3. Операция присваивания
// 2. Операция присваивания
        else if (op == "=" || op == ":=") {
            int val = getValue(evalStack.top()); evalStack.pop();
            std::string varName = evalStack.top(); evalStack.pop();
            
            // Проверяем, существует ли переменная перед записью в неё
            if (variables.find(varName) == variables.end()) {
                throw std::runtime_error("Ошибка выполнения: Попытка присвоить значение необъявленной переменной '" + varName + "'!");
            }
            
            variables[varName] = val; // Записываем значение в память
        }
        // 4. Безусловный переход
        else if (op == "j") {
            int addr = std::stoi(evalStack.top()); evalStack.pop();
            ip = addr - 1; 
        }
        // 5. Условный переход по лжи (jump if false)
        else if (op == "jf") {
            int addr = std::stoi(evalStack.top()); evalStack.pop();
            int cond = getValue(evalStack.top()); evalStack.pop();
            if (cond == 0) { 
                ip = addr - 1; 
            }
        }
        // 6. Вывод переменной на экран
        else if (op == "Output" || op == "output") {
            int val = getValue(evalStack.top()); evalStack.pop();
            std::cout << val << std::endl;
        }

        // Новая команда: Объявление переменной. В стеке лежит [Имя переменной]
        else if (op == "decl_var") {
            std::string varName = evalStack.top(); evalStack.pop();
            
            // Проверяем на повторное объявление (например, int a; int a;)
            if (variables.find(varName) != variables.end()) {
                throw std::runtime_error("Ошибка выполнения: Переменная '" + varName + "' уже объявлена!");
            }
            
            variables[varName] = 0; // Выделяем память и инициализируем нулем по умолчанию
        }
        // 7. Операнды
        else {
            evalStack.push(op);
        }
    }
}