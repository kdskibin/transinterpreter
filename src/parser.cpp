#include "parser.h"
#include <stdexcept>
#include <string>

// ============================================================
// Вспомогательные функции для построения элементов магазина
// ============================================================

static StackSym T(int t) { return {StackSym::TERM,    t}; }
static StackSym N(int n) { return {StackSym::NONTERM, n}; }
static StackSym A(int a) { return {StackSym::ACTION,  a}; }

// ============================================================
// Вспомогательные наборы токенов для λ-правил
// ============================================================

// FOLLOW(H): всё кроме [
static const std::vector<int> FOLLOW_H = {
    TOKEN_ASSIGN, TOKEN_RPAREN, TOKEN_RBRACKET,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV,
    TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_EQ, TOKEN_NE,
    TOKEN_SEMICOLON, TOKEN_END, TOKEN_THEN, TOKEN_DO, TOKEN_ELSE, TOKEN_EOF
};

// FOLLOW(U): токены, завершающие хвост выражения
static const std::vector<int> FOLLOW_U = {
    TOKEN_RPAREN, TOKEN_RBRACKET,
    TOKEN_SEMICOLON, TOKEN_END, TOKEN_THEN, TOKEN_DO, TOKEN_ELSE, TOKEN_EOF,
    TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_EQ, TOKEN_NE
};

// FOLLOW(VM): токены, завершающие хвост терма
static const std::vector<int> FOLLOW_VM = {
    TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_RPAREN, TOKEN_RBRACKET,
    TOKEN_SEMICOLON, TOKEN_END, TOKEN_THEN, TOKEN_DO, TOKEN_ELSE, TOKEN_EOF,
    TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_EQ, TOKEN_NE
};

// FOLLOW(E): токены, при которых ветки else нет
static const std::vector<int> FOLLOW_E = {
    TOKEN_SEMICOLON, TOKEN_END, TOKEN_EOF
};

// Все токены (для NT_Z → λ)
static const std::vector<int> ALL_TOKENS = {
    TOKEN_ID, TOKEN_INT_CONST, TOKEN_REAL_CONST,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV,
    TOKEN_LPAREN, TOKEN_RPAREN, TOKEN_LBRACKET, TOKEN_RBRACKET,
    TOKEN_ASSIGN, TOKEN_LT, TOKEN_GT, TOKEN_LE, TOKEN_GE, TOKEN_EQ, TOKEN_NE,
    TOKEN_COMMA, TOKEN_SEMICOLON,
    TOKEN_IF, TOKEN_THEN, TOKEN_ELSE, TOKEN_WHILE, TOKEN_DO,
    TOKEN_BEGIN, TOKEN_END, TOKEN_READ, TOKEN_WRITE,
    TOKEN_INT, TOKEN_REAL, TOKEN_SQRT, TOKEN_ABS, TOKEN_EOF
};

// Начальные токены выражения (FIRST(S) = FIRST(T) = FIRST(F))
static const std::vector<int> FIRST_S = {
    TOKEN_LPAREN, TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_ID, TOKEN_INT_CONST, TOKEN_REAL_CONST,
    TOKEN_SQRT, TOKEN_ABS
};

// ============================================================
// Инициализация таблицы LL(1) разбора
//
// Каждое правило хранится как вектор элементов магазина
// в порядке ВЫПОЛНЕНИЯ (левый → правый).
// При добавлении в магазин правило разворачивается, чтобы
// первый элемент оказался на вершине стека.
//
// Семантические действия встроены по следующему принципу:
//   - для терминала αᵢ с действием aᵢ: [..., T(αᵢ), A(aᵢ), ...]
//     → действие срабатывает ПОСЛЕ сопоставления терминала
//   - для нетерминала αᵢ с действием aᵢ: [..., A(aᵢ), N(αᵢ), ...]
//     → действие срабатывает ДО раскрытия нетерминала (т.е. после предыдущего)
// ============================================================

void Parser::addRule(int nt, const std::vector<int>& toks, const Rule& rule) {
    for (int tok : toks)
        parseTable[{nt, tok}] = rule;
}

