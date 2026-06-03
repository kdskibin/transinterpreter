#include <iostream>
#include <fstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

void printUsage(const char* program_name) {
    std::cout << "Использование: " << program_name << " <файл_программы> [опции]\n";
    std::cout << "Опции:\n";
    std::cout << "  --debug    Вывод отладочной информации\n";
    std::cout << "  --tokens   Показать список токенов\n";
    std::cout << "  --ops      Показать ОПС\n";
    std::cout << "  --symbols  Показать таблицу символов\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string filename = argv[1];
    bool debug = false;
    bool show_tokens = false;
    bool show_ops = false;
    bool show_symbols = false;
    
    // Обработка опций
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            debug = true;
            show_tokens = true;
            show_ops = true;
            show_symbols = true;
        } else if (arg == "--tokens") {
            show_tokens = true;
        } else if (arg == "--ops") {
            show_ops = true;
        } else if (arg == "--symbols") {
            show_symbols = true;
        }
    }
    
    try {
        //  ШАГ 1: Лексический анализ 
        if (debug) std::cout << "\n=== ЛЕКСИЧЕСКИЙ АНАЛИЗ ===\n";
        
        Lexer lexer(filename);
        std::vector<Token> tokens = lexer.tokenize();
        
        if (debug) {
            std::cout << "Найдено токенов: " << tokens.size() << "\n";
        }
        
        if (show_tokens) {
            std::cout << "\n=== СПИСОК ТОКЕНОВ ===\n";
            for (size_t i = 0; i < tokens.size(); i++) {
                const Token& tok = tokens[i];
                std::cout << i << ": " << tok.value 
                         << " (тип=" << tok.type 
                         << ", строка=" << tok.line 
                         << ", столбец=" << tok.column << ")\n";
            }
        }
        
        //  ШАГ 2: Синтаксический анализ 
        if (debug) std::cout << "\n=== СИНТАКСИЧЕСКИЙ АНАЛИЗ ===\n";
        
        Parser parser(tokens);
        OPS ops = parser.parse();
        
        if (debug) {
            std::cout << "Сгенерировано элементов ОПС: " << ops.size() << "\n";
        }
        
        if (show_ops) {
            std::cout << "\n";
            ops.print();
        }
        
        if (show_symbols) {
            parser.getSymbolTable().print();
        }
        
        // ===== ШАГ 3: Интерпретация =====
        if (debug) std::cout << "\n=== ВЫПОЛНЕНИЕ ПРОГРАММЫ ===\n\n";
        
        Interpreter interpreter(ops, parser.getSymbolTable());
        interpreter.run();
        
        if (debug) std::cout << "\n=== ПРОГРАММА ЗАВЕРШЕНА УСПЕШНО ===\n";
        
    } catch (const LexerError& e) {
        std::cerr << "\nЛЕКСИЧЕСКАЯ ОШИБКА:\n";
        std::cerr << "   Строка " << e.line << ", позиция " << e.column << "\n";
        std::cerr << "   " << e.what() << "\n\n";
        return 1;
        
    } catch (const ParserError& e) {
        std::cerr << "\nСИНТАКСИЧЕСКАЯ ОШИБКА:\n";
        std::cerr << "   Строка " << e.line << ", позиция " << e.column << "\n";
        std::cerr << "   " << e.what() << "\n\n";
        return 1;
        
    } catch (const RuntimeError& e) {
        std::cerr << "\nОШИБКА ВЫПОЛНЕНИЯ:\n";
        std::cerr << "   Строка " << e.line << ", позиция " << e.column << "\n";
        std::cerr << "   " << e.what() << "\n\n";
        return 1;
        
    } catch (const std::exception& e) {
        std::cerr << "\nНЕИЗВЕСТНАЯ ОШИБКА:\n";
        std::cerr << "   " << e.what() << "\n\n";
        return 1;
    }
    
    return 0;
}