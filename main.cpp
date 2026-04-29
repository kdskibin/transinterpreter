#include "LexicalAnalyzer.h"
#include <iostream>
#include <variant>

int main() {
    std::string test = "a = 10 + 4";

    try {
        bool is_correct = LexicalAnalyzer::IsLexicalCorrect(test);
        std::cout << "is_correct: " << std::boolalpha << is_correct << std::endl;

        auto terminals = LexicalAnalyzer::GetTerminals();
        std::cout << "Total terminals: " << terminals.size() << std::endl;

        for (const auto& terminal : terminals) {
            std::cout << "Terminal type: " << static_cast<int>(terminal.TerminalType)
                      << " at line " << terminal.LinePointer
                      << ", char " << terminal.CharPointer;

            // Посещаем variant с помощью std::visit
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    std::cout << " (no value)" << std::endl;
                } else if constexpr (std::is_same_v<T, int>) {
                    std::cout << " (integer: " << arg << ")" << std::endl;
                } else if constexpr (std::is_same_v<T, bool>) {
                    std::cout << " (boolean: " << std::boolalpha << arg << ")" << std::endl;
                } else if constexpr (std::is_same_v<T, std::string>) {
                    std::cout << " (string: \"" << arg << "\")" << std::endl;
                }
            }, terminal.Value);
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}