void Parser::initTable() {

    // ------ NT_P: программа ------
    // P → int V ; P      семантика: SET_INT перед V
    parseTable[{NT_P, TOKEN_INT}] = {
        T(TOKEN_INT), A(ACT_SET_INT), N(NT_V), T(TOKEN_SEMICOLON), N(NT_P)
    };
    // P → real V ; P
    parseTable[{NT_P, TOKEN_REAL}] = {
        T(TOKEN_REAL), A(ACT_SET_REAL), N(NT_V), T(TOKEN_SEMICOLON), N(NT_P)
    };
    // P → begin A Q end
    parseTable[{NT_P, TOKEN_BEGIN}] = {
        T(TOKEN_BEGIN), N(NT_A), N(NT_Q), T(TOKEN_END)
    };

    // ------ NT_V: объявление переменной ------
    // V → id W     (сохраняем имя, W уточнит — скаляр или массив)
    parseTable[{NT_V, TOKEN_ID}] = {
        T(TOKEN_ID), A(ACT_SAVE_NAME), N(NT_W)
    };

    // ------ NT_W: хвост объявления ------
    // W → , id W   (сначала объявляем предыдущее имя как скаляр, затем берём следующее)
    parseTable[{NT_W, TOKEN_COMMA}] = {
        A(ACT_DECL_SCALAR), T(TOKEN_COMMA), T(TOKEN_ID), A(ACT_SAVE_NAME), N(NT_W)
    };
    // W → [ int_const ]  (объявляем массив, размер = lastToken после INT_CONST)
    parseTable[{NT_W, TOKEN_LBRACKET}] = {
        T(TOKEN_LBRACKET), T(TOKEN_INT_CONST), A(ACT_DECL_ARRAY), T(TOKEN_RBRACKET)
    };
    // W → λ   (объявляем накопленное имя как скаляр)
    parseTable[{NT_W, TOKEN_SEMICOLON}] = { A(ACT_DECL_SCALAR) };

    // ------ NT_Q: хвост блока (список операторов через ;) ------
    // Q → ; A Q
    parseTable[{NT_Q, TOKEN_SEMICOLON}] = {
        T(TOKEN_SEMICOLON), N(NT_A), N(NT_Q)
    };
    // Q → λ
    parseTable[{NT_Q, TOKEN_END}] = {};
    parseTable[{NT_Q, TOKEN_EOF}] = {};

    // ------ NT_A: оператор ------

    // A → id H := S Z   семантика: a □ □ □ :=
    // Раскрытие: id↓a, H, :=, S, Z(:=)
    parseTable[{NT_A, TOKEN_ID}] = {
        T(TOKEN_ID), A(ACT_OPERAND), N(NT_H),
        T(TOKEN_ASSIGN), N(NT_S), A(ACT_ASSIGN), N(NT_Z)
    };

    // A → if C then A E Z   семантика: □ □ 1 □ □ 3
    // PROG1 срабатывает после 'then', PROG3 — перед NT_Z (после E)
    parseTable[{NT_A, TOKEN_IF}] = {
        T(TOKEN_IF), N(NT_C), T(TOKEN_THEN), A(ACT_PROG1),
        N(NT_A), N(NT_E), A(ACT_PROG3), N(NT_Z)
    };

    // A → while C do A Z   семантика: 4 □ 1 □ 5
    // PROG4 после 'while', PROG1 после 'do', PROG5 перед NT_Z (после тела)
    parseTable[{NT_A, TOKEN_WHILE}] = {
        T(TOKEN_WHILE), A(ACT_PROG4), N(NT_C),
        T(TOKEN_DO), A(ACT_PROG1), N(NT_A),
        A(ACT_PROG5), N(NT_Z)
    };

    // A → read ( id H )   семантика: □ □ a □ r
    parseTable[{NT_A, TOKEN_READ}] = {
        T(TOKEN_READ), T(TOKEN_LPAREN),
        T(TOKEN_ID), A(ACT_OPERAND), N(NT_H),
        T(TOKEN_RPAREN), A(ACT_READ)
    };

    // A → write ( S )   семантика: □ □ □ w
    parseTable[{NT_A, TOKEN_WRITE}] = {
        T(TOKEN_WRITE), T(TOKEN_LPAREN), N(NT_S),
        T(TOKEN_RPAREN), A(ACT_WRITE)
    };

    // A → begin A Q end
    parseTable[{NT_A, TOKEN_BEGIN}] = {
        T(TOKEN_BEGIN), N(NT_A), N(NT_Q), T(TOKEN_END)
    };

    // ------ NT_H: индексация массива ------
    // H → [ S ]   семантика: □ □ □ i
    parseTable[{NT_H, TOKEN_LBRACKET}] = {
        T(TOKEN_LBRACKET), N(NT_S), T(TOKEN_RBRACKET), A(ACT_INDEX)
    };
    // H → λ
    addRule(NT_H, FOLLOW_H, {});

    // ------ NT_E: ветка else ------
    // E → else A   семантика: 2 □
    // PROG2 срабатывает после 'else' (создаёт J-заглушку, заполняет JF-метку)
    parseTable[{NT_E, TOKEN_ELSE}] = {
        T(TOKEN_ELSE), A(ACT_PROG2), N(NT_A)
    };
    // E → λ
    addRule(NT_E, FOLLOW_E, {});

    // ------ NT_C: условие (S R S) ------
    // В НФ Грейбах: C → <начало_выражения> VU D
    // Все действия □ — сравнение выполняется в NT_D.

    parseTable[{NT_C, TOKEN_LPAREN}] = {
        T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_PLUS}] = {
        T(TOKEN_PLUS), N(NT_G), N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_MINUS}] = {
        T(TOKEN_MINUS), N(NT_G), A(ACT_NEG), N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_ID}] = {
        T(TOKEN_ID), A(ACT_OPERAND), N(NT_H), N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_INT_CONST}] = {
        T(TOKEN_INT_CONST), A(ACT_OPERAND), N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_REAL_CONST}] = {
        T(TOKEN_REAL_CONST), A(ACT_OPERAND), N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_SQRT}] = {
        T(TOKEN_SQRT), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        A(ACT_SQRT), N(NT_VM), N(NT_U), N(NT_D)
    };
    parseTable[{NT_C, TOKEN_ABS}] = {
        T(TOKEN_ABS), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        A(ACT_ABS), N(NT_VM), N(NT_U), N(NT_D)
    };

    // ------ NT_D: операция сравнения + правый операнд ------
    // D → < SZ   семантика: □ □ <
    // Действие сравнения стоит перед NT_Z, т.е. после обработки S.
    parseTable[{NT_D, TOKEN_LT}]  = { T(TOKEN_LT),  N(NT_S), A(ACT_LT),  N(NT_Z) };
    parseTable[{NT_D, TOKEN_GT}]  = { T(TOKEN_GT),  N(NT_S), A(ACT_GT),  N(NT_Z) };
    parseTable[{NT_D, TOKEN_LE}]  = { T(TOKEN_LE),  N(NT_S), A(ACT_LE),  N(NT_Z) };
    parseTable[{NT_D, TOKEN_GE}]  = { T(TOKEN_GE),  N(NT_S), A(ACT_GE),  N(NT_Z) };
    parseTable[{NT_D, TOKEN_EQ}]  = { T(TOKEN_EQ),  N(NT_S), A(ACT_EQ),  N(NT_Z) };
    parseTable[{NT_D, TOKEN_NE}]  = { T(TOKEN_NE),  N(NT_S), A(ACT_NE),  N(NT_Z) };

    // ------ NT_S: выражение (S → T U) ------
    // Семантика 'a' — после сопоставления id/константы.
    // Семантика -' — перед NT_VM (после NT_G), т.е. unary minus применяется к G.

    parseTable[{NT_S, TOKEN_LPAREN}] = {
        T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_PLUS}] = {
        T(TOKEN_PLUS), N(NT_G), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_MINUS}] = {
        T(TOKEN_MINUS), N(NT_G), A(ACT_NEG), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_ID}] = {
        T(TOKEN_ID), A(ACT_OPERAND), N(NT_H), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_INT_CONST}] = {
        T(TOKEN_INT_CONST), A(ACT_OPERAND), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_REAL_CONST}] = {
        T(TOKEN_REAL_CONST), A(ACT_OPERAND), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_SQRT}] = {
        T(TOKEN_SQRT), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        A(ACT_SQRT), N(NT_VM), N(NT_U)
    };
    parseTable[{NT_S, TOKEN_ABS}] = {
        T(TOKEN_ABS), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        A(ACT_ABS), N(NT_VM), N(NT_U)
    };

    // ------ NT_U: хвост выражения (U → + TU | - TU | λ) ------
    // Семантика +/- срабатывает перед рекурсивным NT_U (т.е. после NT_T).
    parseTable[{NT_U, TOKEN_PLUS}]  = { T(TOKEN_PLUS),  N(NT_T), A(ACT_ADD), N(NT_U) };
    parseTable[{NT_U, TOKEN_MINUS}] = { T(TOKEN_MINUS), N(NT_T), A(ACT_SUB), N(NT_U) };
    addRule(NT_U, FOLLOW_U, {});

    // ------ NT_T: терм (T → F V) ------
    parseTable[{NT_T, TOKEN_LPAREN}] = {
        T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_PLUS}] = {
        T(TOKEN_PLUS), N(NT_G), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_MINUS}] = {
        T(TOKEN_MINUS), N(NT_G), A(ACT_NEG), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_ID}] = {
        T(TOKEN_ID), A(ACT_OPERAND), N(NT_H), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_INT_CONST}] = {
        T(TOKEN_INT_CONST), A(ACT_OPERAND), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_REAL_CONST}] = {
        T(TOKEN_REAL_CONST), A(ACT_OPERAND), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_SQRT}] = {
        T(TOKEN_SQRT), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        A(ACT_SQRT), N(NT_VM)
    };
    parseTable[{NT_T, TOKEN_ABS}] = {
        T(TOKEN_ABS), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN),
        A(ACT_ABS), N(NT_VM)
    };

    // ------ NT_VM: хвост терма (V → * FV | / FV | λ) ------
    parseTable[{NT_VM, TOKEN_MULT}] = { T(TOKEN_MULT), N(NT_F), A(ACT_MUL), N(NT_VM) };
    parseTable[{NT_VM, TOKEN_DIV}]  = { T(TOKEN_DIV),  N(NT_F), A(ACT_DIV), N(NT_VM) };
    addRule(NT_VM, FOLLOW_VM, {});

    // ------ NT_F: фактор ------
    parseTable[{NT_F, TOKEN_LPAREN}] = {
        T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN)
    };
    parseTable[{NT_F, TOKEN_PLUS}] = {
        T(TOKEN_PLUS), N(NT_G)
    };
    // F → - G Z   семантика: □ -' (действие -' перед NT_Z, т.е. после G)
    parseTable[{NT_F, TOKEN_MINUS}] = {
        T(TOKEN_MINUS), N(NT_G), A(ACT_NEG), N(NT_Z)
    };
    parseTable[{NT_F, TOKEN_ID}] = {
        T(TOKEN_ID), A(ACT_OPERAND), N(NT_H)
    };
    parseTable[{NT_F, TOKEN_INT_CONST}]  = { T(TOKEN_INT_CONST),  A(ACT_OPERAND) };
    parseTable[{NT_F, TOKEN_REAL_CONST}] = { T(TOKEN_REAL_CONST), A(ACT_OPERAND) };
    parseTable[{NT_F, TOKEN_SQRT}] = {
        T(TOKEN_SQRT), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN), A(ACT_SQRT)
    };
    parseTable[{NT_F, TOKEN_ABS}] = {
        T(TOKEN_ABS), T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN), A(ACT_ABS)
    };

    // ------ NT_G: операнд унарных операций (без +/- как унарных) ------
    parseTable[{NT_G, TOKEN_LPAREN}]    = { T(TOKEN_LPAREN), N(NT_S), T(TOKEN_RPAREN) };
    parseTable[{NT_G, TOKEN_ID}]        = { T(TOKEN_ID), A(ACT_OPERAND), N(NT_H) };
    parseTable[{NT_G, TOKEN_INT_CONST}] = { T(TOKEN_INT_CONST),  A(ACT_OPERAND) };
    parseTable[{NT_G, TOKEN_REAL_CONST}]= { T(TOKEN_REAL_CONST), A(ACT_OPERAND) };

    // ------ NT_Z: вспомогательный нетерминал (всегда λ) ------
    // Семантическое действие Z несёт в себе родительское правило (ACT_* перед N(NT_Z)).
    // Само Z раскрывается в пустое правило для любого токена.
    addRule(NT_Z, ALL_TOKENS, {});
}

