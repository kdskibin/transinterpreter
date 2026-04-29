#pragma once
#include "Enums.h"
#include <string>
#include <variant>
#include <stdexcept>


class Terminal {
public:
    ETerminalType TerminalType;
    int CharPointer;
    int LinePointer;
    
    // Хранилище значения токена:
    // int - для чисел, bool - для булевых литералов,
    // std::string - для строк и идентификаторов,
    // std::monostate - для токенов без значения.
    std::variant<std::monostate, int, bool, std::string> Value;
    
    // Токен без значения (операторы, скобки, ключевые слова и т.п.)
    Terminal(ETerminalType type, int line, int charPos)
        : TerminalType(type), LinePointer(line), CharPointer(charPos), Value(std::monostate{}) {}
    
    // Числовой литерал
    Terminal(ETerminalType type, int line, int charPos, int numValue)
        : TerminalType(type), LinePointer(line), CharPointer(charPos), Value(numValue) {
        if (type != ETerminalType::Number)
            throw std::invalid_argument("Terminal type mismatch for Number");
    }
    
    // Булевый литерал
    Terminal(ETerminalType type, int line, int charPos, bool boolValue)
        : TerminalType(type), LinePointer(line), CharPointer(charPos), Value(boolValue) {
        if (type != ETerminalType::Boolean)
            throw std::invalid_argument("Terminal type mismatch for Boolean");
    }
    
    // Строковый литерал или имя переменной
    Terminal(ETerminalType type, int line, int charPos, const std::string& strValue)
        : TerminalType(type), LinePointer(line), CharPointer(charPos), Value(strValue) {
        if (type != ETerminalType::TextLine && type != ETerminalType::VariableName)
            throw std::invalid_argument("Terminal type mismatch for string value");
    }
    
    // Вспомогательные методы доступа с проверкой типа (опционально)
    int getNumber() const {
        if (TerminalType != ETerminalType::Number) throw std::runtime_error("Not a number terminal");
        return std::get<int>(Value);
    }
    bool getBoolean() const {
        if (TerminalType != ETerminalType::Boolean) throw std::runtime_error("Not a boolean terminal");
        return std::get<bool>(Value);
    }
    const std::string& getString() const {
        if (TerminalType != ETerminalType::TextLine && TerminalType != ETerminalType::VariableName)
            throw std::runtime_error("Not a string terminal");
        return std::get<std::string>(Value);
    }
};