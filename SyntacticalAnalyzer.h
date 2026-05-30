#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Terminal.h"

// Синтаксический анализатор: проверяет соответствие последовательности токенов грамматике языка.
// Использует метод рекурсивного спуска с подпоследовательностями.
class SyntacticalAnalyzer {
public:
    // Запускает синтаксический анализ.
    // Записывает лог разбора в файл "SAlog.txt".
    // Возвращает true при успехе, иначе false.
    static bool isSyntacticalCorrect(const std::vector<std::shared_ptr<Terminal>>& terminals);

private:
    using TermVec = std::vector<std::shared_ptr<Terminal>>;

    static std::string _log;
    static int _logCounter;
    static std::string _tabs;
    static int _tabsCounter;

    // Управление отступами и логом
    static void incTabs();
    static void decTabs();
    static void appendLog(const std::string& msg);

    // Поиск парной закрывающей скобки
    static int findPairedClosingBracket(int leftIdx, const TermVec& t);

    // Вспомогательный доступ к токену с проверкой границ
    static ETerminalType typeAt(int idx, const TermVec& t);
    static bool typeIs(int idx, const TermVec& t, ETerminalType expected);

    // Поиск первого вхождения типа токена
    static int findFirst(const TermVec& t, ETerminalType type);

    // Создание подвектора (аналог срезов C#)
    static TermVec slice(const TermVec& t, int from, int to);
    static TermVec sliceFrom(const TermVec& t, int from);

    // ── Правила грамматики ──────────────────────────────────────
    // 1.  Блок инструкций
    static bool parseInstructionBlock(const TermVec& t);
    // 2.  Последующая инструкция (ε | <Блок инструкций>)
    static bool parseFollowingInstruction(const TermVec& t);
    // 3.  Инициализация переменной
    static bool parseVariableInitialization(const TermVec& t);
    // 4.  Присваивание
    static bool parseAssignment(const TermVec& t);
    // 5.  Аргумент присваивания
    static bool parseAssignmentArgument(const TermVec& t);
    // 6.  Логическое ИЛИ
    static bool parseLogicalOR(const TermVec& t);
    // 7.  Логическое И
    static bool parseLogicalAND(const TermVec& t);
    // 8.  Аргумент логического И
    static bool parseLogicalANDArgument(const TermVec& t);
    // 9.  Отрицание
    static bool parseNegation(const TermVec& t);
    // 10. Аргумент отрицания
    static bool parseNegationArgument(const TermVec& t);
    // 11. Строковое сравнение
    static bool parseStringComparison(const TermVec& t);
    // 12. Конкатенация
    static bool parseConcatenation(const TermVec& t);
    // 13. Аргумент конкатенации
    static bool parseConcatenationArgument(const TermVec& t);
    // 14. Числовое сравнение
    static bool parseNumericalComparison(const TermVec& t);
    // 16. Сложение и вычитание
    static bool parseAdditionAndSubtraction(const TermVec& t);
    // 17. Умножение и деление
    static bool parseMultiplicationAndDivision(const TermVec& t);
    // 18. Унарный минус
    static bool parseUnaryMinus(const TermVec& t);
    // 19. Аргумент унарного минуса
    static bool parseUnaryMinusArgument(const TermVec& t);
    // 20. Идентификатор (имя переменной или имя[индекс])
    static bool parseIdentifier(const TermVec& t);
    // 21. Индексатор
    static bool parseIndexer(const TermVec& t);
};
