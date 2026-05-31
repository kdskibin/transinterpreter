#pragma once
#include <string>
#include <vector>
#include <memory>
#include "Terminal.h"

// Лексический анализатор: разбивает исходный код на токены (терминалы)
class LexicalAnalyzer {
public:
    // Выполняет лексический анализ строки кода.
    // Возвращает true при успехе, иначе бросает исключение.
    static bool isLexicalCorrect(const std::string& data);

    // Возвращает список токенов после анализа
    static std::vector<std::shared_ptr<Terminal>> getTerminals();

    // Методы, вызываемые из таблицы переходов
    static void skipWhitespace();
    static void processSimpleToken(ETerminalType type);
    static void NUM_Analyse();
    static void ID_Analyse();
    static void STR_Analyse();
    static void LESS_Analyse();
    static void MORE_Analyse();
    static void EQUAL_Analyse();
    static void AND_Analyse();
    static void OR_Analyse();
    static void NOT_Analyse(); // Анализ оператора '!' или '!='

private:
    static std::string _data;
    static std::vector<std::shared_ptr<Terminal>> _terminals;
    static int _charPointer; // Позиция в текущей строке
    static int _linePointer; // Номер текущей строки
    static int _char;        // Сохранённая позиция начала текущего токена
    static int _pointer;     // Абсолютная позиция в строке данных

    // Текущий символ
    static char currentChar();
    // Продвинуть указатель вперёд, обновив счётчики строк
    static void advancePointer();
    // Определить группу текущего символа для таблицы переходов
    static std::string currentCharGroup();
    // Запустить анализ одного токена
    static void startAnalyse();
    // Добавить терминал без значения
    static void readTerminal(ETerminalType type);
    // Добавить терминал со значением (число, строка, булево, идентификатор)
    static void readTerminalWithValue(ETerminalType type, const std::string& value);

};
