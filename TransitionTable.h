#pragma once
#include <string>
#include <functional>
#include <unordered_map>


class LexicalAnalyzer;

class TransitionTable {
public:
    using Action = std::function<void()>;
    static bool TryGetAction(const std::string& charGroup, Action& action);

private:
    static const std::unordered_map<std::string, Action>& GetTable();
};