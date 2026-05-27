#include "RPNTranslator.h"
#include <stdexcept>
#include <algorithm>

// Инициализация статических членов
RPNTranslator::TermVec  RPNTranslator::_input;
RPNTranslator::RPNVec   RPNTranslator::_output;
RPNTranslator::MarkVec  RPNTranslator::_tempMarks;
RPNTranslator::MarkVec  RPNTranslator::_constMarks;
RPNTranslator::RPNVec   RPNTranslator::_operationStack;

// ────────────────────────────────────────────────────────────
//  Предикаты
// ────────────────────────────────────────────────────────────

bool RPNTranslator::isOpeningParenthesis(const std::shared_ptr<Terminal>& t) {
    auto tp = t->terminalType;
    return tp == ETerminalType::LeftParen ||
           tp == ETerminalType::LeftBracket ||
           tp == ETerminalType::LeftBrace;
}

bool RPNTranslator::isOperand(const std::shared_ptr<Terminal>& t) {
    auto tp = t->terminalType;
    return tp == ETerminalType::Number ||
           tp == ETerminalType::TextLine ||
           tp == ETerminalType::Boolean ||
           tp == ETerminalType::VariableName;
}

bool RPNTranslator::isOperationOrParenthesis(const std::shared_ptr<Terminal>& t) {
    switch (t->terminalType) {
        case ETerminalType::Int: case ETerminalType::String: case ETerminalType::Bool:
        case ETerminalType::Semicolon: case ETerminalType::Output: case ETerminalType::Input:
        case ETerminalType::Assignment: case ETerminalType::And: case ETerminalType::Or:
        case ETerminalType::Equal: case ETerminalType::Less: case ETerminalType::Greater:
        case ETerminalType::GreaterEqual: case ETerminalType::LessEqual:
        case ETerminalType::Plus: case ETerminalType::Minus: case ETerminalType::Divide:
        case ETerminalType::Multiply: case ETerminalType::Modulus: case ETerminalType::Not:
        case ETerminalType::LeftParen: case ETerminalType::RightParen:
        case ETerminalType::LeftBracket: case ETerminalType::RightBracket:
        case ETerminalType::LeftBrace: case ETerminalType::RightBrace:
            return true;
        default:
            return false;
    }
}

bool RPNTranslator::isWritableInOutput(const std::shared_ptr<RPNSymbol>& s) {
    switch (s->rpnType) {
        case ERPNType::T_Semicolon:
        case ERPNType::T_RightParen: case ERPNType::T_LeftParen:
        case ERPNType::T_RightBracket: case ERPNType::T_LeftBracket:
        case ERPNType::T_LeftBrace: case ERPNType::T_RightBrace:
            return false;
        default:
            return true;
    }
}

bool RPNTranslator::isWritableInOperationStack(const std::shared_ptr<RPNSymbol>& s) {
    switch (s->rpnType) {
        case ERPNType::T_Semicolon:
        case ERPNType::T_RightParen: case ERPNType::T_RightBracket:
        case ERPNType::T_RightBrace: case ERPNType::T_LeftParen:
            return false;
        default:
            return true;
    }
}

bool RPNTranslator::isVariableInitialization(const std::shared_ptr<RPNSymbol>& s) {
    return s->rpnType == ERPNType::F_Int ||
           s->rpnType == ERPNType::F_String ||
           s->rpnType == ERPNType::F_Bool;
}

// ────────────────────────────────────────────────────────────
//  Приоритеты
// ────────────────────────────────────────────────────────────

