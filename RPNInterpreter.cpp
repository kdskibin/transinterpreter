#include "RPNInterpreter.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cmath>

// ────────────────────────────────────────────────────────────
//  Вспомогательные методы
// ────────────────────────────────────────────────────────────

int RPNInterpreter::resolveNumberOperand(std::shared_ptr<RPNSymbol> sym,
                                          const std::map<std::string, std::vector<std::string>>& variables) {
    if (auto* n = dynamic_cast<RPNNumber*>(sym.get()))
        return n->data;

    if (auto* id = dynamic_cast<RPNIdentifier*>(sym.get())) {
        auto it = variables.find(id->name);
        if (it == variables.end())
            throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
        if (it->second[0] != "A_Number")
            throw std::runtime_error("Операция применима только к числам. Переменная '" + id->name + "' не число.");
        return std::stoi(it->second[1]);
    }

    if (auto* aa = dynamic_cast<RPNArrayAccess*>(sym.get())) {
        auto it = variables.find(aa->arrayName);
        if (it == variables.end())
            throw std::runtime_error("Массив '" + aa->arrayName + "' не объявлен.");
        std::string val = it->second[aa->index + 2];
        if (val.empty()) val = "0";
        return std::stoi(val);
    }

    throw std::runtime_error("Неверный тип операнда для числовой операции.");
}

// ────────────────────────────────────────────────────────────
//  Точка входа
// ────────────────────────────────────────────────────────────

