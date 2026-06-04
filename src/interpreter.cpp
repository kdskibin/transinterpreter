#include "interpreter.h"
#include <iostream>
#include <cmath>
#include <iomanip>

Interpreter::Interpreter(const OPS& ops, const SymbolTable& symbols)
    : ops(ops), symbols(symbols), debug(false), pc(0) {}

Interpreter::Interpreter(const OPS& ops, const SymbolTable& symbols, bool debug)
    : ops(ops), symbols(symbols), debug(debug), pc(0) {}

void Interpreter::printStack() const {
    std::cout << "    стек: [";
    for (size_t i = 0; i < evalStack.size(); i++) {
        if (i > 0) std::cout << ", ";
        const Value& v = evalStack[i];
        std::cout << v.typeName();
        if (v.type == Value::INT)
            std::cout << "(" << v.data.int_val << ")";
        else if (v.type == Value::REAL)
            std::cout << "(" << v.data.real_val << ")";
        else if (v.type == Value::BOOL)
            std::cout << "(" << (v.data.bool_val ? "true" : "false") << ")";
        else if (v.type == Value::ARRAY_REF || v.type == Value::VAR_INT_REF || v.type == Value::VAR_REAL_REF)
            std::cout << "(id=" << v.data.array_ref.array_id << ", idx=" << v.data.array_ref.index << ")";
    }
    std::cout << "]\n";
}

// Разыменование ссылки на переменную.
// Если на стеке лежит ссылка (VAR_INT_REF / VAR_REAL_REF / ARRAY_REF) -
// возвращаем реальное значение. Если уже значение - возвращаем как есть.
Value Interpreter::deref(const Value& v) {
    if (v.type == Value::VAR_INT_REF) {
        int id = v.data.array_ref.array_id;
        return Value::makeInt(int_vars.count(id) ? int_vars[id] : 0);
    }
    if (v.type == Value::VAR_REAL_REF) {
        int id = v.data.array_ref.array_id;
        return Value::makeReal(real_vars.count(id) ? real_vars[id] : 0.0);
    }
    if (v.type == Value::ARRAY_REF) {
        int id  = v.data.array_ref.array_id;
        int idx = v.data.array_ref.index;
        if (int_arrays.count(id))  return Value::makeInt(int_arrays[id][idx]);
        if (real_arrays.count(id)) return Value::makeReal(real_arrays[id][idx]);
    }
    return v; // уже значение (INT, REAL, BOOL)
}

// Получение операнда из элемента ОПС.
// Для переменных возвращает ссылку (VAR_INT_REF / VAR_REAL_REF),
// а не значение - это нужно чтобы := и read знали куда записывать.
Value Interpreter::getOperandValue(const OPSElement& elem) {
    if (elem.type != OPSElement::OPERAND) {
        throw RuntimeError("Ожидался операнд", elem.source_line, elem.source_column);
    }

    switch (elem.value.operand.op_type) {
        case VAR_INT: {
            Value ref;
            ref.type = Value::VAR_INT_REF;
            ref.data.array_ref.array_id = elem.value.operand.index;
            ref.data.array_ref.index    = -1;
            return ref;
        }
        case VAR_REAL: {
            Value ref;
            ref.type = Value::VAR_REAL_REF;
            ref.data.array_ref.array_id = elem.value.operand.index;
            ref.data.array_ref.index    = -1;
            return ref;
        }
        case CONST_INT:
            return Value::makeInt(symbols.getIntConst(elem.value.operand.index));
        case CONST_REAL:
            return Value::makeReal(symbols.getRealConst(elem.value.operand.index));
        case ARRAY_INT: {
            int id = elem.value.operand.index;
            // Ленивая инициализация: если массив ещё не создан — создаём
            if (int_arrays.find(id) == int_arrays.end()) {
                int_arrays[id].resize(10000, 0);
            }
            Value val;
            val.type = Value::ARRAY_REF;
            val.data.array_ref.array_id = id;
            val.data.array_ref.index    = -1;
            return val;
        }
        case ARRAY_REAL: {
            int id = elem.value.operand.index;
            // Ленивая инициализация: если массив ещё не создан — создаём
            if (real_arrays.find(id) == real_arrays.end()) {
                real_arrays[id].resize(10000, 0.0);
            }
            Value val;
            val.type = Value::ARRAY_REF;
            val.data.array_ref.array_id = id;
            val.data.array_ref.index    = -1;
            return val;
        }
        default:
            throw RuntimeError("Неизвестный тип операнда", elem.source_line, elem.source_column);
    }
}

