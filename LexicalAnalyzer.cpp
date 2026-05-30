#include "LexicalAnalyzer.h"
#include <iostream>
#include <stdexcept>
#include <functional>
#include <unordered_map>

// Инициализация статических членов
std::string LexicalAnalyzer::_data;
std::vector<std::shared_ptr<Terminal>> LexicalAnalyzer::_terminals;
int LexicalAnalyzer::_charPointer = 1;
int LexicalAnalyzer::_linePointer = 1;
int LexicalAnalyzer::_char        = 1;
int LexicalAnalyzer::_pointer     = 0;

// ────────────────────────────────────────────────────────────
//  Вспомогательные методы
// ────────────────────────────────────────────────────────────

char LexicalAnalyzer::currentChar() {
    return _data[_pointer];
}

void LexicalAnalyzer::advancePointer() {
    if (_pointer >= static_cast<int>(_data.size()))
        return;
    _pointer++;
    _charPointer++;
    if (_pointer < static_cast<int>(_data.size()) && _data[_pointer] == '\n') {
        _linePointer++;
        _charPointer = 0;
        _char = 1;
    }
}

void LexicalAnalyzer::readTerminal(ETerminalType type) {
    _terminals.push_back(std::make_shared<Terminal>(type, _linePointer, _char));
}

void LexicalAnalyzer::readTerminalWithValue(ETerminalType type, const std::string& value) {
    switch (type) {
        case ETerminalType::Number:
            _terminals.push_back(std::make_shared<TerminalNumber>(_linePointer, _char, value));
            break;
        case ETerminalType::TextLine:
            _terminals.push_back(std::make_shared<TerminalTextLine>(_linePointer, _char, value));
            break;
        case ETerminalType::Boolean:
            _terminals.push_back(std::make_shared<TerminalBoolean>(_linePointer, _char, value));
            break;
        case ETerminalType::VariableName:
            _terminals.push_back(std::make_shared<TerminalIdentifier>(_linePointer, _char, value));
            break;
        default:
            throw std::runtime_error("Невозможный тип терминала");
    }
}

std::string LexicalAnalyzer::currentCharGroup() {
    char c = currentChar();

    if (c >= '0' && c <= '9')                          return "<ц>";
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') || c == '_')            return "<б>";
    if (c == '"')  return "<\">";
    if (c == ' ' || c == '\n') return "< >";
    if (c == ';')  return "<;>";
    if (c == '+')  return "<+>";
    if (c == '-')  return "<->";
    if (c == '*')  return "<*>";
    if (c == '/')  return "</>";
    if (c == '%')  return "<%>";
    if (c == '<')  return "<<>";
    if (c == '>')  return "<>>";
    if (c == '=')  return "<=>";
    if (c == '&')  return "<&>";
    if (c == '|')  return "<|>";
    if (c == '!')  return "<!>";
    if (c == '(')  return "<(>";
    if (c == ')')  return "<)>";
    if (c == '[')  return "<[>";
    if (c == ']')  return "<]>";
    if (c == '{')  return "<{>";
    if (c == '}')  return "<}>";

    // Кириллица и прочие допустимые символы внутри строковых литералов обрабатываются в STR_Analyse,
    // здесь они попадают только если встречаются вне строк — это ошибка
    std::cout << "Некорректный символ: " << c
              << "\tСтрока " << _linePointer
              << ";\tСимвол " << _charPointer << ";\n";
    throw std::out_of_range(std::string("символ '") + c + "' недопустим в грамматике");
}

// ────────────────────────────────────────────────────────────
//  Таблица переходов (встроена как unordered_map)
// ────────────────────────────────────────────────────────────