// ============================================================
// Вспомогательные методы
// ============================================================

Parser::Parser(const std::vector<Token>& toks)
    : tokens(toks), current(0) {
    if (!tokens.empty()) lastToken = tokens[0];
    initTable();
}

Token Parser::curTok() const {
    if (current < tokens.size()) return tokens[current];
    return Token(TOKEN_EOF, "", 0, 0);
}

std::string Parser::tokenName(int type) {
    switch (type) {
        case TOKEN_ASSIGN:    return "':='";
        case TOKEN_THEN:      return "'then'";
        case TOKEN_DO:        return "'do'";
        case TOKEN_END:       return "'end'";
        case TOKEN_BEGIN:     return "'begin'";
        case TOKEN_SEMICOLON: return "';'";
        case TOKEN_RPAREN:    return "')'";
        case TOKEN_RBRACKET:  return "']'";
        case TOKEN_LBRACKET:  return "'['";
        case TOKEN_LPAREN:    return "'('";
        case TOKEN_EOF:       return "конец файла";
        default:              return "символ";
    }
}

void Parser::matchToken(int expectedType) {
    Token tok = curTok();
    if (tok.type != expectedType) {
        throw ParserError("Ожидалось " + tokenName(expectedType),
                          tok.line, tok.column);
    }
    lastToken = tok;
    current++;
}

