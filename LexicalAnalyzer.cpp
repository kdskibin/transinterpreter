#include "LexicalAnalyzer.h"
#include "TransitionTable.h"
#include <stdexcept>
#include <iostream>
#include <cctype>


std::string LexicalAnalyzer::Data;
size_t LexicalAnalyzer::_pointer = 0;
int LexicalAnalyzer::_charPointer = 1;
int LexicalAnalyzer::_linePointer = 1;
int LexicalAnalyzer::_char = 1;
std::vector<Terminal> LexicalAnalyzer::Terminals;

bool LexicalAnalyzer::IsLexicalCorrect(const std::string& data) {
    Data = data;
    _pointer = 0;
    _charPointer = 1;
    _linePointer = 1;
    _char = 1;
    Terminals.clear();

    while (_pointer < Data.size()) {
        Start_Analyse();
    }
    return true;
}

std::vector<Terminal> LexicalAnalyzer::GetTerminals() {
    return Terminals;
}

char LexicalAnalyzer::CurrentChar() {
    return Data[_pointer];
}

void LexicalAnalyzer::Advance() {
    ++_pointer;
    ++_charPointer;
    if (_pointer < Data.size() && Data[_pointer] == '\n') {
        ++_linePointer;
        _charPointer = 0;
        _char = 1;
    }
}

std::string LexicalAnalyzer::CurrentCharGroup() {
    char c = CurrentChar();
    if (c >= '0' && c <= '9')
        return "<ц>";
    else if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' )
        return "<б>";
    else if (c == '"') return "<\">";
    else if (c == ' ' || c == '\n') return "< >";
    else if (c == ';') return "<;>";
    else if (c == '+') return "<+>";
    else if (c == '-') return "<->";
    else if (c == '*') return "<*>";
    else if (c == '/') return "</>";
    else if (c == '%') return "<%>";
    else if (c == '<') return "<<>";
    else if (c == '>') return "<>>";
    else if (c == '=') return "<=>";
    else if (c == '&') return "<&>";
    else if (c == '|') return "<|>";
    else if (c == '!') return "<!>";
    else if (c == '(') return "<(>";
    else if (c == ')') return "<)>";
    else if (c == '[') return "<[>";
    else if (c == ']') return "<]>";
    else if (c == '{') return "<{>";
    else if (c == '}') return "<}>";
    else if ( (c >= 'а' && c <= 'я') || (c >= 'А' && c <= 'Я') || c == '?' || c == ',' || c == '.' )
        return "<o>";
    else {
        std::cerr << "Некорректный символ: " << c
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";" << std::endl;
        throw std::invalid_argument(std::string("символ \"") + c + "\" недопустим в грамматике");
    }
}

void LexicalAnalyzer::Start_Analyse() {
    _char = _charPointer;
    std::string charGroup = CurrentCharGroup();
    TransitionTable::Action action;
    if (TransitionTable::TryGetAction(charGroup, action)) {
        action();
    } else {
        std::cerr << "Некорректный символ: " << CurrentChar()
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";" << std::endl;
        throw std::runtime_error("Недопустимый символ.");
    }
}

void LexicalAnalyzer::ReadTerminal(ETerminalType type) {
    Terminals.emplace_back(type, _linePointer, _char);
}

void LexicalAnalyzer::ReadTerminal(ETerminalType type, const std::string& value) {
    switch (type) {
        case ETerminalType::Number: {
            int num = std::stoi(value);
            Terminals.emplace_back(type, _linePointer, _char, num);
            break;
        }
        case ETerminalType::TextLine:
            Terminals.emplace_back(type, _linePointer, _char, value);
            break;
        case ETerminalType::Boolean: {
            bool b;
            if (value == "true") b = true;
            else if (value == "false") b = false;
            else throw std::invalid_argument("Invalid boolean value: " + value);
            Terminals.emplace_back(type, _linePointer, _char, b);
            break;
        }
        case ETerminalType::VariableName:
            Terminals.emplace_back(type, _linePointer, _char, value);
            break;
        default:
            throw std::logic_error("Невозможный тип терминала");
    }
}

void LexicalAnalyzer::SkipWhitespace() {
    Advance();
}

void LexicalAnalyzer::ProcessSimpleToken(ETerminalType terminalType) {
    ReadTerminal(terminalType);
    Advance();
}

