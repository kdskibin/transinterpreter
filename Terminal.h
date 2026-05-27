#pragma once
#include <string>
#include <stdexcept>
#include "Enums.h"

// Базовый класс для всех терминалов (токенов) лексического анализатора
class Terminal {
public:
    ETerminalType terminalType;
    int charPointer = 0;  // Позиция символа в строке
    int linePointer = 0;  // Номер строки

    Terminal(ETerminalType type, int linePtr, int charPtr)
        : terminalType(type), linePointer(linePtr), charPointer(charPtr) {}

    virtual ~Terminal() = default;
};

// Строковый литерал
class TerminalTextLine : public Terminal {
public:
    std::string data;

    TerminalTextLine(int linePtr, int charPtr, const std::string& val)
        : Terminal(ETerminalType::TextLine, linePtr, charPtr), data(val) {}
};

// Целочисленный литерал
class TerminalNumber : public Terminal {
public:
    int data;

    TerminalNumber(int linePtr, int charPtr, const std::string& val)
        : Terminal(ETerminalType::Number, linePtr, charPtr), data(std::stoi(val)) {}
};

// Булевый литерал
class TerminalBoolean : public Terminal {
public:
    bool data;

    TerminalBoolean(int linePtr, int charPtr, const std::string& val)
        : Terminal(ETerminalType::Boolean, linePtr, charPtr), data(val == "true") {}
};

// Идентификатор (имя переменной)
class TerminalIdentifier : public Terminal {
public:
    std::string name;

    TerminalIdentifier(int linePtr, int charPtr, const std::string& n)
        : Terminal(ETerminalType::VariableName, linePtr, charPtr), name(n) {}
};