int RPNTranslator::getRPNSymbolPriority(const std::shared_ptr<RPNSymbol>& s) {
    switch (s->rpnType) {
        case ERPNType::T_Semicolon:
        case ERPNType::T_LeftParen: case ERPNType::T_LeftBrace: case ERPNType::T_LeftBracket:
        case ERPNType::F_ConditionalJumpToMark: case ERPNType::F_UnconditionalJumpToMark:
        case ERPNType::M_Mark:        return -1;
        case ERPNType::F_Assignment:  return  0;
        case ERPNType::F_And: case ERPNType::F_Or:  return 1;
        case ERPNType::F_Equal: case ERPNType::F_Less: case ERPNType::F_Greater:
        case ERPNType::F_LessEqual: case ERPNType::F_GreaterEqual: return 2;
        case ERPNType::F_Plus: case ERPNType::F_Minus:  return 3;
        case ERPNType::F_Multiply: case ERPNType::F_Divide: case ERPNType::F_Modulus: return 4;
        case ERPNType::F_Not:    return 5;
        case ERPNType::F_Int: case ERPNType::F_String: case ERPNType::F_Bool:
        case ERPNType::F_IntArray: case ERPNType::F_StringArray: case ERPNType::F_BoolArray: return 6;
        case ERPNType::F_Input: case ERPNType::F_Output: return 7;
        case ERPNType::F_Index: return 8;
        default:
            throw std::runtime_error("Неизвестный тип символа RPN для определения приоритета.");
    }
}

ERPNType RPNTranslator::toArrayInit(const std::shared_ptr<RPNSymbol>& s) {
    switch (s->rpnType) {
        case ERPNType::F_Int:    return ERPNType::F_IntArray;
        case ERPNType::F_String: return ERPNType::F_StringArray;
        case ERPNType::F_Bool:   return ERPNType::F_BoolArray;
        default:
            throw std::runtime_error("toArrayInit: неверный тип операции инициализации.");
    }
}

// ────────────────────────────────────────────────────────────
//  Перевод терминалов в символы RPN
// ────────────────────────────────────────────────────────────

std::shared_ptr<RPNSymbol> RPNTranslator::translateToRPNSymbol(const std::shared_ptr<Terminal>& t) {
    switch (t->terminalType) {
        case ETerminalType::Assignment:  return std::make_shared<RPNSymbol>(ERPNType::F_Assignment);
        case ETerminalType::And:         return std::make_shared<RPNSymbol>(ERPNType::F_And);
        case ETerminalType::Or:          return std::make_shared<RPNSymbol>(ERPNType::F_Or);
        case ETerminalType::Equal:       return std::make_shared<RPNSymbol>(ERPNType::F_Equal);
        case ETerminalType::Less:        return std::make_shared<RPNSymbol>(ERPNType::F_Less);
        case ETerminalType::Greater:     return std::make_shared<RPNSymbol>(ERPNType::F_Greater);
        case ETerminalType::LessEqual:   return std::make_shared<RPNSymbol>(ERPNType::F_LessEqual);
        case ETerminalType::GreaterEqual:return std::make_shared<RPNSymbol>(ERPNType::F_GreaterEqual);
        case ETerminalType::Plus:        return std::make_shared<RPNSymbol>(ERPNType::F_Plus);
        case ETerminalType::Minus:       return std::make_shared<RPNSymbol>(ERPNType::F_Minus);
        case ETerminalType::Multiply:    return std::make_shared<RPNSymbol>(ERPNType::F_Multiply);
        case ETerminalType::Divide:      return std::make_shared<RPNSymbol>(ERPNType::F_Divide);
        case ETerminalType::Modulus:     return std::make_shared<RPNSymbol>(ERPNType::F_Modulus);
        case ETerminalType::Not:         return std::make_shared<RPNSymbol>(ERPNType::F_Not);
        case ETerminalType::Int:         return std::make_shared<RPNSymbol>(ERPNType::F_Int);
        case ETerminalType::String:      return std::make_shared<RPNSymbol>(ERPNType::F_String);
        case ETerminalType::Bool:        return std::make_shared<RPNSymbol>(ERPNType::F_Bool);
        case ETerminalType::Input:       return std::make_shared<RPNSymbol>(ERPNType::F_Input);
        case ETerminalType::Output:      return std::make_shared<RPNSymbol>(ERPNType::F_Output);
        case ETerminalType::LeftBracket: return std::make_shared<RPNSymbol>(ERPNType::T_LeftBracket);
        case ETerminalType::RightBracket:return std::make_shared<RPNSymbol>(ERPNType::T_RightBracket);
        case ETerminalType::LeftParen:   return std::make_shared<RPNSymbol>(ERPNType::T_LeftParen);
        case ETerminalType::RightParen:  return std::make_shared<RPNSymbol>(ERPNType::T_RightParen);
        case ETerminalType::LeftBrace:   return std::make_shared<RPNSymbol>(ERPNType::T_LeftBrace);
        case ETerminalType::RightBrace:  return std::make_shared<RPNSymbol>(ERPNType::T_RightBrace);
        case ETerminalType::Semicolon:   return std::make_shared<RPNSymbol>(ERPNType::T_Semicolon);
        default:
            throw std::runtime_error("translateToRPNSymbol: неизвестный тип терминала.");
    }
}

