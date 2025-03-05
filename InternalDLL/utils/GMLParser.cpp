#include "crt.h"
#include "../dependencies/Zydis/Zydis.h"
#include "../Drawing.h"
#include "api.h"
#include "logging.h"
#include "GMLParser.h"

namespace PARSER {
    char Lexer::back() {
        return position >= 1 ? source[position - 1] : '\0';
    }

    char Lexer::peek() {
        return position < source.size() ? source[position] : '\0';
    }

    char Lexer::get() {
        return position < source.size() ? source[position++] : '\0';
    }

    bool Lexer::match(char expected) {
        if (peek() == expected) {
            get();
            return true;
        }
        return false;
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;

        while (position < source.size()) {
            char current = peek();

            if (std::isspace(current)) {
                get();  // Skip whitespace
            }
            else if (std::isalpha(current) || current == '_') {  // Keywords or Identifiers
                std::string identifier;
                while (std::isalnum(peek()) || peek() == '_') {
                    identifier += get();
                }
                if (isKeyword(identifier)) {
                    if (identifier == "else")
                        if (tokens.back().value == "end")
                            tokens.pop_back(); // Nom the useless end token

                    tokens.push_back({ TokenType::Keyword, identifier });
                }
                else {
                    tokens.push_back({ TokenType::Identifier, identifier });
                }
            }
            else if (std::isdigit(current)) {  // Numbers
                std::string number;
                char _peek = peek();
                while (_peek == '.' || std::isdigit(_peek)) {
                    number += get();
                    _peek = peek();
                }
                tokens.push_back({ TokenType::Number, number });
            }
            else if (current == '"') {  // Strings
                std::string str;
                str += get();  // Consume opening quote
                while (peek() != '\0') {
                    if (back() != '\\' && peek() == '"')
                        break;
                    str += get();
                }
                if (peek() == '"') {
                    str += get();  // Consume closing quote
                    tokens.push_back({ TokenType::Identifier, str });  // Use Identifier for strings
                }
                else {
                    throw std::runtime_error("Unterminated string literal");
                }
            }
            else if (current == '[') { // Arrays
                int off = 1;
                bool inString = false;
                std::string result{};
                result += get(); // Consume opening bracket
                while (off > 0) {
                    switch (peek()) {
                    case '\0':
                        off = -1;
                        break;
                    case '[':
                        if (inString)
                            break;
                        off++;
                        break;
                    case ']':
                        if (inString)
                            break;
                        off--;
                        break;
                    case '"':
                        if (back() != '\\')
                            inString = !inString;
                        break;
                    }
                    result += get();
                }
                if (off == 0) {
                    tokens.push_back({ TokenType::Identifier, result }); // Use Identifier for arrays too, because I am stupid
                }
                else {
                    throw std::runtime_error("Unterminated array");
                }
            }
            else if (current == '+') {  // +
                get();
                tokens.push_back({ TokenType::Operator, "+" });
            }
            else if (current == '-') {  // -
                get();
                tokens.push_back({ TokenType::Operator, "-" });
            }
            else if (current == '*') {  // *
                get();
                tokens.push_back({ TokenType::Operator, "*" });
            }
            else if (current == '/') {  // /
                get();
                tokens.push_back({ TokenType::Operator, "/" });
            }
            else if (current == '&') {  // &&
                get();
                if (match('&')) {
                    tokens.push_back({ TokenType::LogicalAnd, "&&" });
                }
                else {
                    throw std::runtime_error("Unexpected '&'");
                }
            }
            else if (current == '|') {  // ||
                get();
                if (match('|')) {
                    tokens.push_back({ TokenType::LogicalOr, "||" });
                }
                else {
                    throw std::runtime_error("Unexpected '|'");
                }
            }
            else if (current == '>') {  // >, >=
                get();
                if (match('=')) {
                    tokens.push_back({ TokenType::Operator, ">=" });
                }
                else {
                    tokens.push_back({ TokenType::Operator, ">" });
                }
            }
            else if (current == '<') {  // <, <=
                get();
                if (match('=')) {
                    tokens.push_back({ TokenType::Operator, "<=" });
                }
                else {
                    tokens.push_back({ TokenType::Operator, "<" });
                }
            }
            else if (current == '=') {  // ==, =
                get();
                if (match('=')) {
                    tokens.push_back({ TokenType::Operator, "==" });
                }
                else {
                    tokens.push_back({ TokenType::Assign, "=" });
                }
            }
            else if (current == '!') {  // !=
                get();
                if (match('=')) {
                    tokens.push_back({ TokenType::Operator, "!=" });
                }
                else {
                    tokens.push_back({ TokenType::Operator, "!" });
                }
            }
            else if (current == ',') {
                get();
                tokens.push_back({ TokenType::Operator, "," });
            }
            else if (current == '(') {  // (
                get();
                tokens.push_back({ TokenType::LeftParen, "(" });
            }
            else if (current == ')') {  // )
                get();
                tokens.push_back({ TokenType::RightParen, ")" });
            }
            else if (current == ';') {  // ;
                get();
                tokens.push_back({ TokenType::EndOfLine, ";" });
            }
            else if (current == '{') { // Alias for then
                get();
                if (tokens.back().value != "else")
                    tokens.push_back({ TokenType::Keyword, "then" });
            }
            else if (current == '}') { // Alias for end
                get();
                tokens.push_back({ TokenType::Keyword, "end" });
            }
            else {
                throw std::runtime_error("Unexpected character: " + std::string(1, current));
            }
        }

        tokens.push_back({ TokenType::EndOfFile, "" });
        return tokens;
    }

