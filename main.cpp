#include <iostream>
#include <string>
#include "FileReader.h"        // Подключаем ваш чтец файлов
#include "LexicalAnalyzer.h"   // Лексический анализатор
#include "LL1Parser.h"         // LL(1)-парсер и генератор ОПС
#include "Interpreter.h"       // Интерпретатор ОПС

int main(int argc, char* argv[]) {

    std::setlocale(LC_ALL, ".UTF8"); 

    std::string filePath;

    // 1. Проверяем, передан ли путь к файлу через аргументы командной строки
    if (argc > 1) {
        filePath = argv[1]; // Берем первый аргумент
    } else {
        // Если аргумент не передан, запрашиваем ввод у пользователя
        std::cout << "Введите путь к файлу с кодом: ";
        std::getline(std::cin, filePath);
    }

    try {
        // 2. Читаем исходный код из файла с помощью FileReader
        std::cout << "[1/4] Чтение файла: " << filePath << "...\n";
        std::string sourceCode = FileReader::read(filePath); 
        // Примечание: проверьте, как точно называется метод чтения в вашем FileReader.
        // Обычно это что-то вроде FileReader::read(filePath) или readFile(filePath).

        // 3. Запускаем лексический анализ (опционально, если LL1Parser не делает это внутри)
        std::cout << "[2/4] Лексический анализ...\n";
        if (!LexicalAnalyzer::isLexicalCorrect(sourceCode)) {
            std::cerr << "Ошибка: Лексический анализ не пройден.\n";
            return 1;
        }

        // 4. Запускаем синтаксический анализ и генерацию ОПС (LL1-парсер)
        std::cout << "[3/4] Синтаксический анализ и генерация ОПС...\n";
        LL1Parser parser;
        parser.parse(sourceCode);
        
        // Выведем полученную ленту ОПС для наглядности
        std::cout << "\n--- Сгенерированная лента ОПС ---\n";
        parser.printRpn();
        std::cout << "---------------------------------\n\n";

        // 5. Передаем готовую ОПС в интерпретатор на выполнение
        std::cout << "[4/4] Запуск интерпретатора...\n";
        Interpreter::execute(parser.getRpn());
        
        std::cout << "\nПрограмма успешно завершила работу!\n";

    } catch (const std::exception& ex) {
        // Ловим любые ошибки (файл не найден, синтаксическая ошибка, деление на ноль в коде и т.д.)
        std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА]: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}