// Запись значения в переменную или элемент массива
void Interpreter::setVariableValue(const Value& var_ref, const Value& value, int line, int col) {
    if (var_ref.type == Value::ARRAY_REF) {
        int array_id = var_ref.data.array_ref.array_id;
        int index    = var_ref.data.array_ref.index;

        if (int_arrays.find(array_id) != int_arrays.end()) {
            if (index < 0 || index >= (int)int_arrays[array_id].size())
                throw RuntimeError("Индекс массива выходит за границы", line, col);
            int_arrays[array_id][index] = value.toInt();
        } else if (real_arrays.find(array_id) != real_arrays.end()) {
            if (index < 0 || index >= (int)real_arrays[array_id].size())
                throw RuntimeError("Индекс массива выходит за границы", line, col);
            real_arrays[array_id][index] = value.toReal();
        } else {
            throw RuntimeError("Массив не инициализирован", line, col);
        }
    } else {
        throw RuntimeError("Ожидалась переменная или элемент массива", line, col);
    }
}

// Выполнение арифметической операции (+, -, *, /)
void Interpreter::executeArithmetic(OperationType op, const OPSElement& elem) {
    Value b = deref(evalStack.back()); evalStack.pop_back();
    Value a = deref(evalStack.back()); evalStack.pop_back();

    bool is_real = (a.type == Value::REAL || b.type == Value::REAL);
    double a_val = a.toReal();
    double b_val = b.toReal();
    double result;

    switch (op) {
        case OP_ADD: result = a_val + b_val; break;
        case OP_SUB: result = a_val - b_val; break;
        case OP_MUL: result = a_val * b_val; break;
        case OP_DIV:
            if (std::abs(b_val) < 1e-10)
                throw RuntimeError("Деление на ноль", elem.source_line, elem.source_column);
            result = a_val / b_val;
            break;
        default:
            throw RuntimeError("Неизвестная арифметическая операция", elem.source_line, elem.source_column);
    }

    if (is_real) evalStack.push_back(Value::makeReal(result));
    else         evalStack.push_back(Value::makeInt((int)result));
}

// Выполнение операции сравнения (<, >, <=, >=, =, <>)
void Interpreter::executeComparison(OperationType op, const OPSElement& elem) {
    Value b = deref(evalStack.back()); evalStack.pop_back();
    Value a = deref(evalStack.back()); evalStack.pop_back();

    double a_val = a.toReal();
    double b_val = b.toReal();
    bool result;

    switch (op) {
        case OP_LT: result = a_val <  b_val; break;
        case OP_GT: result = a_val >  b_val; break;
        case OP_LE: result = a_val <= b_val; break;
        case OP_GE: result = a_val >= b_val; break;
        case OP_EQ: result = std::abs(a_val - b_val) <  1e-10; break;
        case OP_NE: result = std::abs(a_val - b_val) >= 1e-10; break;
        default:
            throw RuntimeError("Неизвестная операция сравнения", elem.source_line, elem.source_column);
    }

    evalStack.push_back(Value::makeBool(result));
}

// Выполнение присваивания (:=)
// Левый операнд должен быть ссылкой на переменную или элемент массива
void Interpreter::executeAssignment(const OPSElement& elem) {
    Value value = deref(evalStack.back()); evalStack.pop_back();
    Value var   = evalStack.back();        evalStack.pop_back();

    if (var.type == Value::VAR_INT_REF) {
        int id = var.data.array_ref.array_id;
        int_vars[id] = value.toInt();
    } else if (var.type == Value::VAR_REAL_REF) {
        int id = var.data.array_ref.array_id;
        real_vars[id] = value.toReal();
    } else if (var.type == Value::ARRAY_REF) {
        setVariableValue(var, value, elem.source_line, elem.source_column);
    } else {
        throw RuntimeError("Ожидалась переменная в левой части :=", elem.source_line, elem.source_column);
    }
}

// Выполнение индексирования массива (операция i)
void Interpreter::executeIndexing(const OPSElement& elem) {
    Value index_val = deref(evalStack.back()); evalStack.pop_back();
    Value array_ref = evalStack.back();        evalStack.pop_back();

    if (array_ref.type != Value::ARRAY_REF) {
        throw RuntimeError("Ожидался массив", elem.source_line, elem.source_column);
    }

    Value result;
    result.type = Value::ARRAY_REF;
    result.data.array_ref.array_id = array_ref.data.array_ref.array_id;
    result.data.array_ref.index    = index_val.toInt();
    evalStack.push_back(result);
}

// Выполнение переходов (jf - условный, j - безусловный)
void Interpreter::executeJump(const OPSElement& elem) {
    if (elem.value.operation == OP_JF) {
        Value label_val = evalStack.back();        evalStack.pop_back();
        Value condition = deref(evalStack.back()); evalStack.pop_back();
        if (!condition.data.bool_val)
            pc = label_val.toInt() - 1; // -1 т.к. в конце цикла будет pc++
    } else if (elem.value.operation == OP_J) {
        Value label_val = evalStack.back(); evalStack.pop_back();
        pc = label_val.toInt() - 1;
    }
}

