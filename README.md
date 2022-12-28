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
        std::cout << "Parsed a token type: " << token.token_type
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
            // A new TokenInfo featuring the token type an empty string, the current line and column 
            auto tok = new TokenInfo { parser.token_type(), "", s.line(), s.column() };
            do {
                tok->value += s.peek(); // Fill the value with current value of the FileTokenStream
                s.next(); // Continue to the next value of the stream
            } while (!s.eof() && identifier.is_valid_char(s.peek(), false)); // Until we are not at then of the stream and we are on a valid character
            return tok;
       }
    );

    // You can then use it like so:
    
    tokenizer.add_parser(new IdentifierParser());
    // or
    tokenizer.add_parser(std::move(std::make_unique<IdentifierParser>()));
}
```
## Quick overview of the file token stream:
```cpp
// FileTokenStream functions:
// Get the current character
stream.peek(); // character
```

```cpp
// Get the current line and column
size_t line = stream.line();
size_t col = stream.column();
```

```cpp
// Pos will return the current position of the index stream
stream.pos(); // size_t

// All thoses functions will use the current position of the string stream
// FileTokenStream::str will return the raw std::string without moving it to the current position
// If you want to get the string a the current pos you need to either use the begin iterator or the cstring
// You should not generally do that by hand
// The strings returned are of course constants
stream.str().begin() + stream.pos(); // current pos
stream.c_str() + stream.pos();
```

```
// next can take a size_t as parameter to specify how many characters to shift
stream.next(); // shift of 1
stream.next(n); // shift of n
```

```cpp
// eof will tell you if you are at the end of the stream
stream.eof();
```

```cpp
// If you shift more than the size of the string eof will be at true
// This is the only valid way to shift as FileTokenStream::seek will break the m_line and m_column parameter
// This is not a problem if you do not care about thoses but it must be considered as it is was the whole point
// of the FileTokenStream handler
stream.seek(pos);
```

```cpp
// FileTokenStream also features starts_with, skip_whitespaces, is_linebreak, find, substr, regex_match
stream.starts_with("hello"); // Works like std::string::starts_with (c++20)
stream.skip_whitespace(); // Skips all matching std::isspace
stream.is_linebreak(); // Is it \n or \r or \r\n
stream.find("str", optional_pos); // Works like std::string::find
stream.substr(0, 10); // Works like std::string::substr

std::smatch match;
stream.regex_match(std::regex(".*"), match);
// return a boolean and fill the match this works like std::regex_match
// and uses the stream starting position
```

```cpp
// You can store states by using the stack state of the filestream
stream.push_state(); // register the last state of the parser

// You may go forward
stream.next();

// Use destructive functions for some parameters
stream.seek(10);

// restore the state
stream.pop_state(); // Restore the stream to it's last pushed state this is the same as stream.pop_state(true); 

// If you do not want to restore the stream but want to pop the state
stream.pop_state(false);

// The stream stack is good when you are unsure of what you do and need to go back

// As this is a stack you can push multiple states but do not forget to pop them
```
