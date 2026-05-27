#include "SyntacticalAnalyzer.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Инициализация статических членов
std::string SyntacticalAnalyzer::_log;
int         SyntacticalAnalyzer::_logCounter = 0;
std::string SyntacticalAnalyzer::_tabs;
int         SyntacticalAnalyzer::_tabsCounter = 0;

// ────────────────────────────────────────────────────────────
//  Вспомогательные методы
// ────────────────────────────────────────────────────────────

void SyntacticalAnalyzer::incTabs() {
    _tabs += '\t';
    _tabsCounter++;
}

void SyntacticalAnalyzer::decTabs() {
    if (!_tabs.empty()) _tabs.erase(0, 1);
    if (_tabsCounter > 0) _tabsCounter--;
}

void SyntacticalAnalyzer::appendLog(const std::string& msg) {
    std::ostringstream ss;
    ss << std::setw(5) << _logCounter << _tabs << msg << "\n";
    _log += ss.str();
    _logCounter++;
}

// Вспомогательный доступ к типу токена с проверкой границ
ETerminalType SyntacticalAnalyzer::typeAt(int idx, const TermVec& t) {
    return t[idx]->terminalType;
}

bool SyntacticalAnalyzer::typeIs(int idx, const TermVec& t, ETerminalType expected) {
    if (idx < 0 || idx >= static_cast<int>(t.size())) return false;
    return t[idx]->terminalType == expected;
}

// Поиск первого вхождения типа токена; возвращает -1 если не найдено
int SyntacticalAnalyzer::findFirst(const TermVec& t, ETerminalType type) {
    for (int i = 0; i < static_cast<int>(t.size()); i++)
        if (t[i]->terminalType == type) return i;
    return -1;
}

// Срез [from, to)
SyntacticalAnalyzer::TermVec SyntacticalAnalyzer::slice(const TermVec& t, int from, int to) {
    if (from >= to || from >= static_cast<int>(t.size())) return {};
    to = std::min(to, static_cast<int>(t.size()));
    return TermVec(t.begin() + from, t.begin() + to);
}

// Срез [from, end)
SyntacticalAnalyzer::TermVec SyntacticalAnalyzer::sliceFrom(const TermVec& t, int from) {
    if (from >= static_cast<int>(t.size())) return {};
    return TermVec(t.begin() + from, t.end());
}

// Поиск парной закрывающей скобки для скобки по индексу leftIdx
int SyntacticalAnalyzer::findPairedClosingBracket(int leftIdx, const TermVec& t) {
    ETerminalType opening = t[leftIdx]->terminalType;
    ETerminalType closing;
    switch (opening) {
        case ETerminalType::LeftParen:   closing = ETerminalType::RightParen;   break;
        case ETerminalType::LeftBrace:   closing = ETerminalType::RightBrace;   break;
        case ETerminalType::LeftBracket: closing = ETerminalType::RightBracket; break;
        default: return -1;
    }
    int counter = 0;
    for (int i = leftIdx; i < static_cast<int>(t.size()); i++) {
        if (t[i]->terminalType == opening)  counter++;
        if (t[i]->terminalType == closing) {
            counter--;
            if (counter == 0) return i;
        }
    }
    return -1;
}

// ────────────────────────────────────────────────────────────
//  Точка входа
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::isSyntacticalCorrect(const TermVec& terminals) {
    _log.clear();
    _logCounter  = 0;
    _tabs.clear();
    _tabsCounter = 0;

    bool result = parseInstructionBlock(terminals);
    std::ofstream logFile("SAlog.txt");
    logFile << _log;
    return result;
}