std::shared_ptr<RPNSymbol> RPNTranslator::translateOperand(const std::shared_ptr<Terminal>& t) {
    if (auto* tl = dynamic_cast<TerminalTextLine*>(t.get())) {
        auto s = std::make_shared<RPNTextLine>(tl->data);
        s->charPointer = tl->charPointer;
        s->linePointer = tl->linePointer;
        return s;
    }
    if (auto* tb = dynamic_cast<TerminalBoolean*>(t.get())) {
        auto s = std::make_shared<RPNBoolean>(tb->data);
        s->charPointer = tb->charPointer;
        s->linePointer = tb->linePointer;
        return s;
    }
    if (auto* tn = dynamic_cast<TerminalNumber*>(t.get())) {
        auto s = std::make_shared<RPNNumber>(tn->data);
        s->charPointer = tn->charPointer;
        s->linePointer = tn->linePointer;
        return s;
    }
    if (auto* ti = dynamic_cast<TerminalIdentifier*>(t.get())) {
        auto s = std::make_shared<RPNIdentifier>(ti->name);
        s->charPointer = ti->charPointer;
        s->linePointer = ti->linePointer;
        return s;
    }
    // Заглушка
    return std::make_shared<RPNSymbol>(ERPNType::A_VariableName);
}

// ────────────────────────────────────────────────────────────
//  Запись меток
// ────────────────────────────────────────────────────────────

void RPNTranslator::writeMarks() {
    for (auto& sym : _output) {
        if (auto* m = dynamic_cast<RPNMark*>(sym.get())) {
            if (_constMarks.empty())
                break;
            // Если позиция уже задана — пропускаем
            if (m->position.has_value())
                continue;
            m->position = _constMarks.front()->position;
            _constMarks.erase(_constMarks.begin());
        }
    }
}

// ────────────────────────────────────────────────────────────
//  Помещение символа в стек (алгоритм Дейкстры)
// ────────────────────────────────────────────────────────────

