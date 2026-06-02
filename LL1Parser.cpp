#include "LL1Parser.h"
#include <iostream>
#include <stdexcept>

LL1Parser::LL1Parser() {
    initParsingTable();
}

void LL1Parser::initParsingTable() {
    // ---- ПРАВИЛО ДЛЯ ПРОГРАММЫ: P -> { L } ----
    _parsingTable[ENonTerminal::Program][ETerminalType::LeftBrace] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftBrace) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) }, 
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightBrace) }
    };

    // ---- ПРАВИЛА ДЛЯ СПИСКА ОПЕРАТОРОВ: L -> A ; L | W ; L | lambda ----
    // Если видим переменную -> это оператор присваивания (Statement)
    _parsingTable[ENonTerminal::StmtList][ETerminalType::VariableName] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Statement) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) }
    };

    _parsingTable[ENonTerminal::StmtList][ETerminalType::Int] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Statement) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) }
    };

    // Если видим while -> это цикл (Statement)
    _parsingTable[ENonTerminal::StmtList][ETerminalType::While] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Statement) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) }
    };
    // Если видим output -> это встроенная функция (трактуем как Statement для теста)

    _parsingTable[ENonTerminal::StmtList][ETerminalType::Output] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Statement) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) }
    };

    _parsingTable[ENonTerminal::StmtList][ETerminalType::If] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Statement) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) }
    };
    // Эпсилон-правило: список операторов завершается, когда видим закрывающую скобку '}'
    _parsingTable[ENonTerminal::StmtList][ETerminalType::RightBrace] = {};

    // ---- ПРАВИЛА ДЛЯ ОПЕРАТОРОВ (Statement) ----
    
    // Цикл: while ( C ) { L } ;
    _parsingTable[ENonTerminal::Statement][ETerminalType::While] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::While) },
        { EStackItemType::SemanticAction, 4 }, 
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftParen) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Condition) }, 
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightParen) },
        { EStackItemType::SemanticAction, 1 }, 
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftBrace) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightBrace) },
        { EStackItemType::SemanticAction, 5 }  
    };

    _parsingTable[ENonTerminal::Statement][ETerminalType::Int] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Int) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::VariableName) }, // Имя переменной улетит в ОПС
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Semicolon) },
        { EStackItemType::SemanticAction, 17 } // Новое семантическое действие для ОПС
    };

    // Присваивание: ID = Expression ;
    _parsingTable[ENonTerminal::Statement][ETerminalType::VariableName] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::VariableName) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Assignment) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) }, 
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Semicolon) },
        { EStackItemType::SemanticAction, 14 } // Записать операцию ':=' в ОПС
    };

    // Правило грамматики: if ( Condition ) { StmtList } else { StmtList }
    _parsingTable[ENonTerminal::Statement][ETerminalType::If] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::If) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftParen) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Condition) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightParen) },
        { EStackItemType::SemanticAction, 1 }, // Программа 1: генерирует 'jf' и резервирует пустышку адреса
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftBrace) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightBrace) },
        { EStackItemType::SemanticAction, 2 }, // Программа 2: генерирует 'j' для обхода else, заполняет адрес 'jf'
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Else) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftBrace) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::StmtList) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightBrace) },
        { EStackItemType::SemanticAction, 3 }  // Программа 3: заполняет адрес безусловного перехода 'j'
    };

    // Оператор вывода для теста: output ( Expression ) ;
    _parsingTable[ENonTerminal::Statement][ETerminalType::Output] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Output) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::LeftParen) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::RightParen) },
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Semicolon) },
        { EStackItemType::SemanticAction, 15 } // <-- Семантическое действие для ОПС
    };

    // ---- МАТЕМАТИЧЕСКИЕ ВЫРАЖЕНИЯ: S -> T U ----
    _parsingTable[ENonTerminal::Expression][ETerminalType::Number] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Term) },           // T
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::ExpressionPrime) } // U
    };
    _parsingTable[ENonTerminal::Expression][ETerminalType::VariableName] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Term) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::ExpressionPrime) }
    };

    // U -> + T <Прог_10> U | - T <Прог_11> U | lambda
    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::Plus] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Plus) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Term) },
        { EStackItemType::SemanticAction, 10 }, 
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::ExpressionPrime) }
    };
    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::Minus] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Minus) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Term) },
        { EStackItemType::SemanticAction, 11 }, 
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::ExpressionPrime) }
    };

    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::Less] = {};
    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::Greater] = {};
    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::Equal] = {};
    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::NotEqual] = {};

    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::Semicolon] = {};  
    _parsingTable[ENonTerminal::ExpressionPrime][ETerminalType::RightParen] = {}; 

    // ---- ТЕРМЫ: T -> F Y ----
    _parsingTable[ENonTerminal::Term][ETerminalType::Number] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Factor) },    // F
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::TermPrime) }   // Y
    };
    _parsingTable[ENonTerminal::Term][ETerminalType::VariableName] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Factor) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::TermPrime) }
    };

    // Y -> * F <Прог_12> Y | / F <Прог_13> Y | lambda
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Multiply] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Multiply) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Factor) },
        { EStackItemType::SemanticAction, 12 }, 
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::TermPrime) }
    };
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Divide] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Divide) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Factor) },
        { EStackItemType::SemanticAction, 13 }, 
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::TermPrime) }
    };
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Plus] = {};
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Minus] = {};
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Semicolon] = {};
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::RightParen] = {};

    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Less] = {};
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Greater] = {};
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::Equal] = {};
    _parsingTable[ENonTerminal::TermPrime][ETerminalType::NotEqual] = {};

    // ---- ПРАВИЛА ДЛЯ УСЛОВИЙ: Condition -> Expression ConditionPrime ----
    _parsingTable[ENonTerminal::Condition][ETerminalType::Number] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::ConditionPrime) }
    };
    _parsingTable[ENonTerminal::Condition][ETerminalType::VariableName] = {
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::ConditionPrime) }
    };

    // ---- ХВОСТ УСЛОВИЯ: ConditionPrime -> < Expression <Прог_20> | > Expression <Прог_21> | == Expression <Прог_22> ----
    _parsingTable[ENonTerminal::ConditionPrime][ETerminalType::Less] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Less) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
        { EStackItemType::SemanticAction, 18 } // Пишем оператор '<' в ОПС
    };
    _parsingTable[ENonTerminal::ConditionPrime][ETerminalType::Greater] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Greater) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
        { EStackItemType::SemanticAction, 19 } // Пишем оператор '>' в ОПС
    };
    _parsingTable[ENonTerminal::ConditionPrime][ETerminalType::Equal] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Equal) },
        { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
        { EStackItemType::SemanticAction, 20 } // Пишем оператор '==' в ОПС
    };

    _parsingTable[ENonTerminal::ConditionPrime][ETerminalType::NotEqual] = {
    { EStackItemType::Terminal,       static_cast<int>(ETerminalType::NotEqual) },
    { EStackItemType::NonTerminal,    static_cast<int>(ENonTerminal::Expression) },
    { EStackItemType::SemanticAction, 16 } // Записать операцию '!=' в ОПС
    };
    // (При необходимости добавь аналогично правила для LessEqual, GreaterEqual, LessGreater/NotEqual)
     _parsingTable[ENonTerminal::ConditionPrime][ETerminalType::RightParen] = {}; // Эпсилон-правило
     _parsingTable[ENonTerminal::ConditionPrime][ETerminalType::Semicolon] = {}; // Эпсилон-правило
        // Неуверен в 2 выше перечисленных эпсилон-правилах, проверь по грамматике. Возможно, их нужно убрать, если после условия всегда должен идти оператор (например, в if или while), а не точка с запятой или закрывающая скобка.
    // ---- ФАКТОРЫ: F -> ID | INT ----
    _parsingTable[ENonTerminal::Factor][ETerminalType::VariableName] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::VariableName) }
    };
    _parsingTable[ENonTerminal::Factor][ETerminalType::Number] = {
        { EStackItemType::Terminal,       static_cast<int>(ETerminalType::Number) }
    };
}