// ────────────────────────────────────────────────────────────
//  1. Блок инструкций
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseInstructionBlock(const TermVec& t) {
    incTabs(); appendLog("1. <Блок инструкций> →");
    incTabs();

    // 1.1  while ( <Лог.ИЛИ> ) { <Блок> } <Следующая>
    appendLog("1.1 while ( <Лог.ИЛИ> ) { <Блок> } <Следующая> →");
    if (typeIs(0, t, ETerminalType::While)) {
        if (typeIs(1, t, ETerminalType::LeftParen)) {
            int rp = findPairedClosingBracket(1, t);
            if (rp != -1 && typeIs(rp + 1, t, ETerminalType::LeftBrace)) {
                int rb = findPairedClosingBracket(rp + 1, t);
                if (rb != -1) {
                    if (parseLogicalOR(slice(t, 2, rp)) &&
                        parseInstructionBlock(slice(t, rp + 2, rb)) &&
                        parseFollowingInstruction(sliceFrom(t, rb + 1))) {
                        appendLog("1.1 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
                        return true;
                    }
                }
            }
        }
    }
    appendLog("1.1 → FALSE");

    // 1.2  if ( <Лог.ИЛИ> ) { <Блок> } <Следующая>
    appendLog("1.2 if ( <Лог.ИЛИ> ) { <Блок> } <Следующая> →");
    if (typeIs(0, t, ETerminalType::If)) {
        if (typeIs(1, t, ETerminalType::LeftParen)) {
            int rp = findPairedClosingBracket(1, t);
            if (rp != -1 && typeIs(rp + 1, t, ETerminalType::LeftBrace)) {
                int rb = findPairedClosingBracket(rp + 1, t);
                if (rb != -1) {
                    if (parseLogicalOR(slice(t, 2, rp)) &&
                        parseInstructionBlock(slice(t, rp + 2, rb)) &&
                        parseFollowingInstruction(sliceFrom(t, rb + 1))) {
                        appendLog("1.2 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
                        return true;
                    }
                }
            }
        }
    }
    appendLog("1.2 → FALSE");

    // 1.3  if ( <Лог.ИЛИ> ) { <Блок> } else { <Блок> } <Следующая>
    appendLog("1.3 if...else → →");
    if (typeIs(0, t, ETerminalType::If)) {
        if (typeIs(1, t, ETerminalType::LeftParen)) {
            int rp = findPairedClosingBracket(1, t);
            if (rp != -1 && typeIs(rp + 1, t, ETerminalType::LeftBrace)) {
                int rb1 = findPairedClosingBracket(rp + 1, t);
                if (rb1 != -1 && typeIs(rb1 + 1, t, ETerminalType::Else) &&
                    typeIs(rb1 + 2, t, ETerminalType::LeftBrace)) {
                    int rb2 = findPairedClosingBracket(rb1 + 2, t);
                    if (rb2 != -1) {
                        if (parseLogicalOR(slice(t, 2, rp)) &&
                            parseInstructionBlock(slice(t, rp + 2, rb1)) &&
                            parseInstructionBlock(slice(t, rb1 + 3, rb2)) &&
                            parseFollowingInstruction(sliceFrom(t, rb2 + 1))) {
                            appendLog("1.3 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
                            return true;
                        }
                    }
                }
            }
        }
    }
    appendLog("1.3 → FALSE");

    // 1.4  input(<Идентификатор>) ; <Следующая>
    appendLog("1.4 input(<Ид>) ; <Следующая> →");
    if (typeIs(0, t, ETerminalType::Input) && typeIs(1, t, ETerminalType::LeftParen)) {
        int rp = findPairedClosingBracket(1, t);
        if (rp != -1 && typeIs(rp + 1, t, ETerminalType::Semicolon)) {
            if (parseIdentifier(slice(t, 2, rp)) &&
                parseFollowingInstruction(sliceFrom(t, rp + 2))) {
                appendLog("1.4 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
                return true;
            }
        }
    }
    appendLog("1.4 → FALSE");

    // 1.5  output(<Логическое выражение>) ; <Следующая>
    appendLog("1.5 output(<ЛогВыр>) ; <Следующая> →");
    if (typeIs(0, t, ETerminalType::Output) && typeIs(1, t, ETerminalType::LeftParen)) {
        int rp = findPairedClosingBracket(1, t);
        if (rp != -1 && typeIs(rp + 1, t, ETerminalType::Semicolon)) {
            if (parseIdentifier(slice(t, 2, rp)) &&
                parseFollowingInstruction(sliceFrom(t, rp + 2))) {
                appendLog("1.5 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
                return true;
            }
        }
    }
    appendLog("1.5 → FALSE");

    int sc = findFirst(t, ETerminalType::Semicolon);

    // 1.6  <Инициализация переменной> ; <Следующая>
    appendLog("1.6 <Инициализация> ; <Следующая> →");
    if (sc != -1) {
        if (parseVariableInitialization(slice(t, 0, sc)) &&
            parseFollowingInstruction(sliceFrom(t, sc + 1))) {
            appendLog("1.6 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
            return true;
        }
    }
    appendLog("1.6 → FALSE");

    // 1.7  <Присваивание> ; <Следующая>
    appendLog("1.7 <Присваивание> ; <Следующая> →");
    if (sc != -1) {
        if (parseAssignment(slice(t, 0, sc)) &&
            parseFollowingInstruction(sliceFrom(t, sc + 1))) {
            appendLog("1.7 → TRUE"); decTabs(); appendLog("1. <Блок инструкций> → TRUE"); decTabs();
            return true;
        }
    }
    appendLog("1.7 → FALSE");

    decTabs(); appendLog("1. <Блок инструкций> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  2. Последующая инструкция
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseFollowingInstruction(const TermVec& t) {
    incTabs(); appendLog("2. <Последующая инструкция> →");
    incTabs();

    if (t.empty()) {
        decTabs(); appendLog("2.2 λ → TRUE");
        decTabs(); appendLog("2. <Последующая инструкция> → TRUE (пусто)");
        decTabs();
        return true;
    }

    appendLog("2.1 <Блок инструкций> →");
    if (parseInstructionBlock(t)) {
        appendLog("2.1 → TRUE"); decTabs(); appendLog("2. <Последующая инструкция> → TRUE"); decTabs();
        return true;
    }
    appendLog("2.1 → FALSE");
    decTabs(); appendLog("2. <Последующая инструкция> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  3. Инициализация переменной
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseVariableInitialization(const TermVec& t) {
    incTabs(); appendLog("3. <Инициализация переменной> →");
    incTabs();

    // 3.1–3.3  type[ЧИСЛО] ИМЯ
    if (static_cast<int>(t.size()) == 5) {
        ETerminalType tp = typeAt(0, t);
        if ((tp == ETerminalType::Int || tp == ETerminalType::Bool || tp == ETerminalType::String) &&
            typeIs(1, t, ETerminalType::LeftBracket) &&
            typeIs(2, t, ETerminalType::Number) &&
            typeIs(3, t, ETerminalType::RightBracket) &&
            typeIs(4, t, ETerminalType::VariableName)) {
            appendLog("3.x type[N] ИМЯ → TRUE");
            decTabs(); appendLog("3. <Инициализация переменной> → TRUE"); decTabs();
            return true;
        }
    }

    // 3.4–3.6  type ИМЯ
    if (static_cast<int>(t.size()) == 2) {
        ETerminalType tp = typeAt(0, t);
        if ((tp == ETerminalType::Int || tp == ETerminalType::Bool || tp == ETerminalType::String) &&
            typeIs(1, t, ETerminalType::VariableName)) {
            appendLog("3.x type ИМЯ → TRUE");
            decTabs(); appendLog("3. <Инициализация переменной> → TRUE"); decTabs();
            return true;
        }
    }

    decTabs(); appendLog("3. <Инициализация переменной> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  4. Присваивание
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseAssignment(const TermVec& t) {
    incTabs(); appendLog("4. <Присваивание> →");
    incTabs();

    int eq = findFirst(t, ETerminalType::Assignment);
    if (eq != -1) {
        if (parseIdentifier(slice(t, 0, eq)) &&
            parseAssignmentArgument(sliceFrom(t, eq + 1))) {
            appendLog("4.1 → TRUE"); decTabs(); appendLog("4. <Присваивание> → TRUE"); decTabs();
            return true;
        }
    }
    appendLog("4.1 → FALSE");
    decTabs(); appendLog("4. <Присваивание> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  5. Аргумент присваивания
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseAssignmentArgument(const TermVec& t) {
    incTabs(); appendLog("5. <Аргумент присваивания> →");
    incTabs();

    if (parseLogicalOR(t)) {
        appendLog("5.1 <Лог.ИЛИ> → TRUE"); decTabs(); appendLog("5. → TRUE"); decTabs();
        return true;
    }
    if (parseConcatenation(t)) {
        appendLog("5.2 <Конкатенация> → TRUE"); decTabs(); appendLog("5. → TRUE"); decTabs();
        return true;
    }
    if (parseAdditionAndSubtraction(t)) {
        appendLog("5.3 <Сложение/вычитание> → TRUE"); decTabs(); appendLog("5. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("5. <Аргумент присваивания> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  6. Логическое ИЛИ
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseLogicalOR(const TermVec& t) {
    incTabs(); appendLog("6. <Логическое ИЛИ> →");
    incTabs();

    int idx = findFirst(t, ETerminalType::Or);
    if (idx != -1) {
        if (parseLogicalAND(slice(t, 0, idx)) && parseLogicalOR(sliceFrom(t, idx + 1))) {
            appendLog("6.1 → TRUE"); decTabs(); appendLog("6. <Лог.ИЛИ> → TRUE"); decTabs();
            return true;
        }
    }
    if (parseLogicalAND(t)) {
        appendLog("6.2 → TRUE"); decTabs(); appendLog("6. <Лог.ИЛИ> → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("6. <Лог.ИЛИ> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  7. Логическое И
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseLogicalAND(const TermVec& t) {
    incTabs(); appendLog("7. <Логическое И> →");
    incTabs();

    int idx = findFirst(t, ETerminalType::And);
    if (idx != -1) {
        if (parseLogicalANDArgument(slice(t, 0, idx)) && parseLogicalOR(sliceFrom(t, idx + 1))) {
            appendLog("7.1 → TRUE"); decTabs(); appendLog("7. <Лог.И> → TRUE"); decTabs();
            return true;
        }
    }
    if (parseLogicalANDArgument(t)) {
        appendLog("7.2 → TRUE"); decTabs(); appendLog("7. <Лог.И> → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("7. <Лог.И> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  8. Аргумент логического И
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseLogicalANDArgument(const TermVec& t) {
    incTabs(); appendLog("8. <Аргумент Лог.И> →");
    incTabs();

    if (parseNegation(t))           { appendLog("8.1 → TRUE"); decTabs(); appendLog("8. → TRUE"); decTabs(); return true; }
    if (parseStringComparison(t))   { appendLog("8.2 → TRUE"); decTabs(); appendLog("8. → TRUE"); decTabs(); return true; }
    if (parseNumericalComparison(t)){ appendLog("8.3 → TRUE"); decTabs(); appendLog("8. → TRUE"); decTabs(); return true; }

    decTabs(); appendLog("8. <Аргумент Лог.И> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  9. Отрицание
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseNegation(const TermVec& t) {
    incTabs(); appendLog("9. <Отрицание> →");
    incTabs();

    if (typeIs(0, t, ETerminalType::Not)) {
        if (parseNegationArgument(sliceFrom(t, 1))) {
            appendLog("9.1 → TRUE"); decTabs(); appendLog("9. → TRUE"); decTabs();
            return true;
        }
    }
    if (parseNegationArgument(t)) {
        appendLog("9.2 → TRUE"); decTabs(); appendLog("9. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("9. <Отрицание> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  10. Аргумент отрицания
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseNegationArgument(const TermVec& t) {
    incTabs(); appendLog("10. <Аргумент отрицания> →");
    incTabs();

    if (typeIs(0, t, ETerminalType::LeftParen)) {
        int rp = findPairedClosingBracket(0, t);
        if (rp == static_cast<int>(t.size()) - 1) {
            if (parseLogicalOR(slice(t, 1, rp))) {
                appendLog("10.1 → TRUE"); decTabs(); appendLog("10. → TRUE"); decTabs();
                return true;
            }
        }
    }
    if (parseIdentifier(t)) {
        appendLog("10.2 → TRUE"); decTabs(); appendLog("10. → TRUE"); decTabs();
        return true;
    }
    if (static_cast<int>(t.size()) == 1 && typeIs(0, t, ETerminalType::Boolean)) {
        appendLog("10.3 БУЛЕАН → TRUE"); decTabs(); appendLog("10. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("10. <Аргумент отрицания> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  11. Строковое сравнение
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseStringComparison(const TermVec& t) {
    incTabs(); appendLog("11. <Строковое сравнение> →");
    incTabs();

    int eq = findFirst(t, ETerminalType::Equal);
    if (eq != -1) {
        if (parseConcatenation(slice(t, 0, eq)) && parseConcatenation(sliceFrom(t, eq + 1))) {
            appendLog("11.1 → TRUE"); decTabs(); appendLog("11. → TRUE"); decTabs();
            return true;
        }
    }
    decTabs(); appendLog("11. <Строковое сравнение> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  12. Конкатенация
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseConcatenation(const TermVec& t) {
    incTabs(); appendLog("12. <Конкатенация> →");
    incTabs();

    int plus = findFirst(t, ETerminalType::Plus);
    if (plus != -1) {
        if (parseConcatenationArgument(slice(t, 0, plus)) && parseConcatenation(sliceFrom(t, plus + 1))) {
            appendLog("12.1 → TRUE"); decTabs(); appendLog("12. → TRUE"); decTabs();
            return true;
        }
    }
    if (parseConcatenationArgument(t)) {
        appendLog("12.2 → TRUE"); decTabs(); appendLog("12. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("12. <Конкатенация> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  13. Аргумент конкатенации
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseConcatenationArgument(const TermVec& t) {
    incTabs(); appendLog("13. <Аргумент конкатенации> →");
    incTabs();

    if (parseIdentifier(t)) {
        appendLog("13.1 → TRUE"); decTabs(); appendLog("13. → TRUE"); decTabs();
        return true;
    }
    if (static_cast<int>(t.size()) == 1 && typeIs(0, t, ETerminalType::TextLine)) {
        appendLog("13.2 СТРОКА → TRUE"); decTabs(); appendLog("13. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("13. <Аргумент конкатенации> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  14. Числовое сравнение
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseNumericalComparison(const TermVec& t) {
    incTabs(); appendLog("14. <Числовое сравнение> →");
    incTabs();

    static const ETerminalType comparisons[] = {
        ETerminalType::Greater, ETerminalType::Less, ETerminalType::Equal,
        ETerminalType::GreaterEqual, ETerminalType::LessEqual
    };

    for (ETerminalType cmp : comparisons) {
        int idx = findFirst(t, cmp);
        if (idx != -1) {
            if (parseAdditionAndSubtraction(slice(t, 0, idx)) &&
                parseAdditionAndSubtraction(sliceFrom(t, idx + 1))) {
                appendLog("14.1 → TRUE"); decTabs(); appendLog("14. → TRUE"); decTabs();
                return true;
            }
        }
    }
    decTabs(); appendLog("14. <Числовое сравнение> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  16. Сложение и вычитание
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseAdditionAndSubtraction(const TermVec& t) {
    incTabs(); appendLog("16. <Сложение/вычитание> →");
    incTabs();

    int plus = findFirst(t, ETerminalType::Plus);
    if (plus != -1) {
        if (parseMultiplicationAndDivision(slice(t, 0, plus)) &&
            parseAdditionAndSubtraction(sliceFrom(t, plus + 1))) {
            appendLog("16.1 → TRUE"); decTabs(); appendLog("16. → TRUE"); decTabs();
            return true;
        }
    }
    int minus = findFirst(t, ETerminalType::Minus);
    if (minus != -1) {
        if (parseMultiplicationAndDivision(slice(t, 0, minus)) &&
            parseAdditionAndSubtraction(sliceFrom(t, minus + 1))) {
            appendLog("16.2 → TRUE"); decTabs(); appendLog("16. → TRUE"); decTabs();
            return true;
        }
    }
    if (parseMultiplicationAndDivision(t)) {
        appendLog("16.3 → TRUE"); decTabs(); appendLog("16. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("16. <Сложение/вычитание> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  17. Умножение и деление
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseMultiplicationAndDivision(const TermVec& t) {
    incTabs(); appendLog("17. <Умножение/деление> →");
    incTabs();

    auto tryOp = [&](ETerminalType op) -> bool {
        int idx = findFirst(t, op);
        if (idx != -1) {
            return parseUnaryMinus(slice(t, 0, idx)) &&
                   parseMultiplicationAndDivision(sliceFrom(t, idx + 1));
        }
        return false;
    };

    if (tryOp(ETerminalType::Multiply)) { appendLog("17.1 → TRUE"); decTabs(); appendLog("17. → TRUE"); decTabs(); return true; }
    if (tryOp(ETerminalType::Divide))   { appendLog("17.2 → TRUE"); decTabs(); appendLog("17. → TRUE"); decTabs(); return true; }
    if (tryOp(ETerminalType::Modulus))  { appendLog("17.3 → TRUE"); decTabs(); appendLog("17. → TRUE"); decTabs(); return true; }
    if (parseUnaryMinus(t))             { appendLog("17.4 → TRUE"); decTabs(); appendLog("17. → TRUE"); decTabs(); return true; }

    decTabs(); appendLog("17. <Умножение/деление> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  18. Унарный минус
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseUnaryMinus(const TermVec& t) {
    incTabs(); appendLog("18. <Унарный минус> →");
    incTabs();

    if (typeIs(0, t, ETerminalType::Minus)) {
        if (parseUnaryMinusArgument(sliceFrom(t, 1))) {
            appendLog("18.1 → TRUE"); decTabs(); appendLog("18. → TRUE"); decTabs();
            return true;
        }
    }
    if (parseUnaryMinusArgument(t)) {
        appendLog("18.2 → TRUE"); decTabs(); appendLog("18. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("18. <Унарный минус> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  19. Аргумент унарного минуса
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseUnaryMinusArgument(const TermVec& t) {
    incTabs(); appendLog("19. <Аргумент унарного минуса> →");
    incTabs();

    if (typeIs(0, t, ETerminalType::LeftParen)) {
        int rp = findPairedClosingBracket(0, t);
        if (rp == static_cast<int>(t.size()) - 1) {
            if (parseAdditionAndSubtraction(slice(t, 1, rp))) {
                appendLog("19.1 → TRUE"); decTabs(); appendLog("19. → TRUE"); decTabs();
                return true;
            }
        }
    }
    if (parseIdentifier(t)) {
        appendLog("19.2 → TRUE"); decTabs(); appendLog("19. → TRUE"); decTabs();
        return true;
    }
    if (static_cast<int>(t.size()) == 1 && typeIs(0, t, ETerminalType::Number)) {
        appendLog("19.3 ЧИСЛО → TRUE"); decTabs(); appendLog("19. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("19. <Аргумент унарного минуса> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  20. Идентификатор
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseIdentifier(const TermVec& t) {
    incTabs(); appendLog("20. <Идентификатор> →");
    incTabs();

    // 20.1  ИМЯ[<Индексатор>]
    if (typeIs(0, t, ETerminalType::VariableName) && typeIs(1, t, ETerminalType::LeftBracket)) {
        int rb = findPairedClosingBracket(1, t);
        if (rb == static_cast<int>(t.size()) - 1) {
            if (parseIndexer(slice(t, 2, rb))) {
                appendLog("20.1 → TRUE"); decTabs(); appendLog("20. → TRUE"); decTabs();
                return true;
            }
        }
    }

    // 20.2  ИМЯ
    if (static_cast<int>(t.size()) == 1 && typeIs(0, t, ETerminalType::VariableName)) {
        appendLog("20.2 → TRUE"); decTabs(); appendLog("20. → TRUE"); decTabs();
        return true;
    }

    decTabs(); appendLog("20. <Идентификатор> → FALSE"); decTabs();
    return false;
}

// ────────────────────────────────────────────────────────────
//  21. Индексатор
// ────────────────────────────────────────────────────────────

bool SyntacticalAnalyzer::parseIndexer(const TermVec& t) {
    incTabs(); appendLog("21. <Индексатор> →");
    incTabs();

    // 21.1  ИМЯ[<Индексатор>]
    if (typeIs(0, t, ETerminalType::VariableName) && typeIs(1, t, ETerminalType::LeftBracket)) {
        int rb = findPairedClosingBracket(1, t);
        if (rb == static_cast<int>(t.size()) - 1) {
            if (parseIndexer(slice(t, 2, rb))) {
                appendLog("21.1 → TRUE"); decTabs(); appendLog("21. → TRUE"); decTabs();
                return true;
            }
        }
    }
    // 21.2  ИМЯ
    if (static_cast<int>(t.size()) == 1 && typeIs(0, t, ETerminalType::VariableName)) {
        appendLog("21.2 → TRUE"); decTabs(); appendLog("21. → TRUE"); decTabs();
        return true;
    }
    // 21.3  ЧИСЛО
    if (static_cast<int>(t.size()) == 1 && typeIs(0, t, ETerminalType::Number)) {
        appendLog("21.3 → TRUE"); decTabs(); appendLog("21. → TRUE"); decTabs();
        return true;
    }
    // 21.4  <Сложение/вычитание>
    if (parseAdditionAndSubtraction(t)) {
        appendLog("21.4 → TRUE"); decTabs(); appendLog("21. → TRUE"); decTabs();
        return true;
    }
    decTabs(); appendLog("21. <Индексатор> → FALSE"); decTabs();
    return false;
}