    bool Lexer::isKeyword(const std::string& word) {
        static const std::unordered_set<std::string> keywords = {
            "if", "then", "else", "end", "var"
        };
        return keywords.count(word) > 0;
    }

    void Interpreter::execute(const std::vector<Token>& tokens) {
        size_t index = 0;
        while (index < tokens.size() && tokens[index].type != TokenType::EndOfFile) {
            executeStatement(tokens, index);
        }
    }

    void Interpreter::executeStatement(const std::vector<Token>& tokens, size_t& index) {
        L_PRINT(LOG_INFO) << "Evaluating: " << tokens[index].value.c_str() << " at index " << index
            << " of type " << static_cast<int>(tokens[index].type);

        if (tokens[index].type == TokenType::Keyword && tokens[index].value == "if") {
            parseIfStatement(tokens, index);
        }
        else if (tokens[index].type == TokenType::Keyword && tokens[index].value == "var") {
            parseVariableDeclaration(tokens, index);
        }
        else if (tokens[index].type == TokenType::Identifier) {
            parseFunctionCall(tokens, index);
        }
        else {
            throw std::runtime_error("Unexpected token: " + tokens[index].value);
        }
    }

    YYRValue Interpreter::parseFunctionCall(const std::vector<Token>& tokens, size_t& index) {
        std::string functionName = tokens[index].value;
        index++;  // Consume function name

        if (index >= tokens.size() || tokens[index].type != TokenType::LeftParen) {
            throw std::runtime_error("Expected '(' after function name: " + functionName);
        }
        index++;  // Consume '('

        std::vector<YYRValue> arguments;

        while (index < tokens.size() && tokens[index].type != TokenType::RightParen) {
            if (tokens[index].type == TokenType::Identifier || tokens[index].type == TokenType::Number) {
                if (tokens[index + 1].type == TokenType::LeftParen) {
                    // It's a function call inside the arguments. Wow.
                    std::vector<Token> newTokens{};
                    size_t newIndex = 0;
                    while (tokens[index].type != TokenType::RightParen)
                        newTokens.push_back(tokens[index++]);
                    newTokens.push_back({ TokenType::RightParen, ")" });
                    newTokens.push_back({ TokenType::EndOfLine, ";" });
                    newTokens.push_back({ TokenType::EndOfFile, "" });
                    YYRValue res = parseFunctionCall(newTokens, newIndex);
                    arguments.push_back(res);
                    index++; // Consume the soy!
                    continue;
                }
                std::string arg_str = tokens[index].value;
                if (variables.contains(arg_str))
                    arguments.push_back(variables.at(arg_str));
                else
                    arguments.push_back(API::StringToRValue(arg_str));
                index++;  // Consume argument
            }

            if (tokens[index].type == TokenType::RightParen) {
                break;  // End of arguments
            }

            if (tokens[index].type == TokenType::Operator && tokens[index].value == ",") {
                index++;  // Consume ','
            }
            else if (tokens[index].type == TokenType::Operator) {
                index--;
                arguments.pop_back();
                arguments.push_back(evaluateExpression(tokens, index));
                if (tokens[index].type == TokenType::RightParen)
                    break;
                if (tokens[index].value != ",")
                    throw std::runtime_error("Expected ',' or ')' in function arguments during evaluation.");
            } else {
                throw std::runtime_error("Expected ',' or ')' in function arguments.");
            }
        }

        if (index >= tokens.size() || tokens[index].type != TokenType::RightParen) {
            throw std::runtime_error("Expected ')' to close function call.");
        }
        index++;  // Consume ')'

        // Simulate function execution
        L_PRINT(LOG_INFO) << "Function Call: " << functionName.c_str() << " with arguments: ";
        for (unsigned int i = 0; i < arguments.size(); i++) {
            const auto& arg = arguments[i];
            L_PRINT(LOG_INFO) << API::RValueToString(arg).c_str();
        }
        CInstance* ctx = execution_context < 0 ? nullptr : API::GetObjectInstanceFromId(execution_context);

        if (index < tokens.size() && tokens[index].type == TokenType::EndOfLine) {
            index++;  // Consume ';' if present
        }
        return API::CallFunction(functionName.c_str(), arguments, ctx);
    }