static std::unordered_map<std::string, std::function<void()>>& getTransitionTable() {
    static std::unordered_map<std::string, std::function<void()>> table = {
        { "<ц>",  []{ LexicalAnalyzer::NUM_Analyse(); } },
        { "<б>",  []{ LexicalAnalyzer::ID_Analyse();  } },
        { "< >",  []{ LexicalAnalyzer::skipWhitespace(); } },
        { "<\">", []{ LexicalAnalyzer::STR_Analyse(); } },
        { "<;>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Semicolon); } },
        { "<+>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Plus);      } },
        { "<->",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Minus);     } },
        { "<*>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Multiply);  } },
        { "</>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Divide);    } },
        { "<%>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Modulus);   } },
        { "<<>",  []{ LexicalAnalyzer::LESS_Analyse();  } },
        { "<>>",  []{ LexicalAnalyzer::MORE_Analyse();  } },
        { "<=>",  []{ LexicalAnalyzer::EQUAL_Analyse(); } },
        { "<!>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::Not);        } },
        { "<&>",  []{ LexicalAnalyzer::AND_Analyse();   } },
        { "<|>",  []{ LexicalAnalyzer::OR_Analyse();    } },
        { "<(>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::LeftParen);  } },
        { "<)>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::RightParen); } },
        { "<[>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::LeftBracket);  } },
        { "<]>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::RightBracket); } },
        { "<{>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::LeftBrace);  } },
        { "<}>",  []{ LexicalAnalyzer::processSimpleToken(ETerminalType::RightBrace); } },
    };
    return table;
}

// ────────────────────────────────────────────────────────────
//  Главный цикл анализа
// ────────────────────────────────────────────────────────────

void LexicalAnalyzer::startAnalyse() {
    _char = _charPointer;
    std::string group = currentCharGroup();

    auto& table = getTransitionTable();
    auto it = table.find(group);
    if (it != table.end()) {
        it->second();
    } else {
        std::cout << "Некорректный символ: " << currentChar()
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";\n";
        throw std::runtime_error("Недопустимый символ.");
    }
}

bool LexicalAnalyzer::isLexicalCorrect(const std::string& data) {
    _data        = data;
    _pointer     = 0;
    _charPointer = 1;
    _linePointer = 1;
    _char        = 1;
    _terminals.clear();

    while (_pointer < static_cast<int>(_data.size())) {
        startAnalyse();
    }
    return true;
}

std::vector<std::shared_ptr<Terminal>> LexicalAnalyzer::getTerminals() {
    return _terminals;
}

// ────────────────────────────────────────────────────────────
//  Методы анализа отдельных категорий токенов
// ────────────────────────────────────────────────────────────

void LexicalAnalyzer::skipWhitespace() {
    advancePointer();
}

void LexicalAnalyzer::processSimpleToken(ETerminalType type) {
    readTerminal(type);
    advancePointer();
}

// Анализ числового литерала
void LexicalAnalyzer::NUM_Analyse() {
    std::string number;
    do {
        number += currentChar();
        advancePointer();
    } while (_pointer < static_cast<int>(_data.size()) && currentCharGroup() == "<ц>");

    readTerminalWithValue(ETerminalType::Number, number);
}

// Анализ идентификатора или ключевого слова
void LexicalAnalyzer::ID_Analyse() {
    std::string identifier;
    do {
        identifier += currentChar();
        advancePointer();
    } while (_pointer < static_cast<int>(_data.size()) &&
             (currentCharGroup() == "<ц>" || currentCharGroup() == "<б>"));

    // Проверяем, является ли идентификатор ключевым словом
    if      (identifier == "while")  readTerminal(ETerminalType::While);
    else if (identifier == "if")     readTerminal(ETerminalType::If);
    else if (identifier == "else")   readTerminal(ETerminalType::Else);
    else if (identifier == "int")    readTerminal(ETerminalType::Int);
    else if (identifier == "string") readTerminal(ETerminalType::String);
    else if (identifier == "bool")   readTerminal(ETerminalType::Bool);
    else if (identifier == "output") readTerminal(ETerminalType::Output);
    else if (identifier == "input")  readTerminal(ETerminalType::Input);
    else if (identifier == "sqrt")   readTerminal(ETerminalType::Sqrt);
    else if (identifier == "true" || identifier == "false")
        readTerminalWithValue(ETerminalType::Boolean, identifier);
    else
        readTerminalWithValue(ETerminalType::VariableName, identifier);
}

// Анализ строкового литерала в двойных кавычках
void LexicalAnalyzer::STR_Analyse() {
    std::string text;
    advancePointer(); // пропускаем открывающую кавычку

    while (_pointer < static_cast<int>(_data.size()) && currentChar() != '"') {
        text += currentChar();
        advancePointer();
    }

    if (_pointer < static_cast<int>(_data.size()) && currentChar() == '"') {
        advancePointer(); // пропускаем закрывающую кавычку
        readTerminalWithValue(ETerminalType::TextLine, text);
    } else {
        std::cout << "Незакрытая строка:"
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";\n";
        throw std::runtime_error("Незакрытая строка.");
    }
}

// Анализ оператора '<' или '<='
void LexicalAnalyzer::LESS_Analyse() {
    advancePointer();
    if (_pointer < static_cast<int>(_data.size()) && currentChar() == '=') {
        readTerminal(ETerminalType::LessEqual);
        advancePointer();
    } else {
        readTerminal(ETerminalType::Less);
    }
}

// Анализ оператора '>' или '>='
void LexicalAnalyzer::MORE_Analyse() {
    advancePointer();
    if (_pointer < static_cast<int>(_data.size()) && currentChar() == '=') {
        readTerminal(ETerminalType::GreaterEqual);
        advancePointer();
    } else {
        readTerminal(ETerminalType::Greater);
    }
}

// Анализ оператора '=' или '=='
void LexicalAnalyzer::EQUAL_Analyse() {
    advancePointer();
    if (_pointer < static_cast<int>(_data.size()) && currentChar() == '=') {
        readTerminal(ETerminalType::Equal);
        advancePointer();
    } else {
        readTerminal(ETerminalType::Assignment);
    }
}

// Анализ оператора '&&'
void LexicalAnalyzer::AND_Analyse() {
    advancePointer();
    if (_pointer < static_cast<int>(_data.size()) && currentChar() == '&') {
        readTerminal(ETerminalType::And);
        advancePointer();
    } else {
        std::cout << "Некорректный оператор: одиночный '&'"
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";\n";
        throw std::runtime_error("Некорректный оператор: одиночный '&'. Используйте '&&'.");
    }
}

// Анализ оператора '||'
void LexicalAnalyzer::OR_Analyse() {
    advancePointer();
    if (_pointer < static_cast<int>(_data.size()) && currentChar() == '|') {
        readTerminal(ETerminalType::Or);
        advancePointer();
    } else {
        std::cout << "Некорректный оператор: одиночный '|'"
                  << "\tСтрока " << _linePointer
                  << ";\tСимвол " << _charPointer << ";\n";
        throw std::runtime_error("Некорректный оператор: одиночный '|'. Используйте '||'.");
    }
}
