# Toks
A single header configurable tokenizer utility provided basic tools for parsing

## Splitting automatically a string by using the spaces
```cpp
int main()
{
    auto tokens = hl::Toks().tokenize("Hello World");
    
    for (auto& token : tokens)
        std::cout << token.value << std::endl;
    // Hello
    // World
}
```

## Splitting everything by space but separating everything between `/* */`

```cpp
int main()
{
    hl::Toks tokenizer;

    tokenizer.add_begin_end_pair("/*", "*/", true, true, 1 /* the token type */);
    
    //
    // TokenBeginEndPair comment("/*", "*/"); If you want to keep the begin and end strings -> "/* this is a comment */"
    // TokenBeginEndPair comment("/*", "*/", false, false); If you do not want to keep the begin and end strings -> " this is a comment "
    // TokenBeginEndPair comment("/*", "*/", false, true); If you do not want to keep the begin string -> " this is a comment */"
    // TokenBeginEndPair comment("/*", "*/", true, false); If you do not want to keep the end string -> "/* this is a comment "
    //
    
    auto tokens = tokenizer.tokenize("hello /* world */ hey");

    // The token_type by default is -1 but can be changed
    for (auto& token : tokens) {
        if (token.token_type != 1)
            std::cout << "This is not a comment: " << token.value << std::endl;
        else
            std::cout << "This is a comment: " << token.value << std::endl;
    }
}
```

## Splitting everything by either words, either the regex match:
```cpp
int main()
{
    hl::Toks tokenizer;
    
    tokenizer.add_regex("[a-z][A-Z][0-9]", 1);
}
```

## Chose between parse until next parser matches or parse as words:
```cpp
int main()
{
    hl::Toks tokenizer;
    
    tokenizer.set_default_as_words(); // Is already set as words by default
    
    tokenizer.tokenize("Hello world"); // Outputs ["Hello", "World"]
    
    tokenizer.set_default_as_until_parser_matches(); // Parse the string a parser matches the input
    
    tokenizer.tokenize("Hello world"); // Outputs ["Hello World"] as there is no parser

    tokenizer.add_keyword("rl");
    
    tokenizer.set_default_as_words();
    tokenizer.tokenize("Hello world"); // Outputs ["Hello", "World"]
    
    tokenizer.set_default_as_until_parser_matches();
    tokenizer.tokenize("Hello world"); // Outputs ["Hello wo", "rl", "d"]
}
```

It will be soon possible to specify unauthorized characters in the default identifier.

## Hypothetical language parser

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
    IDENTIFIER
};

int main(void)
{
    hl::Toks tokenizer;

    tokenizer.set_default_type(TokenType::IDENTIFIER); // change the default token type
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
        std::cout << "A parsed has parsed a token type: " << token.token_type
                  << " it has for value " << token.value
                  << " and was parsed at " << token.line << ":" << token.column;
    }
}
```

The order of tokens matter so make sure to put them in order for example:
```cpp
    tokenizer.add_keyword("=", ...);
    tokenizer.add_keyword("==", ...); // This one will always be ignored due that the parser will parse the string as two "="
                                      // This is a problem as you cannot know if it parsed the string like this: "= =" or like this "=="
```
```cpp
    tokenizer.add_keyword("==", ...);
    tokenizer.add_keyword("=", ...);
    // This works because if there is no "==" there can still be one "=" so make sure to check the order of your tokens
```

For this kind of use you can forbid the parser to parse default identifiers like so: `tokenizer.tokenize("...", false);`

## Your own parsers:

As you saw this might feel limited so it is possible to add your own parsers to the tokenizer
```cpp
class IdentifierParser : public TokenParser {
public:
    IdentifierParser(int type=-1) : TokenParser(type) {}
    
    virtual int parser_type() const override {
        return 0; // Parser identifier user defined - Reserved ones are negative values as they are the builtin parsers
    }

    bool is_valid_char(char c, bool is_first_character) const {
        if (std::isalpha(c) || c == '_') {
            return true;
        }
        if (is_first_character)
            return false;
        return std::isdigit(c);
    }
};

int main(void)
{
    hl::Toks tokenizer;

    // You must register a callback for your parser
    // The callback must return either null if the parser does not work here
    // Or a new valid allocated pointer to a TokenInfo make sure to fill it well
    tokenizer.register_parser_callback(0,
        [](FileTokenStream& s, TokenParser& parser) -> TokenInfo * {
            auto& identifier = static_cast<IdentifierParser&>(parser);
            if (!identifier.is_valid_char(s.peek(), true))
                return nullptr;
            auto tok = new TokenInfo { parser.token_type(), "", s.line(), s.column() }; // A new TokenInfo featuring the 
            do {
                tok->value += s.peek();
                s.next(); // Continue to the next value of stream
            } while (!s.eof() && identifier.is_valid_char(s.peek(), false));
            if (s.eof() || s.is_whitespace()) // to be a valid identifier you need to have a space at the end or an eof
                return tok;
            delete tok;
            return nullptr;
       }
    );
    // FileTokenStream::next can take a size_t as parameter to specify how many characters to shift
    // If you shift more than the size of the string eof will be at true
    // FileTokenStream also features starts_with, skip_whitespaces, is_linebreak, find, substr
    // All thoses functions will use the current position of the string stream
    // FileTokenStream::str will return the raw std::string without moving it to the current position
    
    // You can then use it like so:
    
    tokenizer.add_parser(new IdentifierParser());
    // or
    tokenizer.add_parser(std::move(std::make_unique<IdentifierParser>()));
}
```