// Выполнение операций ввода (r) и вывода (w)
void Interpreter::executeIO(OperationType op, const OPSElement&) {
    if (op == OP_READ) {
        Value var = evalStack.back(); evalStack.pop_back();

        if (var.type == Value::VAR_INT_REF) {
            int id = var.data.array_ref.array_id;
            int value;
            std::cin >> value;
            int_vars[id] = value;
        } else if (var.type == Value::VAR_REAL_REF) {
            int id = var.data.array_ref.array_id;
            double value;
            std::cin >> value;
            real_vars[id] = value;
        } else if (var.type == Value::ARRAY_REF) {
            int array_id = var.data.array_ref.array_id;
            int index    = var.data.array_ref.index;
            if (int_arrays.find(array_id) != int_arrays.end()) {
                int value; std::cin >> value;
                int_arrays[array_id][index] = value;
            } else if (real_arrays.find(array_id) != real_arrays.end()) {
                double value; std::cin >> value;
                real_arrays[array_id][index] = value;
            }
        }

    } else if (op == OP_WRITE) {
        Value val = deref(evalStack.back()); evalStack.pop_back();

        if (val.type == Value::INT) {
            std::cout << val.data.int_val << std::endl;
        } else if (val.type == Value::REAL) {
            std::cout << std::fixed << std::setprecision(6) << val.data.real_val << std::endl;
        } else if (val.type == Value::ARRAY_REF) {
            int array_id = val.data.array_ref.array_id;
            int index    = val.data.array_ref.index;
            if (int_arrays.find(array_id) != int_arrays.end())
                std::cout << int_arrays[array_id][index] << std::endl;
            else if (real_arrays.find(array_id) != real_arrays.end())
                std::cout << std::fixed << std::setprecision(6)
                          << real_arrays[array_id][index] << std::endl;
        }
    }
}

// Выполнение унарного минуса
void Interpreter::executeFunction(OperationType /*op*/, const OPSElement& /*elem*/) {
    Value arg      = deref(evalStack.back()); evalStack.pop_back();
    double arg_val = arg.toReal();
    double result = -arg_val;

    if (arg.type == Value::REAL)
        evalStack.push_back(Value::makeReal(result));
    else
        evalStack.push_back(Value::makeInt((int)result));
}

// Определение типа операции и вызов соответствующего обработчика
void Interpreter::executeOperation(const OPSElement& elem) {
    OperationType op = elem.value.operation;

    if (op == OP_ADD || op == OP_SUB || op == OP_MUL || op == OP_DIV)
        executeArithmetic(op, elem);
    else if (op == OP_LT || op == OP_GT || op == OP_LE ||
             op == OP_GE || op == OP_EQ || op == OP_NE)
        executeComparison(op, elem);
    else if (op == OP_ASSIGN)
        executeAssignment(elem);
    else if (op == OP_INDEX)
        executeIndexing(elem);
    else if (op == OP_JF || op == OP_J)
        executeJump(elem);
    else if (op == OP_READ || op == OP_WRITE)
        executeIO(op, elem);
    else if (op == OP_NEG)
        executeFunction(op, elem);
    else
        throw RuntimeError("Неизвестная операция", elem.source_line, elem.source_column);
}

static const char* opName(OperationType op) {
    static const char* names[] = {
        "+", "-", "*", "/", "-'", ":=", "i",
        "<", ">", "<=", ">=", "=", "<>",
        "jf", "j", "read", "write"
    };
    return names[op];
}

// Главный цикл интерпретатора - обход ОПС и выполнение каждого элемента
void Interpreter::run() {
    pc = 0;
    while (pc < ops.size()) {
        const OPSElement& elem = ops[pc];

        if (debug) {
            std::cout << "#" << pc << " (" << elem.source_line << "): ";
            switch (elem.type) {
                case OPSElement::OPERAND:
                    std::cout << "OP:";
                    switch (elem.value.operand.op_type) {
                        case VAR_INT:    std::cout << "VAR_INT[" << elem.value.operand.index << "]"; break;
                        case VAR_REAL:   std::cout << "VAR_REAL[" << elem.value.operand.index << "]"; break;
                        case CONST_INT:  std::cout << "CONST_INT[" << elem.value.operand.index << "]"; break;
                        case CONST_REAL: std::cout << "CONST_REAL[" << elem.value.operand.index << "]"; break;
                        case ARRAY_INT:  std::cout << "ARRAY_INT[" << elem.value.operand.index << "]"; break;
                        case ARRAY_REAL: std::cout << "ARRAY_REAL[" << elem.value.operand.index << "]"; break;
                    }
                    break;
                case OPSElement::OPERATION:
                    std::cout << "OP:" << opName(elem.value.operation);
                    break;
                case OPSElement::LABEL:
                    std::cout << "LABEL(" << elem.value.label << ")";
                    break;
            }
            std::cout << "\n";
            printStack();
        }

        switch (elem.type) {
            case OPSElement::OPERAND:
                evalStack.push_back(getOperandValue(elem));
                break;
            case OPSElement::OPERATION:
                executeOperation(elem);
                break;
            case OPSElement::LABEL:
                // Метка кладётся на стек как целое число (адрес перехода)
                evalStack.push_back(Value::makeInt(elem.value.label));
                break;
        }

        if (debug) printStack();

        pc++;
    }
}
