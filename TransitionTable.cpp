#include "TransitionTable.h"
#include "LexicalAnalyzer.h"


const std::unordered_map<std::string, TransitionTable::Action>& TransitionTable::GetTable() {
    static const std::unordered_map<std::string, Action> table = {
        {"<ц>", LexicalAnalyzer::NUM_Analyse},
        {"<б>", LexicalAnalyzer::ID_Analyse},
        {"< >", []() { LexicalAnalyzer::SkipWhitespace(); }},
        {"<\">", LexicalAnalyzer::STR_Analyse},
        {"<;>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Semicolon); }},
        {"<+>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Plus); }},
        {"<->", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Minus); }},
        {"<*>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Multiply); }},
        {"</>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Divide); }},
        {"<%>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Modulus); }},
        {"<<>", LexicalAnalyzer::LESS_Analyse},
        {"<>>", LexicalAnalyzer::MORE_Analyse},
        {"<=>", LexicalAnalyzer::EQUAL_Analyse},
        {"<&>", LexicalAnalyzer::AND_Analyse},
        {"<|>", LexicalAnalyzer::OR_Analyse},
        {"<!>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::Not); }},
        {"<(>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::LeftParen); }},
        {"<)>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::RightParen); }},
        {"<[>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::LeftBracket); }},
        {"<]>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::RightBracket); }},
        {"<{>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::LeftBrace); }},
        {"<}>", []() { LexicalAnalyzer::ProcessSimpleToken(ETerminalType::RightBrace); }}
    };
    return table;
}

bool TransitionTable::TryGetAction(const std::string& charGroup, Action& action) {
    const auto& table = GetTable();
    auto it = table.find(charGroup);
    if (it != table.end()) {
        action = it->second;
        return true;
    }
    return false;
}