void RPNInterpreter::executeInstructions(const std::vector<std::shared_ptr<RPNSymbol>>& rpn) {
    std::map<std::string, std::vector<std::string>> variables;
    std::vector<std::shared_ptr<RPNSymbol>> stack;

    for (int iteration = 0; iteration < static_cast<int>(rpn.size()); iteration++) {
        std::shared_ptr<RPNSymbol> symbol = rpn[iteration];
        ERPNType type = symbol->rpnType;

        // ── Операнды и метки — кладем в стек ──
        if (type == ERPNType::A_VariableName ||
            type == ERPNType::A_Number ||
            type == ERPNType::A_TextLine ||
            type == ERPNType::A_Boolean ||
            type == ERPNType::M_Mark) {
            stack.push_back(symbol);
            continue;
        }

        // ── Объявление переменных ──
        if (type == ERPNType::F_Int || type == ERPNType::F_String || type == ERPNType::F_Bool) {
            if (auto* var = dynamic_cast<RPNIdentifier*>(stack.back().get())) {
                stack.pop_back();
                if (variables.count(var->name))
                    throw std::runtime_error("Переменная '" + var->name + "' уже объявлена.");

                std::string varType;
                if (type == ERPNType::F_Int) varType = "A_Number";
                else if (type == ERPNType::F_String) varType = "A_TextLine";
                else varType = "A_Boolean";

                variables[var->name] = {varType, ""};
                std::cout << "int " << var->name << std::endl;
            } else {
                throw std::runtime_error("После типа 'int' ожидался идентификатор.");
            }
            continue;
        }

        // ── Объявление массивов ──
        if (type == ERPNType::F_IntArray || type == ERPNType::F_StringArray || type == ERPNType::F_BoolArray) {
            if (auto* var = dynamic_cast<RPNIdentifier*>(stack.back().get())) {
                stack.pop_back();
                if (variables.count(var->name))
                    throw std::runtime_error("Переменная '" + var->name + "' уже объявлена.");

                if (auto* sizeSym = dynamic_cast<RPNNumber*>(stack.back().get())) {
                    stack.pop_back();
                    int size = sizeSym->data;

                    std::string varType;
                    if (type == ERPNType::F_IntArray) varType = "A_Number";
                    else if (type == ERPNType::F_StringArray) varType = "A_TextLine";
                    else varType = "A_Boolean";

                    variables[var->name] = {varType, std::to_string(size)};
                    for (int i = 0; i < size; i++)
                        variables[var->name].push_back("");

                    std::cout << (type == ERPNType::F_IntArray ? "int" : type == ERPNType::F_StringArray ? "string" : "bool")
                              << "[] " << var->name << " = " << size << std::endl;
                } else {
                    throw std::runtime_error("После типа массива ожидалось число элементов.");
                }
            } else {
                throw std::runtime_error("После типа массива ожидался идентификатор.");
            }
            continue;
        }

        // ── Присваивание ──
        if (type == ERPNType::F_Assignment) {
            if (stack.size() < 2)
                throw std::runtime_error("Ошибка присваивания: недостаточно операндов.");

            auto valueSym = stack.back(); stack.pop_back();
            auto targetSym = stack.back(); stack.pop_back();

            // Присваивание элементу массива: target = arrayAccess
            if (auto* targetArr = dynamic_cast<RPNArrayAccess*>(targetSym.get())) {
                auto it = variables.find(targetArr->arrayName);
                if (it == variables.end())
                    throw std::runtime_error("Массив '" + targetArr->arrayName + "' не объявлен.");
                if (it->second[0] != "A_Number")
                    throw std::runtime_error("Тип массива несовместим с присваиванием числа.");

                if (auto* num = dynamic_cast<RPNNumber*>(valueSym.get())) {
                    it->second[targetArr->index + 2] = std::to_string(num->data);
                    std::cout << targetArr->arrayName << "[" << targetArr->index << "] = " << num->data << std::endl;
                } else if (auto* srcArr = dynamic_cast<RPNArrayAccess*>(valueSym.get())) {
                    auto srcIt = variables.find(srcArr->arrayName);
                    if (srcIt == variables.end())
                        throw std::runtime_error("Массив '" + srcArr->arrayName + "' не объявлен.");
                    std::string val = srcIt->second[srcArr->index + 2];
                    if (val.empty()) val = "0";
                    it->second[targetArr->index + 2] = val;
                    std::cout << targetArr->arrayName << "[" << targetArr->index << "] = " << val << std::endl;
                } else if (auto* valId = dynamic_cast<RPNIdentifier*>(valueSym.get())) {
                    auto valIt = variables.find(valId->name);
                    if (valIt == variables.end())
                        throw std::runtime_error("Переменная '" + valId->name + "' не объявлена.");
                    it->second[targetArr->index + 2] = valIt->second[1];
                    std::cout << targetArr->arrayName << "[" << targetArr->index << "] = " << valIt->second[1] << std::endl;
                } else {
                    throw std::runtime_error("Неподдерживаемый тип значения для присваивания элементу массива.");
                }
            }
            // Присваивание скалярной переменной: target = value
            else if (auto* targetVar = dynamic_cast<RPNIdentifier*>(targetSym.get())) {
                std::string varName = targetVar->name;
                auto it = variables.find(varName);
                if (it == variables.end())
                    throw std::runtime_error("Переменная '" + varName + "' не объявлена.");

                if (auto* num = dynamic_cast<RPNNumber*>(valueSym.get())) {
                    if (it->second[0] != "A_Number")
                        throw std::runtime_error("Несоответствие типов: нельзя присвоить число переменной типа '" + it->second[0] + "'.");
                    it->second[1] = std::to_string(num->data);
                    std::cout << varName << " = " << num->data << std::endl;
                } else if (auto* arrAcc = dynamic_cast<RPNArrayAccess*>(valueSym.get())) {
                    auto arrIt = variables.find(arrAcc->arrayName);
                    if (arrIt == variables.end())
                        throw std::runtime_error("Массив '" + arrAcc->arrayName + "' не объявлен.");
                    if (it->second[0] != "A_Number")
                        throw std::runtime_error("Несоответствие типов.");
                    std::string val = arrIt->second[arrAcc->index + 2];
                    if (val.empty()) val = "0";
                    it->second[1] = val;
                    std::cout << varName << " = " << val << std::endl;
                } else if (auto* txt = dynamic_cast<RPNTextLine*>(valueSym.get())) {
                    if (it->second[0] != "A_TextLine")
                        throw std::runtime_error("Несоответствие типов: нельзя присвоить строку.");
                    it->second[1] = txt->data;
                    std::cout << varName << " = " << txt->data << std::endl;
                } else if (auto* bl = dynamic_cast<RPNBoolean*>(valueSym.get())) {
                    if (it->second[0] != "A_Boolean")
                        throw std::runtime_error("Несоответствие типов: нельзя присвоить булево.");
                    it->second[1] = bl->data ? "true" : "false";
                    std::cout << varName << " = " << (bl->data ? "true" : "false") << std::endl;
                } else if (auto* srcVar = dynamic_cast<RPNIdentifier*>(valueSym.get())) {
                    auto srcIt = variables.find(srcVar->name);
                    if (srcIt == variables.end())
                        throw std::runtime_error("Переменная '" + srcVar->name + "' не объявлена.");
                    if (it->second[0] != srcIt->second[0])
                        throw std::runtime_error("Несоответствие типов при присваивании.");
                    it->second[1] = srcIt->second[1];
                    std::cout << varName << " = " << srcIt->second[1] << std::endl;
                } else {
                    throw std::runtime_error("Неподдерживаемый тип значения для присваивания.");
                }
            }
            else {
                throw std::runtime_error("Неверный тип цели для присваивания.");
            }
            continue;
        }

        // ── Сравнения ──
        if (type == ERPNType::F_Greater || type == ERPNType::F_Less || type == ERPNType::F_Equal ||
            type == ERPNType::F_GreaterEqual || type == ERPNType::F_LessEqual) {
            if (stack.size() < 2)
                throw std::runtime_error("Ошибка сравнения: недостаточно операндов.");

            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();

            // Числовое сравнение
            if ((dynamic_cast<RPNIdentifier*>(op1.get()) || dynamic_cast<RPNNumber*>(op1.get())) &&
                (dynamic_cast<RPNIdentifier*>(op2.get()) || dynamic_cast<RPNNumber*>(op2.get()))) {
                int val1 = resolveNumberOperand(op1, variables);
                int val2 = resolveNumberOperand(op2, variables);
                bool result = false;
                switch (type) {
                    case ERPNType::F_Greater:   result = val1 > val2; break;
                    case ERPNType::F_Less:      result = val1 < val2; break;
                    case ERPNType::F_Equal:     result = val1 == val2; break;
                    case ERPNType::F_GreaterEqual: result = val1 >= val2; break;
                    case ERPNType::F_LessEqual:   result = val1 <= val2; break;
                    default: break;
                }
                stack.push_back(std::make_shared<RPNBoolean>(result));
            }
            // Строковое сравнение (только ==)
            else if (type == ERPNType::F_Equal &&
                     ((dynamic_cast<RPNIdentifier*>(op1.get()) || dynamic_cast<RPNTextLine*>(op1.get())) &&
                      (dynamic_cast<RPNIdentifier*>(op2.get()) || dynamic_cast<RPNTextLine*>(op2.get())))) {
                std::string s1, s2;
                if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) {
                    auto it = variables.find(id->name);
                    if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                    if (it->second[0] != "A_TextLine") throw std::runtime_error("Операция '==' для строк: переменная '" + id->name + "' не строка.");
                    s1 = it->second[1];
                } else {
                    s1 = dynamic_cast<RPNTextLine*>(op1.get())->data;
                }
                if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) {
                    auto it = variables.find(id->name);
                    if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                    if (it->second[0] != "A_TextLine") throw std::runtime_error("Операция '==' для строк: переменная '" + id->name + "' не строка.");
                    s2 = it->second[1];
                } else {
                    s2 = dynamic_cast<RPNTextLine*>(op2.get())->data;
                }
                bool result = (s1 == s2);
                stack.push_back(std::make_shared<RPNBoolean>(result));
            }
            // Булево сравнение (только ==)
            else if (type == ERPNType::F_Equal &&
                     ((dynamic_cast<RPNIdentifier*>(op1.get()) || dynamic_cast<RPNBoolean*>(op1.get())) &&
                      (dynamic_cast<RPNIdentifier*>(op2.get()) || dynamic_cast<RPNBoolean*>(op2.get())))) {
                bool b1, b2;
                if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) {
                    auto it = variables.find(id->name);
                    if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                    if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '==' для булевых: переменная '" + id->name + "' не булева.");
                    b1 = (it->second[1] == "true");
                } else {
                    b1 = dynamic_cast<RPNBoolean*>(op1.get())->data;
                }
                if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) {
                    auto it = variables.find(id->name);
                    if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                    if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '==' для булевых: переменная '" + id->name + "' не булева.");
                    b2 = (it->second[1] == "true");
                } else {
                    b2 = dynamic_cast<RPNBoolean*>(op2.get())->data;
                }
                bool result = (b1 == b2);
                stack.push_back(std::make_shared<RPNBoolean>(result));
            }
            else {
                throw std::runtime_error("Несоответствие типов для операции сравнения.");
            }
            continue;
        }

        // ── Логические операции ──
        if (type == ERPNType::F_And) {
            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();

            bool v1, v2;
            if (auto* b = dynamic_cast<RPNBoolean*>(op1.get())) v1 = b->data;
            else if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '&&' применима только к булевым.");
                v1 = (it->second[1] == "true");
            }
            else throw std::runtime_error("Неверный тип первого операнда для '&&'.");

            if (auto* b = dynamic_cast<RPNBoolean*>(op2.get())) v2 = b->data;
            else if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '&&' применима только к булевым.");
                v2 = (it->second[1] == "true");
            }
            else throw std::runtime_error("Неверный тип второго операнда для '&&'.");

            stack.push_back(std::make_shared<RPNBoolean>(v1 && v2));
            continue;
        }

        if (type == ERPNType::F_Or) {
            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();

            bool v1, v2;
            if (auto* b = dynamic_cast<RPNBoolean*>(op1.get())) v1 = b->data;
            else if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '||' применима только к булевым.");
                v1 = (it->second[1] == "true");
            }
            else throw std::runtime_error("Неверный тип первого операнда для '||'.");

            if (auto* b = dynamic_cast<RPNBoolean*>(op2.get())) v2 = b->data;
            else if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '||' применима только к булевым.");
                v2 = (it->second[1] == "true");
            }
            else throw std::runtime_error("Неверный тип второго операнда для '||'.");

            stack.push_back(std::make_shared<RPNBoolean>(v1 || v2));
            continue;
        }

        if (type == ERPNType::F_Not) {
            auto op1 = stack.back(); stack.pop_back();

            bool v1;
            if (auto* b = dynamic_cast<RPNBoolean*>(op1.get())) v1 = b->data;
            else if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                if (it->second[0] != "A_Boolean") throw std::runtime_error("Операция '!' применима только к булевым.");
                v1 = (it->second[1] == "true");
            }
            else throw std::runtime_error("Неверный тип операнда для '!'.");

            stack.push_back(std::make_shared<RPNBoolean>(!v1));
            continue;
        }

        // ── Управление потоком ──
        if (type == ERPNType::F_ConditionalJumpToMark) {
            if (stack.size() < 2)
                throw std::runtime_error("Ошибка условного перехода: недостаточно операндов.");

            auto markSym = stack.back(); stack.pop_back();
            auto* mark = dynamic_cast<RPNMark*>(markSym.get());
            if (!mark) throw std::runtime_error("Ошибка условного перехода: ожидалась метка.");

            auto condSym = stack.back(); stack.pop_back();
            bool condValue;
            if (auto* b = dynamic_cast<RPNBoolean*>(condSym.get())) {
                condValue = b->data;
            } else if (auto* id = dynamic_cast<RPNIdentifier*>(condSym.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                if (it->second[0] != "A_Boolean") throw std::runtime_error("Условие должно быть булевым.");
                condValue = (it->second[1] == "true");
            }
            else throw std::runtime_error("Ошибка условного перехода: ожидалось булево условие.");

            if (!condValue && mark->position.has_value()) {
                iteration = mark->position.value() - 1;
            }
            continue;
        }

        if (type == ERPNType::F_UnconditionalJumpToMark) {
            auto markSym = stack.back(); stack.pop_back();
            auto* mark = dynamic_cast<RPNMark*>(markSym.get());
            if (!mark) throw std::runtime_error("Ошибка безусловного перехода: ожидалась метка.");
            if (!mark->position.has_value())
                throw std::runtime_error("Ошибка безусловного перехода: позиция метки не установлена.");
            iteration = mark->position.value() - 1;
            continue;
        }

        // ── Арифметические операции ──
        if (type == ERPNType::F_Plus) {
            if (stack.size() < 2)
                throw std::runtime_error("Ошибка '+': недостаточно операндов.");

            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();

            // Определяем типы
            std::string type1, type2;
            if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                type1 = it->second[0];
            } else if (dynamic_cast<RPNNumber*>(op1.get())) type1 = "A_Number";
            else if (dynamic_cast<RPNTextLine*>(op1.get())) type1 = "A_TextLine";
            else throw std::runtime_error("Неподдерживаемый тип первого операнда для '+'.");

            if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                type2 = it->second[0];
            } else if (dynamic_cast<RPNNumber*>(op2.get())) type2 = "A_Number";
            else if (dynamic_cast<RPNTextLine*>(op2.get())) type2 = "A_TextLine";
            else throw std::runtime_error("Неподдерживаемый тип второго операнда для '+'.");

            if (type1 == "A_Number" && type2 == "A_Number") {
                int v1 = resolveNumberOperand(op1, variables);
                int v2 = resolveNumberOperand(op2, variables);
                stack.push_back(std::make_shared<RPNNumber>(ERPNType::A_Number, v1 + v2));
            } else if (type1 == "A_TextLine" && type2 == "A_TextLine") {
                std::string s1, s2;
                if (auto* id = dynamic_cast<RPNIdentifier*>(op1.get())) s1 = variables.at(id->name)[1];
                else s1 = dynamic_cast<RPNTextLine*>(op1.get())->data;
                if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) s2 = variables.at(id->name)[1];
                else s2 = dynamic_cast<RPNTextLine*>(op2.get())->data;
                stack.push_back(std::make_shared<RPNTextLine>(ERPNType::A_TextLine, s1 + s2));
            } else {
                throw std::runtime_error("Несоответствие типов для операции '+'.");
            }
            continue;
        }

        if (type == ERPNType::F_Minus) {
            if (stack.empty())
                throw std::runtime_error("Ошибка '-': стек пуст.");

            auto op2 = stack.back(); stack.pop_back();

            // Унарный минус: проверяем, есть ли еще элемент
            bool isUnary = stack.empty();

            if (isUnary) {
                int val;
                if (auto* id = dynamic_cast<RPNIdentifier*>(op2.get())) {
                    auto it = variables.find(id->name);
                    if (it == variables.end()) throw std::runtime_error("Переменная '" + id->name + "' не объявлена.");
                    if (it->second[0] != "A_Number") throw std::runtime_error("Унарный минус применим только к числам.");
                    val = std::stoi(it->second[1]);
                } else if (auto* n = dynamic_cast<RPNNumber*>(op2.get())) {
                    val = n->data;
                } else {
                    throw std::runtime_error("Неверный тип операнда для унарного минуса.");
                }
                stack.push_back(std::make_shared<RPNNumber>(ERPNType::A_Number, -val));
            } else {
                auto op1 = stack.back(); stack.pop_back();
                int v1 = resolveNumberOperand(op1, variables);
                int v2 = resolveNumberOperand(op2, variables);
                stack.push_back(std::make_shared<RPNNumber>(ERPNType::A_Number, v1 - v2));
            }
            continue;
        }

        if (type == ERPNType::F_Multiply) {
            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();
            int v1 = resolveNumberOperand(op1, variables);
            int v2 = resolveNumberOperand(op2, variables);
            stack.push_back(std::make_shared<RPNNumber>(ERPNType::A_Number, v1 * v2));
            continue;
        }

        if (type == ERPNType::F_Divide) {
            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();
            int v1 = resolveNumberOperand(op1, variables);
            int v2 = resolveNumberOperand(op2, variables);
            if (v2 == 0) throw std::runtime_error("Деление на ноль.");
            stack.push_back(std::make_shared<RPNNumber>(ERPNType::A_Number, v1 / v2));
            continue;
        }

        if (type == ERPNType::F_Modulus) {
            auto op2 = stack.back(); stack.pop_back();
            auto op1 = stack.back(); stack.pop_back();
            int v1 = resolveNumberOperand(op1, variables);
            int v2 = resolveNumberOperand(op2, variables);
            if (v2 == 0) throw std::runtime_error("Деление на ноль (при операции остатка).");
            stack.push_back(std::make_shared<RPNNumber>(ERPNType::A_Number, v1 % v2));
            continue;
        }

        // ── Индексация массива ──
        if (type == ERPNType::F_Index) {
            if (stack.size() < 2)
                throw std::runtime_error("Ошибка индексации: недостаточно операндов.");

            auto indexSym = stack.back(); stack.pop_back();
            auto arraySym = stack.back(); stack.pop_back();

            auto* arrId = dynamic_cast<RPNIdentifier*>(arraySym.get());
            if (!arrId) throw std::runtime_error("Ошибка индексации: ожидалось имя массива.");

            auto it = variables.find(arrId->name);
            if (it == variables.end())
                throw std::runtime_error("Массив '" + arrId->name + "' не объявлен.");

            int idxVal;
            if (auto* id = dynamic_cast<RPNIdentifier*>(indexSym.get())) {
                auto vit = variables.find(id->name);
                if (vit == variables.end()) throw std::runtime_error("Переменная индекса '" + id->name + "' не объявлена.");
                if (vit->second[0] != "A_Number") throw std::runtime_error("Индекс массива должен быть числом.");
                idxVal = std::stoi(vit->second[1]);
            } else if (auto* n = dynamic_cast<RPNNumber*>(indexSym.get())) {
                idxVal = n->data;
            } else {
                throw std::runtime_error("Неверный тип для индекса массива.");
            }

            int arrSize = std::stoi(it->second[1]);
            if (idxVal < 0 || idxVal >= arrSize)
                throw std::runtime_error("Индекс '" + std::to_string(idxVal) + "' выходит за границы массива '" + arrId->name + "' размером '" + std::to_string(arrSize) + "'.");

            stack.push_back(std::make_shared<RPNArrayAccess>(arrId->name, idxVal));
            continue;
        }

        // ── Вывод ──
        if (type == ERPNType::F_Output) {
            if (stack.empty())
                throw std::runtime_error("Ошибка Output: стек пуст.");

            auto top = stack.back(); stack.pop_back();

            if (auto* id = dynamic_cast<RPNIdentifier*>(top.get())) {
                auto it = variables.find(id->name);
                if (it == variables.end())
                    throw std::runtime_error("Переменная '" + id->name + "' не объявлена для вывода.");

                // Если size > 2, это массив
                if (static_cast<int>(it->second.size()) > 2) {
                    std::cout << id->name << "[]: ";
                    for (size_t k = 2; k < it->second.size(); k++) {
                        std::cout << it->second[k] << (k == it->second.size() - 1 ? "" : ", ");
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << it->second[1] << std::endl;
                }
            } else if (auto* arrAcc = dynamic_cast<RPNArrayAccess*>(top.get())) {
                auto it = variables.find(arrAcc->arrayName);
                if (it == variables.end())
                    throw std::runtime_error("Массив '" + arrAcc->arrayName + "' не объявлен.");
                int arrSize = std::stoi(it->second[1]);
                if (arrAcc->index < 0 || arrAcc->index >= arrSize)
                    throw std::runtime_error("Индекс '" + std::to_string(arrAcc->index) + "' выходит за границы.");
                std::string val = it->second[arrAcc->index + 2];
                if (val.empty()) val = "0";
                std::cout << val << std::endl;
            } else if (auto* n = dynamic_cast<RPNNumber*>(top.get())) {
                std::cout << n->data << std::endl;
            } else if (auto* txt = dynamic_cast<RPNTextLine*>(top.get())) {
                std::cout << txt->data << std::endl;
            } else if (auto* bl = dynamic_cast<RPNBoolean*>(top.get())) {
                std::cout << (bl->data ? "true" : "false") << std::endl;
            } else {
                throw std::runtime_error("Неверный тип аргумента для Output.");
            }
            continue;
        }

        // ── Ввод ──
        if (type == ERPNType::F_Input) {
            if (stack.empty())
                throw std::runtime_error("Ошибка Input: стек пуст.");

            auto top = stack.back(); stack.pop_back();
            auto* id = dynamic_cast<RPNIdentifier*>(top.get());
            if (!id) throw std::runtime_error("Неверный тип аргумента для Input.");

            auto it = variables.find(id->name);
            if (it == variables.end())
                throw std::runtime_error("Переменная '" + id->name + "' не объявлена для ввода.");

            std::string input;
            std::cout << "Введите значение для " << it->second[0] << " " << id->name << ": ";
            std::getline(std::cin, input);

            if (it->second[0] == "A_Number") {
                int val;
                if (!(std::stringstream(input) >> val))
                    throw std::runtime_error("Ошибка ввода: ожидалось целое число.");
                it->second[1] = std::to_string(val);
            } else if (it->second[0] == "A_TextLine") {
                it->second[1] = input;
            } else if (it->second[0] == "A_Boolean") {
                if (input == "true") it->second[1] = "true";
                else if (input == "false") it->second[1] = "false";
                else throw std::runtime_error("Ошибка ввода: ожидалось true/false.");
            } else {
                throw std::runtime_error("Неподдерживаемый тип переменной для Input.");
            }
            continue;
        }

        // Неизвестный тип
        throw std::runtime_error("Неизвестный или неподдерживаемый тип операции RPN: " + std::to_string(static_cast<int>(type)));
    }
}