    void Interpreter::parseIfStatement(const std::vector<Token>& tokens, size_t& index) {
        index++;  // Consume 'if'

        // Parse condition
        RValue condition = evaluateExpression(tokens, index);

        // Ensure the next token is 'then'
        if (index >= tokens.size() || tokens[index].type != TokenType::Keyword || tokens[index].value != "then") {
            throw std::runtime_error("Expected 'then' or '{' keyword after 'if' condition.");
        }
        index++;  // Consume 'then'

        if (condition.asBool()) {
            // Execute the 'then' branch
            executeStatement(tokens, index);
            // Skip the 'else' branch if present
            while (index < tokens.size() &&
                tokens[index].type == TokenType::Keyword &&
                tokens[index].value != "end") {
                if (tokens[index].value == "else") {
                    index++;  // Consume 'else'
                    while (index < tokens.size() &&
                        tokens[index].type != TokenType::Keyword &&
                        tokens[index].value != "end") {
                        index++;
                    }
                }
                else {
                    break;
                }
            }
        }
        else {
            // Skip the 'then' branch
            while (index < tokens.size() &&
                tokens[index].type != TokenType::Keyword &&
                tokens[index].value != "else") {
                index++;
            }

            // Handle the 'else' branch if present
            if (index < tokens.size() && tokens[index].value == "else") {
                index++;  // Consume 'else'
                executeStatement(tokens, index);
            }
        }

        // Ensure the block ends with 'end'
        if (index >= tokens.size() || tokens[index].type != TokenType::Keyword || tokens[index].value != "end") {
            throw std::runtime_error("Expected 'end' or '}' keyword to close 'if' statement.");
        }
        index++;  // Consume 'end'
    }

    void Interpreter::parseVariableDeclaration(const std::vector<Token>& tokens, size_t& index) {
        index++;  // Consume 'var'

        if (tokens[index].type != TokenType::Identifier) {
            throw std::runtime_error("Expected variable name after 'var'.");
        }

        std::string varName = tokens[index].value;
        index++;  // Consume variable name

        if (tokens[index].type != TokenType::Assign) {
            throw std::runtime_error("Expected '=' after variable name.");
        }
        index++;  // Consume '='

        RValue value = evaluateExpression(tokens, index);
        variables[varName] = value;

        if (tokens[index - 1].type == TokenType::EndOfLine) { // something evil happened
            index--;
        }

        if (tokens[index].type != TokenType::EndOfLine) {
            throw std::runtime_error("Expected ';' after variable declaration.");
        }
        index++;  // Consume ';'
    }