// ============================================================
// Семантические действия (раздел 4 описания)
// ============================================================

void Parser::executeAction(int action) {
    switch (action) {

        case ACT_NONE: break;

        // ---- Запись операнда в ОПС ----
        case ACT_OPERAND: {
            int idx;
            if (lastToken.type == TOKEN_ID) {
                if (symbols.findIntVar(lastToken.value, idx))
                    ops.addOperand(VAR_INT, idx, lastToken.line, lastToken.column);
                else if (symbols.findRealVar(lastToken.value, idx))
                    ops.addOperand(VAR_REAL, idx, lastToken.line, lastToken.column);
                else if (symbols.findIntArray(lastToken.value, idx))
                    ops.addOperand(ARRAY_INT, idx, lastToken.line, lastToken.column);
                else if (symbols.findRealArray(lastToken.value, idx))
                    ops.addOperand(ARRAY_REAL, idx, lastToken.line, lastToken.column);
                else
                    throw ParserError("Необъявленная переменная: " + lastToken.value,
                                      lastToken.line, lastToken.column);
            } else if (lastToken.type == TOKEN_INT_CONST) {
                // Числовое значение уже преобразовано в лексере
                idx = symbols.addIntConst(lastToken.int_val);
                ops.addOperand(CONST_INT, idx, lastToken.line, lastToken.column);
            } else if (lastToken.type == TOKEN_REAL_CONST) {
                // Числовое значение уже преобразовано в лексере
                idx = symbols.addRealConst(lastToken.real_val);
                ops.addOperand(CONST_REAL, idx, lastToken.line, lastToken.column);
            }
            break;
        }

        // ---- Арифметические операции ----
        case ACT_ASSIGN: ops.addOperation(OP_ASSIGN, lastToken.line, lastToken.column); break;
        case ACT_ADD:    ops.addOperation(OP_ADD,    lastToken.line, lastToken.column); break;
        case ACT_SUB:    ops.addOperation(OP_SUB,    lastToken.line, lastToken.column); break;
        case ACT_MUL:    ops.addOperation(OP_MUL,    lastToken.line, lastToken.column); break;
        case ACT_DIV:    ops.addOperation(OP_DIV,    lastToken.line, lastToken.column); break;
        case ACT_NEG:    ops.addOperation(OP_NEG,    lastToken.line, lastToken.column); break;
        case ACT_INDEX:  ops.addOperation(OP_INDEX,  lastToken.line, lastToken.column); break;

        // ---- Операции сравнения ----
        case ACT_LT:  ops.addOperation(OP_LT,  lastToken.line, lastToken.column); break;
        case ACT_GT:  ops.addOperation(OP_GT,  lastToken.line, lastToken.column); break;
        case ACT_LE:  ops.addOperation(OP_LE,  lastToken.line, lastToken.column); break;
        case ACT_GE:  ops.addOperation(OP_GE,  lastToken.line, lastToken.column); break;
        case ACT_EQ:  ops.addOperation(OP_EQ,  lastToken.line, lastToken.column); break;
        case ACT_NE:  ops.addOperation(OP_NE,  lastToken.line, lastToken.column); break;

        // ---- Ввод / вывод ----
        case ACT_READ:  ops.addOperation(OP_READ,  lastToken.line, lastToken.column); break;
        case ACT_WRITE: ops.addOperation(OP_WRITE, lastToken.line, lastToken.column); break;

        // ---- Стандартные функции ----
        case ACT_SQRT: ops.addOperation(OP_SQRT, lastToken.line, lastToken.column); break;
        case ACT_ABS:  ops.addOperation(OP_ABS,  lastToken.line, lastToken.column); break;

        // ---- Программа 1: условный переход (if/while) ----
        // Добавляем метку-заглушку и JF; позицию заглушки кладём в стек меток.
        case ACT_PROG1: {
            int jfPos = (int)ops.size();
            ops.addLabel(-1, lastToken.line, lastToken.column);
            ops.addOperation(OP_JF, lastToken.line, lastToken.column);
            labelStack.push(jfPos);
            break;
        }

        // ---- Программа 2: начало ветки else ----
        // Создаём J-заглушку, заполняем предыдущую JF-метку (она должна прыгнуть сюда),
        // кладём позицию J-заглушки в стек.
        case ACT_PROG2: {
            int jPos = (int)ops.size();
            ops.addLabel(-1, lastToken.line, lastToken.column);
            ops.addOperation(OP_J, lastToken.line, lastToken.column);
            ops.patchLabel(labelStack.top(), (int)ops.size()); // JF → начало else
            labelStack.top() = jPos;                           // теперь ждём заполнения J
            break;
        }

        // ---- Программа 3: конец if (заполнить последнюю открытую метку) ----
        case ACT_PROG3: {
            ops.patchLabel(labelStack.top(), (int)ops.size());
            labelStack.pop();
            break;
        }

        // ---- Программа 4: начало while (запомнить позицию начала цикла) ----
        case ACT_PROG4: {
            loopStack.push((int)ops.size()); // позиция = начало условия
            break;
        }

        // ---- Программа 5: конец while ----
        // Добавляем J-переход на начало цикла, заполняем JF-метку выходом из цикла.
        case ACT_PROG5: {
            int loopStart = loopStack.top(); loopStack.pop();
            ops.addLabel(loopStart, lastToken.line, lastToken.column);
            ops.addOperation(OP_J, lastToken.line, lastToken.column);
            ops.patchLabel(labelStack.top(), (int)ops.size()); // JF → после J
            labelStack.pop();
            break;
        }

        // ---- Вспомогательные действия для объявлений ----
        case ACT_SET_INT:  currentVarType = TYPE_INT;  break;
        case ACT_SET_REAL: currentVarType = TYPE_REAL; break;

        case ACT_SAVE_NAME:
            pendingName = lastToken.value;
            pendingLine = lastToken.line;
            pendingCol  = lastToken.column;
            break;

        case ACT_DECL_SCALAR:
            if (!pendingName.empty()) {
                if (currentVarType == TYPE_INT) symbols.addIntVar(pendingName);
                else                            symbols.addRealVar(pendingName);
                pendingName.clear();
            }
            break;

        case ACT_DECL_ARRAY: {
            // Числовое значение уже преобразовано в лексере
            int sz = lastToken.int_val;
            if (currentVarType == TYPE_INT) symbols.addIntArray(pendingName, sz);
            else                            symbols.addRealArray(pendingName, sz);
            pendingName.clear();
            break;
        }

        default: break;
    }
}