void LexicalAnalyzer::NUM_Analyse() {
    std::string number;
    while (_pointer < Data.size() && CurrentChar() >= '0' && CurrentChar() <= '9') {
        number += CurrentChar();
        Advance();
    }
    ReadTerminal(ETerminalType::Number, number);
}

void LexicalAnalyzer::ID_Analyse() {
    std::string identifier;
    while (_pointer < Data.size() &&
           ((CurrentChar() >= 'a' && CurrentChar() <= 'z') ||
            (CurrentChar() >= 'A' && CurrentChar() <= 'Z') ||
            (CurrentChar() >= '0' && CurrentChar() <= '9') ||
            CurrentChar() == '_')) {
        identifier += CurrentChar();
        Advance();
    }
    // Проверка ключевых слов
    if (identifier == "while") {
        ReadTerminal(ETerminalType::While);
    } else if (identifier == "if") {
        ReadTerminal(ETerminalType::If);
    } else if (identifier == "else") {
        ReadTerminal(ETerminalType::Else);
    } else if (identifier == "int") {
        ReadTerminal(ETerminalType::Int);
    } else if (identifier == "string") {
        ReadTerminal(ETerminalType::String);
    } else if (identifier == "bool") {
        ReadTerminal(ETerminalType::Bool);
    } else if (identifier == "output") {
        ReadTerminal(ETerminalType::Output);
    } else if (identifier == "input") {
        ReadTerminal(ETerminalType::Input);
    } else if (identifier == "true" || identifier == "false") {
        ReadTerminal(ETerminalType::Boolean, identifier);
    } else if (identifier == "sqrt") {
        ReadTerminal(ETerminalType::Sqrt);
    } else {
        ReadTerminal(ETerminalType::VariableName, identifier);
    }
}

void LexicalAnalyzer::STR_Analyse() {
    std::string textLine;
    Advance(); // пропуск открывающей кавычки
    while (_pointer < Data.size() && CurrentChar() != '"') {
        textLine += CurrentChar();
        Advance();
    }
    if (_pointer < Data.size() && CurrentChar() == '"') {
        Advance(); // пропуск закрывающей кавычки
        ReadTerminal(ETerminalType::TextLine, textLine);
    } else {
        std::cerr << "Незакрытая строка: "
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";" << std::endl;
        throw std::runtime_error("Незакрытая строка.");
    }
}

void LexicalAnalyzer::LESS_Analyse() {
    Advance(); // пропуск '<'
    if (_pointer < Data.size() && CurrentChar() == '=') {
        ReadTerminal(ETerminalType::LessEqual);
        Advance();
    } else {
        ReadTerminal(ETerminalType::Less);
    }
}

void LexicalAnalyzer::MORE_Analyse() {
    Advance(); // пропуск '>'
    if (_pointer < Data.size() && CurrentChar() == '=') {
        ReadTerminal(ETerminalType::GreaterEqual);
        Advance();
    } else {
        ReadTerminal(ETerminalType::Greater);
    }
}

void LexicalAnalyzer::EQUAL_Analyse() {
    Advance(); // пропуск первого '='
    if (_pointer < Data.size() && CurrentChar() == '=') {
        ReadTerminal(ETerminalType::Equal);
        Advance();
    } else {
        ReadTerminal(ETerminalType::Assignment);
    }
}

void LexicalAnalyzer::AND_Analyse() {
    Advance(); // пропуск первого '&'
    if (_pointer < Data.size() && CurrentChar() == '&') {
        ReadTerminal(ETerminalType::And);
        Advance();
    } else {
        std::cerr << "Некорректный оператор: одиночный '&'"
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";" << std::endl;
        throw std::runtime_error("Некорректный оператор: одиночный '&'. Используйте '&&'.");
    }
}

void LexicalAnalyzer::OR_Analyse() {
    Advance(); // пропуск первого '|'
    if (_pointer < Data.size() && CurrentChar() == '|') {
        ReadTerminal(ETerminalType::Or);
        Advance();
    } else {
        std::cerr << "Некорректный оператор: одиночный '|'"
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";" << std::endl;
        throw std::runtime_error("Некорректный оператор: одиночный '|'. Используйте '||'.");
    }
}