    RValue Interpreter::evaluateExpression(const std::vector<Token>& tokens, size_t& index) {
        if (index >= tokens.size()) {
            throw std::runtime_error("Unexpected end of expression");
        }
        RValue result{};
        RValue _left = parsePrimary(tokens, index);
        if (_left.kind != VALUE_REAL)
            return _left;
        double left = _left.asReal();

        while (index < tokens.size()) {
            const Token& token = tokens[index];

            // Handle relational operators
            if (token.type == TokenType::Operator) {
                std::string op = token.value;
                if (op == ",")
                    break;
                index++;  // Consume operator

                RValue _right = parsePrimary(tokens, index);
                double right = _right.asReal();

                L_PRINT(LOG_INFO) << "Evaluating: " << left << " " << op.c_str() << " " << right;

                if (op == ">") {
                    left = left > right;
                }
                else if (op == "<") {
                    left = left < right;
                }
                else if (op == ">=") {
                    left = left >= right;
                }
                else if (op == "<=") {
                    left = left <= right;
                }
                else if (op == "==") {
                    left = left == right;
                }
                else if (op == "!=") {
                    left = left != right;
                }
                else if (op == "+") {
                    left += right;
                }
                else if (op == "-") {
                    left -= right;
                }
                else if (op == "*") {
                    left *= right;
                }
                else if (op == "/") {
                    left /= right;
                }
                else {
                    throw std::runtime_error("Unexpected operator: " + op);
                }
                result = YYRValue(left);
            }
            else {
                break;  // Exit loop if no operator is found
            }
        }

        return result;
    }


    RValue Interpreter::parsePrimary(const std::vector<Token>& tokens, size_t& index) {
        if (index >= tokens.size()) {
            throw std::runtime_error("Unexpected end of expression");
        }

        const Token& token = tokens[index];
        if (token.type == TokenType::Number) {
            index++;  // Consume number
            return YYRValue(std::stod(token.value));
        }
        else if (token.type == TokenType::Identifier) {
            std::string varName = token.value;
            index++;  // Consume identifier

            // Retrieve variable value
            if (variables.find(varName) != variables.end()) {
                return variables[varName];
            }
            else {
                RValue res = API::StringToRValue(varName);
                if (res.kind != VALUE_UNDEFINED)
                    return res;
                int temp = -1;
                if (FunctionFind(varName.c_str(), &temp)) {
                    index--;
                    YYRValue res = parseFunctionCall(tokens, index);
                    return res;
                }
                throw std::runtime_error("Undefined variable: " + varName);
            }
        }
        else if (token.type == TokenType::LeftParen) {
            index++;  // Consume '('
            RValue value = evaluateExpression(tokens, index);
            if (index >= tokens.size() || tokens[index].type != TokenType::RightParen) {
                throw std::runtime_error("Expected ')'");
            }
            index++;  // Consume ')'
            return value;
        }
        else {
            throw std::runtime_error("Unexpected token in expression: " + token.value);
        }
    }

    void ExecuteCode(std::string s, int context) {
        try {
            L_PRINT(LOG_INFO) << "Initializing lexer...";
            Lexer lexer(s);
            L_PRINT(LOG_INFO) << "Tokenizing code...";
            auto tokens = lexer.tokenize();

            L_PRINT(LOG_INFO) << "Initializing interpreter...";
            Interpreter interpreter;
            L_PRINT(LOG_INFO) << "Setting context...";
            interpreter.execution_context = context;
            L_PRINT(LOG_INFO) << "Executing code...";
            interpreter.execute(tokens);
        }
        catch (const std::runtime_error& ex) {
            ImGuiToast toast(ImGuiToastType::Error);
            toast.setTitle("GML Error");
            toast.setContent(ex.what());
            ImGui::InsertNotification(toast);
            Drawing::bErrorOccurred = true;
        }
    }
}