// ============================================================
// Главный цикл магазинного LL(1)-анализатора
// ============================================================

OPS Parser::parse() {
    // Инициализируем магазин: на дне маркер конца, на вершине — стартовый нетерминал P
    std::stack<StackSym> stk;
    stk.push(T(TOKEN_EOF));   // дно магазина
    stk.push(N(NT_P));        // стартовый нетерминал

    while (!stk.empty()) {
        StackSym top = stk.top();
        stk.pop();

        // ---- Семантическое действие ----
        if (top.kind == StackSym::ACTION) {
            executeAction(top.id);
            continue;
        }

        // ---- Терминал: сопоставить с текущей лексемой ----
        if (top.kind == StackSym::TERM) {
            if (top.id == TOKEN_EOF && curTok().type == TOKEN_EOF) break;

            // Если текущий токен — EOF, а ожидалось что-то конкретное —
            // сообщить, ЧЕГО не хватило (а не generic "unexpected EOF").
            if (curTok().type == TOKEN_EOF) {
                throw ParserError("Неожиданный конец файла. Ожидалось " + tokenName(top.id),
                                  curTok().line, curTok().column);
            }

            matchToken(top.id);
            continue;
        }

        // ---- Нетерминал: найти правило в таблице LL(1) ----
        auto key = std::make_pair(top.id, (int)curTok().type);
        auto it  = parseTable.find(key);

        if (it == parseTable.end()) {
            Token tok = curTok();
            if (tok.type == TOKEN_EOF) {
                throw ParserError("Неожиданный конец файла.", tok.line, tok.column);
            }
            throw ParserError("Неожиданный токен: '" + tok.value + "'",
                              tok.line, tok.column);
        }

        // Правило раскрываем в обратном порядке, чтобы первый элемент
        // оказался на вершине магазина
        const Rule& rule = it->second;
        for (int i = (int)rule.size() - 1; i >= 0; --i)
            stk.push(rule[i]);
    }

    return ops;
}
