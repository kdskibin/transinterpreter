#pragma once

// Типы терминалов (токенов), распознаваемых лексическим анализатором
enum class ETerminalType {
    // Литералы
    Number, TextLine, Boolean,
    // Арифметические операторы
    Plus, Minus, Multiply, Divide, Modulus,
    // Логические операторы
    And, Or, Not,
    // Скобки
    LeftParen, RightParen, LeftBracket, RightBracket, LeftBrace, RightBrace,
    // Кавычки
    DoubleQuote,
    // Присваивание и сравнение
    Assignment, Equal, Less, Greater, LessEqual, GreaterEqual,
    // Идентификаторы и ключевые слова
    VariableName, If, Else, While,
    // Типы данных
    Int, String, Bool,
    // Встроенные функции
    Output, Input, Sqrt, Pow,
    // Разделитель
    Semicolon
};

// Типы символов обратной польской нотации (RPN)
// F_ — операции, A_ — аргументы, T_ — служебные токены, M_ — метки
enum class ERPNType {
    // Операции ввода/вывода
    F_Output, F_Input,
    // Присваивание
    F_Assignment,
    // Логические операции
    F_And, F_Or,
    // Сравнения
    F_Equal, F_Less, F_Greater, F_LessEqual, F_GreaterEqual,
    // Арифметические операции
    F_Plus, F_Minus, F_Multiply, F_Divide, F_Modulus,
    // Унарные и функции
    F_Not, F_Sqrt, F_Pow,
    // Индексация массива
    F_Index,
    // Объявление переменных
    F_Int, F_String, F_Bool,
    // Объявление массивов
    F_IntArray, F_StringArray, F_BoolArray,
    // Аргументы (операнды)
    A_Number, A_TextLine, A_Boolean, A_VariableName,
    // Служебные токены
    T_If, T_Else, T_While, T_Semicolon,
    T_LeftParen, T_RightParen,
    T_LeftBracket, T_RightBracket,
    T_LeftBrace, T_RightBrace,
    // Управление потоком
    F_ConditionalJumpToMark,
    F_UnconditionalJumpToMark,
    // Метка перехода
    M_Mark
};

// Конкретные типы меток для управления потоком
enum class EMarkType {
    WhileBeginMark, // Начало тела цикла while
    WhileEndMark,   // Конец тела цикла while
    IfMark,         // Переход при ложном условии if
    ElseMark        // Безусловный переход в конце блока if (минуя else)
};