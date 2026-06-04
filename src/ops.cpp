#include "ops.h"
#include <iostream>
#include <iomanip>

//  OPSElement 

OPSElement::OPSElement() : type(OPERAND), source_line(0), source_column(0) {
    value.operand.op_type = VAR_INT;
    value.operand.index = 0;
}

//  OPS 

void OPS::addOperand(OperandType type, int index, int line, int col) {
    OPSElement elem;
    elem.type = OPSElement::OPERAND;
    elem.value.operand.op_type = type;
    elem.value.operand.index = index;
    elem.source_line = line;
    elem.source_column = col;
    elements.push_back(elem);
}

void OPS::addOperation(OperationType op, int line, int col) {
    OPSElement elem;
    elem.type = OPSElement::OPERATION;
    elem.value.operation = op;
    elem.source_line = line;
    elem.source_column = col;
    elements.push_back(elem);
}

void OPS::addLabel(int label_num, int line, int col) {
    OPSElement elem;
    elem.type = OPSElement::LABEL;
    elem.value.label = label_num;
    elem.source_line = line;
    elem.source_column = col;
    elements.push_back(elem);
}

void OPS::print() const {
    std::cout << "=== ОПС (Обратная Польская Строка) ===\n";
    
    const char* op_names[] = {
        "+", "-", "*", "/", "-'", ":=", "i",
        "<", ">", "<=", ">=", "=", "<>",
        "jf", "j", "read", "write"
    };
    
    for (size_t i = 0; i < elements.size(); i++) {
        std::cout << std::setw(4) << i << ": ";
        
        const OPSElement& elem = elements[i];
        
        switch (elem.type) {
            case OPSElement::OPERAND:
                std::cout << "OPERAND ";
                switch (elem.value.operand.op_type) {
                    case VAR_INT: std::cout << "VAR_INT[" << elem.value.operand.index << "]"; break;
                    case VAR_REAL: std::cout << "VAR_REAL[" << elem.value.operand.index << "]"; break;
                    case CONST_INT: std::cout << "CONST_INT[" << elem.value.operand.index << "]"; break;
                    case CONST_REAL: std::cout << "CONST_REAL[" << elem.value.operand.index << "]"; break;
                    case ARRAY_INT: std::cout << "ARRAY_INT[" << elem.value.operand.index << "]"; break;
                    case ARRAY_REAL: std::cout << "ARRAY_REAL[" << elem.value.operand.index << "]"; break;
                }
                break;
                
            case OPSElement::OPERATION:
                std::cout << "OPERATION " << op_names[elem.value.operation];
                break;
                
            case OPSElement::LABEL:
                std::cout << "LABEL " << elem.value.label;
                break;
        }
        
        std::cout << " (line " << elem.source_line << ")\n";
    }
    std::cout << "=====================================\n";
}

//  SymbolTable 

SymbolTable::SymbolTable() 
    : next_int_var_id(0), next_real_var_id(0), 
      next_int_array_id(0), next_real_array_id(0) {}

int SymbolTable::addIntVar(const std::string& name) {
    if (int_vars.find(name) != int_vars.end()) {
        return int_vars[name];
    }
    int id = next_int_var_id++;
    int_vars[name] = id;
    return id;
}

int SymbolTable::addRealVar(const std::string& name) {
    if (real_vars.find(name) != real_vars.end()) {
        return real_vars[name];
    }
    int id = next_real_var_id++;
    real_vars[name] = id;
    return id;
}

int SymbolTable::addIntArray(const std::string& name, int size) {
    if (int_arrays.find(name) != int_arrays.end()) {
        return int_arrays[name];
    }
    int id = next_int_array_id++;
    int_arrays[name] = id;
    return id;
}

int SymbolTable::addRealArray(const std::string& name, int size) {
    if (real_arrays.find(name) != real_arrays.end()) {
        return real_arrays[name];
    }
    int id = next_real_array_id++;
    real_arrays[name] = id;
    return id;
}

bool SymbolTable::findIntVar(const std::string& name, int& index) {
    auto it = int_vars.find(name);
    if (it != int_vars.end()) {
        index = it->second;
        return true;
    }
    return false;
}

bool SymbolTable::findRealVar(const std::string& name, int& index) {
    auto it = real_vars.find(name);
    if (it != real_vars.end()) {
        index = it->second;
        return true;
    }
    return false;
}

bool SymbolTable::findIntArray(const std::string& name, int& index) {
    auto it = int_arrays.find(name);
    if (it != int_arrays.end()) {
        index = it->second;
        return true;
    }
    return false;
}

bool SymbolTable::findRealArray(const std::string& name, int& index) {
    auto it = real_arrays.find(name);
    if (it != real_arrays.end()) {
        index = it->second;
        return true;
    }
    return false;
}

int SymbolTable::addIntConst(int value) {
    // Проверяем, есть ли уже такая константа
    for (size_t i = 0; i < int_constants.size(); i++) {
        if (int_constants[i] == value) {
            return i;
        }
    }
    int_constants.push_back(value);
    return int_constants.size() - 1;
}

int SymbolTable::addRealConst(double value) {
    for (size_t i = 0; i < real_constants.size(); i++) {
        if (real_constants[i] == value) {
            return i;
        }
    }
    real_constants.push_back(value);
    return real_constants.size() - 1;
}

int SymbolTable::getIntConst(int index) const {
    return int_constants[index];
}

double SymbolTable::getRealConst(int index) const {
    return real_constants[index];
}

void SymbolTable::print() const {
    std::cout << "\n=== Таблица символов ===\n";
    
    std::cout << "Int переменные:\n";
    for (const auto& p : int_vars) {
        std::cout << "  " << p.first << " -> " << p.second << "\n";
    }
    
    std::cout << "Real переменные:\n";
    for (const auto& p : real_vars) {
        std::cout << "  " << p.first << " -> " << p.second << "\n";
    }
    
    std::cout << "Int массивы:\n";
    for (const auto& p : int_arrays) {
        std::cout << "  " << p.first << " -> " << p.second << "\n";
    }
    
    std::cout << "Int константы:\n";
    for (size_t i = 0; i < int_constants.size(); i++) {
        std::cout << "  [" << i << "] = " << int_constants[i] << "\n";
    }
    
    std::cout << "Real константы:\n";
    for (size_t i = 0; i < real_constants.size(); i++) {
        std::cout << "  [" << i << "] = " << real_constants[i] << "\n";
    }
    
    std::cout << "========================\n";
    
}