void LL1Parser::parse(const std::string& sourceCode) {
    // 1. Извлекаем токены из вашего лексера (Определяем tokens)
    if (!LexicalAnalyzer::isLexicalCorrect(sourceCode)) {
        throw std::runtime_error("Лексическая ошибка. Синтаксический анализ невозможен.");
    }
    auto tokens = LexicalAnalyzer::getTerminals();
    size_t tokenIdx = 0; // Определяем tokenIdx

    // Очищаем структуры данных от предыдущих запусков
    while (!_parseStack.empty()) _parseStack.pop();
    while (!_labelStack.empty()) _labelStack.pop();
    _rpn.clear();

    // 2. Инициализация магазина: маркер дна (EndOfFile) и стартовый нетерминал
    _parseStack.push({ EStackItemType::Terminal, static_cast<int>(ETerminalType::EndOfFile) });
    _parseStack.push({ EStackItemType::NonTerminal, static_cast<int>(ENonTerminal::Program) });

    // 3. Главный цикл магазинного автомата
    while (!_parseStack.empty()) {
        StackItem top = _parseStack.top(); // Определяем top, извлекая вершину магазина
        _parseStack.pop();

        // Берем текущий токен. Если лента пуста — подставляем EndOfFile
        std::shared_ptr<Terminal> currentToken = (tokenIdx < tokens.size()) 
            ? tokens[tokenIdx] 
            : std::make_shared<Terminal>(ETerminalType::EndOfFile, 0, 0);

        if (top.type == EStackItemType::Terminal) {
            // Сравниваем тип напрямую через публичное поле terminalType
            if (top.id == static_cast<int>(currentToken->terminalType)) {
                
                ETerminalType currentType = currentToken->terminalType;

                // Если токен — это операнд, достаем его значение и кидаем в ОПС
                if (currentType == ETerminalType::VariableName || 
                    currentType == ETerminalType::Number || 
                    currentType == ETerminalType::TextLine ||
                    currentType == ETerminalType::Boolean) {
                    
                    std::string valStr = "";
                    
                    // Безопасно приводим указатель к нужному классу-наследнику
                    if (currentType == ETerminalType::VariableName) {
                        auto t = std::static_pointer_cast<TerminalIdentifier>(currentToken);
                        valStr = t->name;
                    } else if (currentType == ETerminalType::Number) {
                        auto t = std::static_pointer_cast<TerminalNumber>(currentToken);
                        valStr = std::to_string(t->data);
                    } else if (currentType == ETerminalType::TextLine) {
                        auto t = std::static_pointer_cast<TerminalTextLine>(currentToken);
                        valStr = t->data;
                    } else if (currentType == ETerminalType::Boolean) {
                        auto t = std::static_pointer_cast<TerminalBoolean>(currentToken);
                        valStr = t->data ? "true" : "false";
                    }
                    
                    _rpn.push_back({ valStr, false });
                }

                if (currentType != ETerminalType::EndOfFile) {
                    tokenIdx++; // Продвигаем ленту токенов
                }
            } else {
                throw std::runtime_error("Синтаксическая ошибка: Неожиданный токен.");
            }
        }
     if (top.type == EStackItemType::NonTerminal) {
            ENonTerminal nonTerm = static_cast<ENonTerminal>(top.id);
            ETerminalType term = currentToken->terminalType;

            // 1. Ищем строку таблицы для данного нетерминала
            auto itRow = _parsingTable.find(nonTerm);
            
            // 2. Проверяем, существует ли правило для этой пары (Нетерминал, Токен)
            if (itRow == _parsingTable.end() || itRow->second.find(term) == itRow->second.end()) {
                // Выбрасываем исключение только если правила РЕАЛЬНО нет в таблице
                throw std::runtime_error(
                    "Parsing error: No rule for NonTerminal ID " + std::to_string(top.id) +
                    " with Token ID " + std::to_string(static_cast<int>(term)) + 
                    " at line " + std::to_string(currentToken->linePointer)
                );
            }

            // 3. Если правило найдено (даже если это эпсилон-правило {}), раскрываем его
            auto rule = itRow->second[term];
            for (auto it = rule.rbegin(); it != rule.rend(); ++it) {
                _parseStack.push(*it);
            }
        }
        else if (top.type == EStackItemType::SemanticAction) {
            // Вызов табличного семантического генератора
            executeSemanticAction(top.id);
        }
    }
    
    std::cout << "Трансляция выполнена успешно! ОПС построена без ошибок.\n";
}