void RPNTranslator::toStack(std::shared_ptr<RPNSymbol> input) {
    if (!_operationStack.empty()) {
        // Правая круглая скобка — выталкиваем до левой
        if (input->rpnType == ERPNType::T_RightParen) {
            while (!_operationStack.empty() &&
                   _operationStack.back()->rpnType != ERPNType::T_LeftParen) {
                if (isWritableInOutput(_operationStack.back()))
                    _output.push_back(_operationStack.back());
                _operationStack.pop_back();
            }
            if (_operationStack.size() > 1) {
                _operationStack.pop_back(); // убираем T_LeftParen
                // Если за скобкой стоит метка (условие if) — выталкиваем управляющие символы
                if (_operationStack.back()->rpnType == ERPNType::M_Mark) {
                    if (isWritableInOutput(_operationStack.back()))
                        _output.push_back(_operationStack.back());
                    _operationStack.pop_back();
                    if (!_operationStack.empty() && isWritableInOutput(_operationStack.back()))
                        _output.push_back(_operationStack.back());
                    if (!_operationStack.empty())
                        _operationStack.pop_back();
                }
            }
            return;
        }

        // Правая квадратная скобка — выталкиваем до левой
        if (input->rpnType == ERPNType::T_RightBracket) {
            while (!_operationStack.empty() &&
                   _operationStack.back()->rpnType != ERPNType::T_LeftBracket) {
                if (isWritableInOutput(_operationStack.back()))
                    _output.push_back(_operationStack.back());
                _operationStack.pop_back();
            }
            // Если перед [ стоит объявление типа — генерируем объявление массива
            if (_operationStack.size() >= 2 &&
                isVariableInitialization(_operationStack[_operationStack.size() - 2])) {
                _operationStack.pop_back(); // убираем T_LeftBracket
                if (!_input.empty() && static_cast<int>(_input.size()) > 1) {
                    _output.push_back(translateOperand(_input[1]));
                    _input.erase(_input.begin() + 1);
                }
                _output.push_back(std::make_shared<RPNSymbol>(toArrayInit(_operationStack.back())));
                _operationStack.pop_back();
            } else {
                if (!_operationStack.empty() &&
                    _operationStack.back()->rpnType == ERPNType::T_LeftBracket)
                    _operationStack.pop_back();
                _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_Index));
            }
            return;
        }

        // Правая фигурная скобка — обрабатывается в основном цикле
        if (input->rpnType == ERPNType::T_RightBrace) {
            while (!_operationStack.empty() &&
                   _operationStack.back()->rpnType != ERPNType::T_LeftBrace) {
                if (isWritableInOutput(_operationStack.back()))
                    _output.push_back(_operationStack.back());
                _operationStack.pop_back();
            }
            if (!_operationStack.empty())
                _operationStack.pop_back(); // убираем T_LeftBrace

            // Ищем WhileBeginMark/WhileEndMark в TempMarks (back() может быть endMark)
            std::shared_ptr<RPNMark> wBegin, wEnd;
            for (auto it = _tempMarks.rbegin(); it != _tempMarks.rend(); ++it) {
                if ((*it)->markType == EMarkType::WhileBeginMark && !wBegin) wBegin = *it;
                if ((*it)->markType == EMarkType::WhileEndMark && !(*it)->position.has_value() && !wEnd) wEnd = *it;
            }

            if (wBegin && wEnd) {
                _output.push_back(wBegin);
                _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_UnconditionalJumpToMark));
                wEnd->position = static_cast<int>(_output.size());
                _tempMarks.erase(std::remove_if(_tempMarks.begin(), _tempMarks.end(),
                        [wBegin](const std::shared_ptr<RPNMark>& m) { return m.get() == wBegin.get(); }), _tempMarks.end());
                _tempMarks.erase(std::remove_if(_tempMarks.begin(), _tempMarks.end(),
                        [wEnd](const std::shared_ptr<RPNMark>& m) { return m.get() == wEnd.get(); }), _tempMarks.end());
            } else if (!_tempMarks.empty() && _tempMarks.back()->markType == EMarkType::IfMark) {
                _tempMarks.back()->position = static_cast<int>(_output.size());
                _constMarks.push_back(_tempMarks.back());
                _tempMarks.pop_back();

                if (_input.size() > 1 && _input[1]->terminalType == ETerminalType::Else) {
                    auto elseMark = std::make_shared<RPNMark>(EMarkType::ElseMark);
                    _output.push_back(elseMark);
                    _tempMarks.push_back(elseMark);
                    _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_UnconditionalJumpToMark));
                }
            } else if (!_tempMarks.empty() && _tempMarks.back()->markType == EMarkType::ElseMark) {
                _tempMarks.back()->position = static_cast<int>(_output.size());
                _constMarks.push_back(_tempMarks.back());
                _tempMarks.pop_back();
            }
            return;
        }

        // Обычная операция — выталкиваем из стека всё с приоритетом >=
        while (!_operationStack.empty() &&
               _operationStack.back()->rpnType != ERPNType::T_LeftParen &&
               _operationStack.back()->rpnType != ERPNType::T_LeftBracket &&
               _operationStack.back()->rpnType != ERPNType::T_LeftBrace &&
               getRPNSymbolPriority(_operationStack.back()) >= getRPNSymbolPriority(input)) {
            if (isWritableInOutput(_operationStack.back()))
                _output.push_back(_operationStack.back());
            _operationStack.pop_back();
        }
    }

    if (isWritableInOperationStack(input))
        _operationStack.push_back(input);
}

