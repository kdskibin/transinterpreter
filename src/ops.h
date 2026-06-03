#ifndef OPS_H
#define OPS_H

#include <vector>
#include <string>
#include <iostream>
#include <map>

// Типы операндов
enum OperandType {
    VAR_INT,
    VAR_REAL,
    CONST_INT,
    CONST_REAL,
    ARRAY_INT,
    ARRAY_REAL
};

// Типы операций
enum OperationType {
    OP_ADD,      // +
    OP_SUB,      // -
    OP_MUL,      // *
    OP_DIV,      // /
    OP_NEG,      // унарный минус
    OP_ASSIGN,   // :=
    OP_INDEX,    // индексирование массива
    OP_LT,       // 
    OP_GT,       // >
    OP_LE,       // <=
    OP_GE,       // >=
    OP_EQ,       // =
    OP_NE,       // <>
    OP_JF,       // условный переход (jump if false)
    OP_J,        // безусловный переход
    OP_READ,     // read
    OP_WRITE     // write
};

// Элемент ОПС
struct OPSElement {
    enum Type {
        OPERAND,
        OPERATION,
        LABEL
    } type;
    
    // Данные
    union {
        struct {
            OperandType op_type;
            int index;  // индекс в таблице переменных/констант
        } operand;
        
        OperationType operation;
        int label;
    } value;
    
    // Для диагностики ошибок
    int source_line;
    int source_column;
    
    OPSElement();
};

// Класс ОПС
class OPS {
private:
    std::vector<OPSElement> elements;
    
public:
    void addOperand(OperandType type, int index, int line = 0, int col = 0);
    void addOperation(OperationType op, int line = 0, int col = 0);
    void addLabel(int label_num, int line = 0, int col = 0);

    size_t size() const { return elements.size(); }
    const OPSElement& operator[](size_t index) const { return elements[index]; }
    OPSElement& operator[](size_t index) { return elements[index]; }
    void patchLabel(int ops_position, int target_position) {  
        elements[ops_position].value.label = target_position;
    }
    void print() const;
    void clear() { elements.clear(); }
};

// Таблицы для хранения данных
class SymbolTable {
private:
    std::map<std::string, int> int_vars;
    std::map<std::string, int> real_vars;
    std::map<std::string, int> int_arrays;
    std::map<std::string, int> real_arrays;
    std::vector<int> int_constants;
    std::vector<double> real_constants;
    
    int next_int_var_id;
    int next_real_var_id;
    int next_int_array_id;
    int next_real_array_id;
    
public:
    SymbolTable();
    
    // Добавление переменных
    int addIntVar(const std::string& name);
    int addRealVar(const std::string& name);
    int addIntArray(const std::string& name, int size);
    int addRealArray(const std::string& name, int size);
    
    // Поиск переменных
    bool findIntVar(const std::string& name, int& index);
    bool findRealVar(const std::string& name, int& index);
    bool findIntArray(const std::string& name, int& index);
    bool findRealArray(const std::string& name, int& index);
    
    // Работа с константами
    int addIntConst(int value);
    int addRealConst(double value);
    int getIntConst(int index) const;
    double getRealConst(int index) const;
    
    void print() const;
};

#endif // OPS_H