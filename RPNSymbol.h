#pragma once
#include <string>
#include <optional>
#include "Enums.h"

// Базовый класс для всех символов RPN
class RPNSymbol {
public:
    ERPNType rpnType;
    int charPointer = 0;
    int linePointer = 0;

    explicit RPNSymbol(ERPNType type) : rpnType(type) {}
    virtual ~RPNSymbol() = default;
};

// Метка перехода (для if/else/while)
class RPNMark : public RPNSymbol {
public:
    bool isFinal = false;
    EMarkType markType;
    std::optional<int> position; // Индекс в Output, на который указывает метка

    explicit RPNMark(EMarkType mType)
        : RPNSymbol(ERPNType::M_Mark), markType(mType) {}
};

// Строковый литерал в RPN
class RPNTextLine : public RPNSymbol {
public:
    std::string data;

    RPNTextLine() : RPNSymbol(ERPNType::A_TextLine) {}
    explicit RPNTextLine(const std::string& val)
        : RPNSymbol(ERPNType::A_TextLine), data(val) {}
    RPNTextLine(ERPNType type, const std::string& val)
        : RPNSymbol(type), data(val) {}
};

// Числовой литерал в RPN
class RPNNumber : public RPNSymbol {
public:
    int data = 0;

    RPNNumber() : RPNSymbol(ERPNType::A_Number) {}
    explicit RPNNumber(int val) : RPNSymbol(ERPNType::A_Number), data(val) {}
    RPNNumber(ERPNType type, int val) : RPNSymbol(type), data(val) {}
};

// Булевый литерал в RPN
class RPNBoolean : public RPNSymbol {
public:
    bool data = false;

    RPNBoolean() : RPNSymbol(ERPNType::A_Boolean) {}
    explicit RPNBoolean(bool val) : RPNSymbol(ERPNType::A_Boolean), data(val) {}
    RPNBoolean(ERPNType type, bool val) : RPNSymbol(type), data(val) {}
};

// Идентификатор переменной в RPN
class RPNIdentifier : public RPNSymbol {
public:
    std::string name;

    RPNIdentifier() : RPNSymbol(ERPNType::A_VariableName) {}
    explicit RPNIdentifier(const std::string& n)
        : RPNSymbol(ERPNType::A_VariableName), name(n) {}
};

// Доступ к элементу массива по индексу
class RPNArrayAccess : public RPNSymbol {
public:
    std::string arrayName;
    int index = 0;

    RPNArrayAccess() : RPNSymbol(ERPNType::F_Index) {}
    RPNArrayAccess(const std::string& arr, int idx)
        : RPNSymbol(ERPNType::F_Index), arrayName(arr), index(idx) {}
};
