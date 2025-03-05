#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace PARSER {
    // Token types
    enum class TokenType {
        Number,
        Identifier,
        Keyword,
        Operator,
        LogicalAnd,
        LogicalOr,
        LogicalXor,
        LeftParen,
        RightParen,
        Assign,
        EndOfLine,
        EndOfFile
    };

    // Token structure
    struct Token {
        TokenType type;
        std::string value;
    };

    // Lexer
    class Lexer {
        std::string source;
        size_t position = 0;

        char back();
        char peek();
        char get();
        bool match(char expected);
    public:
        explicit Lexer(const std::string& sourceCode) : source(sourceCode) {}
        std::vector<Token> tokenize();
    private:
        bool isKeyword(const std::string& word);
    };

    // Interpreter
    class Interpreter {
        std::unordered_map<std::string, RValue> variables;
    public:
        int execution_context = -1;
        void execute(const std::vector<Token>& tokens);
    private:
        void executeStatement(const std::vector<Token>& tokens, size_t& index);
        YYRValue parseFunctionCall(const std::vector<Token>& tokens, size_t& index);
        void parseIfStatement(const std::vector<Token>& tokens, size_t& index);
        void parseVariableDeclaration(const std::vector<Token>& tokens, size_t& index);
        RValue evaluateExpression(const std::vector<Token>& tokens, size_t& index);
        RValue parsePrimary(const std::vector<Token>& tokens, size_t& index);
    };
    void ExecuteCode(std::string s, int context = -1);
}