// ────────────────────────────────────────────────────────────
//  Главный метод трансляции
// ────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<RPNSymbol>>
RPNTranslator::convertToRPN(const std::vector<std::shared_ptr<Terminal>>& inputTerminals) {
    _input          = TermVec(inputTerminals);
    _output.clear();
    _tempMarks.clear();
    _constMarks.clear();
    _operationStack.clear();

    while (!_input.empty()) {
        auto front = _input.front();

        if (isOpeningParenthesis(front)) {
            _operationStack.push_back(translateToRPNSymbol(front));
            _input.erase(_input.begin());
        }
        else if (isOperationOrParenthesis(front)) {
            toStack(translateToRPNSymbol(front));
            _input.erase(_input.begin());
        }
        else if (isOperand(front)) {
            _output.push_back(translateOperand(front));
            _input.erase(_input.begin());
        }
        else if (front->terminalType == ETerminalType::While) {
            // Метка начала цикла
            auto beginMark = std::make_shared<RPNMark>(EMarkType::WhileBeginMark);
            _tempMarks.push_back(beginMark);
            beginMark->position = static_cast<int>(_output.size());
            _output.push_back(beginMark);

            auto endMark = std::make_shared<RPNMark>(EMarkType::WhileEndMark);
            _tempMarks.push_back(endMark);

            _input.erase(_input.begin()); // убираем while

            // Пропускаем открывающую скобку условия
            if (!_input.empty() && _input.front()->terminalType == ETerminalType::LeftParen)
                _input.erase(_input.begin());

            // Обрабатываем условие до закрывающей скобки
            while (!_input.empty() && _input.front()->terminalType != ETerminalType::RightParen) {
                if (isOperationOrParenthesis(_input.front()))
                    toStack(translateToRPNSymbol(_input.front()));
                else if (isOperand(_input.front()))
                    _output.push_back(translateOperand(_input.front()));
                _input.erase(_input.begin());
            }

            // Выталкиваем оставшиеся операции из стека
            while (!_operationStack.empty() &&
                   _operationStack.back()->rpnType != ERPNType::T_LeftParen) {
                if (isWritableInOutput(_operationStack.back()))
                    _output.push_back(_operationStack.back());
                _operationStack.pop_back();
            }

            // Убираем закрывающую скобку
            if (!_input.empty() && _input.front()->terminalType == ETerminalType::RightParen)
                _input.erase(_input.begin());

            // Условный переход к метке конца цикла
            _output.push_back(endMark);
            _constMarks.push_back(endMark);
            _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_ConditionalJumpToMark));
        }
        else if (front->terminalType == ETerminalType::If) {
            _operationStack.push_back(std::make_shared<RPNSymbol>(ERPNType::F_ConditionalJumpToMark));
            auto ifMark = std::make_shared<RPNMark>(EMarkType::IfMark);
            _operationStack.push_back(ifMark);
            _tempMarks.push_back(ifMark);
            _input.erase(_input.begin());
        }
        else if (front->terminalType == ETerminalType::Else) {
            auto elseMark = std::make_shared<RPNMark>(EMarkType::ElseMark);
            _tempMarks.push_back(elseMark);
            _output.push_back(elseMark);
            _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_UnconditionalJumpToMark));
            _input.erase(_input.begin());
        }
        else if (front->terminalType == ETerminalType::RightBrace) {
            // Правая фигурная — выталкиваем до левой
            while (!_operationStack.empty() &&
                   _operationStack.back()->rpnType != ERPNType::T_LeftBrace) {
                if (isWritableInOutput(_operationStack.back()))
                    _output.push_back(_operationStack.back());
                _operationStack.pop_back();
            }
            if (!_operationStack.empty())
                _operationStack.pop_back(); // убираем T_LeftBrace

            // Проверяем незавершённый цикл while
            std::shared_ptr<RPNMark> wBegin, wEnd;
            for (auto it = _tempMarks.rbegin(); it != _tempMarks.rend(); ++it) {
                if ((*it)->markType == EMarkType::WhileBeginMark && !wBegin) wBegin = *it;
                if ((*it)->markType == EMarkType::WhileEndMark && !(*it)->position.has_value() && !wEnd) wEnd = *it;
            }

            if (wBegin && wEnd) {
                _output.push_back(wBegin);
                _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_UnconditionalJumpToMark));
                wEnd->position = static_cast<int>(_output.size());
                _tempMarks.erase(std::remove_if(_tempMarks.begin(), _tempMarks.end(),
                        [wBegin](const std::shared_ptr<RPNMark>& m) { return m.get() == wBegin.get(); }), _tempMarks.end());
                _tempMarks.erase(std::remove_if(_tempMarks.begin(), _tempMarks.end(),
                        [wEnd](const std::shared_ptr<RPNMark>& m) { return m.get() == wEnd.get(); }), _tempMarks.end());
            } else if (!_tempMarks.empty() && _tempMarks.back()->markType == EMarkType::IfMark) {
                _tempMarks.back()->position = static_cast<int>(_output.size());
                _constMarks.erase(std::remove_if(_constMarks.begin(), _constMarks.end(),
                    [&_tempMarks](const std::shared_ptr<RPNMark>& m) { return m.get() == _tempMarks.back().get(); }), _constMarks.end());
                _constMarks.push_back(_tempMarks.back());
                _tempMarks.pop_back();

                // Если следующий токен — else, подготавливаем метку else
                if (_input.size() > 1 && _input[1]->terminalType == ETerminalType::Else) {
                    auto elseMark = std::make_shared<RPNMark>(EMarkType::ElseMark);
                    _output.push_back(elseMark);
                    _tempMarks.push_back(elseMark);
                    _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_UnconditionalJumpToMark));
                }
            } else if (!_tempMarks.empty() && _tempMarks.back()->markType == EMarkType::ElseMark) {
                _tempMarks.back()->position = static_cast<int>(_output.size());
                _constMarks.erase(std::remove_if(_constMarks.begin(), _constMarks.end(),
                    [&_tempMarks](const std::shared_ptr<RPNMark>& m) { return m.get() == _tempMarks.back().get(); }), _constMarks.end());
                _constMarks.push_back(_tempMarks.back());
                _tempMarks.pop_back();
            }
            _input.erase(_input.begin());
        }
        else {
            // Неизвестный токен — пропускаем
            _input.erase(_input.begin());
        }
    }

    // Выталкиваем оставшиеся операции из стека
    while (!_operationStack.empty()) {
        if (isWritableInOutput(_operationStack.back())) {
            _output.push_back(_operationStack.back());

            // Если это метка начала цикла while, добавляем безусловный переход
            if (auto* mark = dynamic_cast<RPNMark*>(_operationStack.back().get())) {
                if (mark->markType == EMarkType::WhileBeginMark) {
                    // Добавляем безусловный переход к началу цикла
                    std::shared_ptr<RPNMark> unfinishedBegin;
                    std::shared_ptr<RPNMark> unfinishedEnd;
                    for (auto& m : _tempMarks) {
                        if (m->markType == EMarkType::WhileBeginMark && !unfinishedBegin)
                            unfinishedBegin = m;
                        if (m->markType == EMarkType::WhileEndMark && !m->position.has_value() && !unfinishedEnd)
                            unfinishedEnd = m;
                    }
                    if (unfinishedBegin)
                        _output.push_back(unfinishedBegin);
                    _output.push_back(std::make_shared<RPNSymbol>(ERPNType::F_UnconditionalJumpToMark));

                    if (unfinishedEnd) {
                        unfinishedEnd->position = static_cast<int>(_output.size()) + 1;
                        _tempMarks.erase(std::remove_if(_tempMarks.begin(), _tempMarks.end(),
                            [unfinishedBegin, unfinishedEnd](const std::shared_ptr<RPNMark>& m) {
                                return m.get() == unfinishedBegin.get() || m.get() == unfinishedEnd.get();
                            }), _tempMarks.end());
                    }
                }
            }
        }
        _operationStack.pop_back();
    }

    writeMarks();
    return _output;
}
