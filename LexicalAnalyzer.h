#pragma once
#include "Enums.h"
#include "Terminal.h"
#include <string>
#include <vector>


class LexicalAnalyzer {
public:
    static bool IsLexicalCorrect(const std::string& data);
    static std::vector<Terminal> GetTerminals();

    // Методы, вызываемые из TransitionTable
    static void SkipWhitespace();
    static void ProcessSimpleToken(ETerminalType terminalType);
    static void NUM_Analyse();
    static void ID_Analyse();
    static void STR_Analyse();
    static void LESS_Analyse();
    static void MORE_Analyse();
    static void EQUAL_Analyse();
    static void AND_Analyse();
    static void OR_Analyse();

private:
    static std::string Data;
    static size_t _pointer;
    static int _charPointer;
    static int _linePointer;
    static int _char;
    static std::vector<Terminal> Terminals;

    static char CurrentChar();
    static void Advance();
    static std::string CurrentCharGroup();
    static void Start_Analyse();
    static void ReadTerminal(ETerminalType type);
    static void ReadTerminal(ETerminalType type, const std::string& value);
};