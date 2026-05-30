#include <iostream>
#include <string>
#include <filesystem>
#include "FileReader.h"
#include "LexicalAnalyzer.h"
#include "SyntacticalAnalyzer.h"
#include "RPNTranslator.h"
#include "RPNInterpreter.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::string testFileName = "test_formulas.txt";
    if (argc > 1) {
        testFileName = argv[1];
    }

    // Определяем путь: текущая рабочая директория
    std::string filePath = testFileName;

    try {
        // Чтение исходного кода из файла
        std::string code = FileReader::read(filePath);

        // Лексический анализ
        LexicalAnalyzer::isLexicalCorrect(code);
        auto terminals = LexicalAnalyzer::getTerminals();

        // Синтаксический анализ
        if (!SyntacticalAnalyzer::isSyntacticalCorrect(terminals)) {
            std::cerr << "Синтаксическая ошибка в коде." << std::endl;
            return 1;
        }

        // Трансляция в RPN
        auto rpn = RPNTranslator::convertToRPN(terminals);

        // Интерпретация и выполнение RPN
        RPNInterpreter::executeInstructions(rpn);

    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
