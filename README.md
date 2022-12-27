# Toks
A single header configurable tokenizer utility provided basic tools for parsing

```cpp
// Here is an example of a simple tokenizer for an hypothetical programming language

#include <iostream>
enum TokenType {
    IF_KEYWORD,
    ELSE_KEYWORD,
    RETURN_KEYWORD,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    STRING_LITERAL,
    COMMENT,
    IDENTIFIER = -1
};

int main(int, char** argv)
{
    hl::Tokenizer tokenizer;

    tokenizer.set_default_type(TokenType::IDENTIFIER);
    tokenizer.add_keyword("if", TokenType::IF_KEYWORD);
    tokenizer.add_keyword("else", TokenType::ELSE_KEYWORD);
    tokenizer.add_keyword("return", TokenType::RETURN_KEYWORD);
    tokenizer.add_keyword("(", TokenType::OPEN_PAREN);
    tokenizer.add_keyword(")", TokenType::CLOSE_PAREN);
    tokenizer.add_keyword("{", TokenType::OPEN_BRACE);
    tokenizer.add_keyword("}", TokenType::CLOSE_BRACE);
    tokenizer.add_begin_end_pair("\"", "\"", false, false, TokenType::STRING_LITERAL);
    tokenizer.add_begin_end_pair("/*", "*/", false, false, TokenType::COMMENT);

    // This will parse "a" "==" and "1" as identifiers as they are not registered as keywords
    // "1;" will also be considered as an indentifier as ';' is not a keyword
    // It technically should be
    auto tokens = tokenizer.tokenize("if (a == 1) { /* comment */ return \"jaaj\"; }");

    for (auto& token : tokens) {
        std::cout << "A parser of type: " << token.type << " has parsed a token type: " << token.token_type
                  << " it has for value " << token.value << " and was parsed at " << token.line << ":" << token.column;
    }
}
```
