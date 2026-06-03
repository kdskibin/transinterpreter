#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ops.h"
#include <map>
#include <vector>
#include <exception>

// Значение в процессе вычислений
struct Value {
    enum Type {
        INT,
        REAL,
        BOOL,
        ARRAY_REF,  // Ссылка на элемент массива
        VAR_INT_REF,
        VAR_REAL_REF
    } type;
    
    union {
        int int_val;
        double real_val;
        bool bool_val;
        struct {
            int array_id;
            int index;
        } array_ref;
    } data;
    
    Value() : type(INT) { data.int_val = 0; }
    
    static Value makeInt(int v) {
        Value val;
        val.type = INT;
        val.data.int_val = v;
        return val;
    }
    
    static Value makeReal(double v) {
        Value val;
        val.type = REAL;
        val.data.real_val = v;
        return val;
    }
    
    static Value makeBool(bool v) {
        Value val;
        val.type = BOOL;
        val.data.bool_val = v;
        return val;
    }
    
    double toReal() const {
        if (type == INT) return (double)data.int_val;
        return data.real_val;
    }
    
    int toInt() const {
        if (type == REAL) return (int)data.real_val;
        return data.int_val;
    }

    const char* typeName() const {
        switch (type) {
            case INT:    return "INT";
            case REAL:   return "REAL";
            case BOOL:   return "BOOL";
            case ARRAY_REF: return "ARRAY_REF";
            case VAR_INT_REF: return "VAR_INT_REF";
            case VAR_REAL_REF: return "VAR_REAL_REF";
            default: return "???";
        }
    }
};

// Исключение для ошибок выполнения
class RuntimeError : public std::exception {
private:
    std::string message;
    
public:
    int line;
    int column;
    
    RuntimeError(const std::string& msg, int l, int c)
        : message(msg), line(l), column(c) {}
    
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// Интерпретатор
class Interpreter {
private:
    const OPS& ops;
    const SymbolTable& symbols;
    bool debug;

    std::vector<Value> evalStack;  // Магазин вычислений (vector как стек)
    void printStack() const;
    
    // Хранилища переменных
    std::map<int, int> int_vars;
    std::map<int, double> real_vars;
    std::map<int, std::vector<int>> int_arrays;
    std::map<int, std::vector<double>> real_arrays;
    
    size_t pc;  // Program Counter - номер текущей команды
    
    // Вспомогательные функции
    Value getOperandValue(const OPSElement& elem);
    void setVariableValue(const Value& var_ref, const Value& value, int line, int col);
    Value deref(const Value& v); 
    
    void executeOperation(const OPSElement& elem);
    void executeArithmetic(OperationType op, const OPSElement& elem);
    void executeComparison(OperationType op, const OPSElement& elem);
    void executeAssignment(const OPSElement& elem);
    void executeIndexing(const OPSElement& elem);
    void executeJump(const OPSElement& elem);
    void executeIO(OperationType op, const OPSElement& elem);
    void executeFunction(OperationType op, const OPSElement& elem);
    
public:
    Interpreter(const OPS& ops, const SymbolTable& symbols);
    Interpreter(const OPS& ops, const SymbolTable& symbols, bool debug);
    
    void run();
};

#endif // INTERPRETER_H