void LL1Parser::executeSemanticAction(int actionId) {
    int k = static_cast<int>(_rpn.size()); // Текущий размер ОПС (индекс будущей команды)

    switch (actionId) {
        case 1: // Программа 1 (Встретили 'do' или тело 'if')
            _labelStack.push(k);              // Сохраняем индекс, где будет лежать адрес перехода
            _rpn.push_back({ "__", true });   // Резервируем место под адрес (пустышка)
            _rpn.push_back({ "JF", false });  // Операция условного перехода
            break;

        case 2: { // Программа 2 (Конец блока IF, перед блоком ELSE)
            // 1. Извлекаем из стека меток адрес пустышки условного перехода 'jf', созданной в case 1
            int ifFalseLabelAddress = _labelStack.top(); _labelStack.pop();
            
            // 2. Генерируем в ОПС безусловный переход 'j', который выполнится в конце блока IF, 
            // чтобы перепрыгнуть ветку ELSE. Тоже резервируем пустышку.
            _labelStack.push(k);             // Запоминаем текущий индекс ОПС в стек меток
            _rpn.push_back({ "__", true });  // Пустышка адреса для выхода из всего IF-ELSE
            _rpn.push_back({ "JMP", false });   // Операция безусловного перехода
            
            // 3. Заполняем адрес 'jf' истинным значением. Куда прыгать, если условие ложно?
            // Прямо сюда — на первую команду блока ELSE, которая пойдет сразу за операцией 'j'!
            int elseStartAddress = static_cast<int>(_rpn.size());
            _rpn[ifFalseLabelAddress].value = std::to_string(elseStartAddress);
            break;
        }

        case 3: { // Программа 3 (Конец всего оператора IF-ELSE, после блока ELSE)
            // 1. Достаем адрес пустышки перехода 'j', созданной в case 2
            int exitLabelAddress = _labelStack.top(); _labelStack.pop();
            
            // 2. Вычисляем адрес команды, которая идет сразу за конструкцией if-else
            int nextCommandAddress = static_cast<int>(_rpn.size());
            
            // 3. Заполняем пустышку истинным адресом выхода
            _rpn[exitLabelAddress].value = std::to_string(nextCommandAddress);
            break;
        }

        case 4: // Программа 4 (Перед проверкой условия цикла while)
            _labelStack.push(k);              // Запоминаем точку возврата на проверку условия
            break;

        case 5: { // Программа 5 (Конец тела цикла while)
            int exitLabelAddress = _labelStack.top(); _labelStack.pop(); // Достаем адрес пустышки jf
            int loopStartAddress = _labelStack.top(); _labelStack.pop(); // Достаем адрес начала условия
            
            // 1. Генерируем безусловный прыжок наверх, к условию
            _rpn.push_back({ std::to_string(loopStartAddress), true });
            _rpn.push_back({ "JMP", false });
            
            // 2. Рассчитываем адрес выхода (это текущая позиция в ОПС сразу за операцией 'j')
            int nextCommandAddress = static_cast<int>(_rpn.size());
            
            // 3. Возвращаемся по сохраненному индексу и заполняем пустышку истинным адресом
            _rpn[exitLabelAddress].value = std::to_string(nextCommandAddress);
            break;
        }

        // Рядовые операции (из раздела 5 вашего Report.md)
        case 10: _rpn.push_back({ "+", false }); break;
        case 11: _rpn.push_back({ "-", false }); break;
        case 12: _rpn.push_back({ "*", false }); break;
        case 13: _rpn.push_back({ "/", false }); break;
        case 14: _rpn.push_back({ ":=", false }); break;
        case 15: _rpn.push_back({ "Output", false }); break;
        case 16: _rpn.push_back({ "!=", false }); break;
        case 17: _rpn.push_back({ "decl_var", false }); break; // Объявление переменной // Отправляем команду в ОПС
        case 18: _rpn.push_back({ "<", false }); break;
        case 19: _rpn.push_back({ ">", false }); break;
        case 20: _rpn.push_back({ "==", false }); break;
    }
}

void LL1Parser::printRpn() const {
    std::cout << "Результат ОПС: ";
    for (size_t i = 0; i < _rpn.size(); ++i) {
        std::cout << "[" << i << "]:" << _rpn[i].value << " ";
    }
    std::cout << "\n";
}