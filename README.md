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

    tokenizer.add_begin_end_pair("/*", "*/", true, true, "Comments" /* Token type */);
    
    //
    // TokenBeginEndPair comment("/*", "*/"); If you want to keep the begin and end strings -> "/* this is a comment */"
    // TokenBeginEndPair comment("/*", "*/", false, false); If you do not want to keep the begin and end strings -> " this is a comment "
    // TokenBeginEndPair comment("/*", "*/", false, true); If you do not want to keep the begin string -> " this is a comment */"
    // TokenBeginEndPair comment("/*", "*/", true, false); If you do not want to keep the end string -> "/* this is a comment "
    //
    
    auto tokens = tokenizer.tokenize("hello /* world */ hey");

    for (auto& token : tokens) {
        if (token.token_type != "Comments")
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
    
    tokenizer.add_regex("[a-z][A-Z][0-9]", "RegexMatchAlphaNum");
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

    tokenizer.add_keyword("rl", "RL_Keyword");
    
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
int main(void)
{
    hl::Toks tokenizer;

    tokenizer.set_default_type("Identifier"); // change the default token type
    tokenizer.add_keyword("if", "If_Keyword");
    tokenizer.add_keyword("else", "Else_Keyword");
    tokenizer.add_keyword("return", "Return_Keyword");
    tokenizer.add_keyword("(", "Open_Paren");
    tokenizer.add_keyword(")", "Close_Paren");
    tokenizer.add_keyword("{", "Open_Brace");
    tokenizer.add_keyword("}", "Close_Brace");
    tokenizer.add_begin_end_pair("\"", "\"", false, false, "String_Literal");
    tokenizer.add_begin_end_pair("/*", "*/", false, false, "Comment");
    tokenizer.set_default_as_until_parser_match();

    // This will parse "a" "==" and "1" as identifiers as they are not registered as keywords
    // 91; will also be considered as an indentifier as ';' is not a keyword
    // It technically should be
    auto tokens = tokenizer.tokenize("if (a == 1) { /* comment */ return \"jaaj\" + 91; }");

    for (auto& token : tokens) {
        std::cout << "Parsed a token type: " << token.token_type
                  << " it has for value " << token.value
                  << " and was parsed at " << token.line << ":" << token.column
                    << std::endl;
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
#include <iostream>
#include "Toks.hpp"

class IdentifierParser : public hl::ToksParser<IdentifierParser> {
public:
    IdentifierParser() : hl::ToksParser<IdentifierParser>("Identifier") {}
    
    bool is_valid_char(char c, bool is_first_character) const {
        if (std::isalpha(c) || c == '_') {
            return true;
        }
        if (is_first_character)
            return false;
        return std::isdigit(c);
    }

    static hl::ParserCallbackResult parser_callback(hl::FileTokenStream& s, hl::TokenParser& parser) {
        auto& identifier = static_cast<IdentifierParser&>(parser);
        if (!identifier.is_valid_char(s.peek(), true))
            return nullptr;
        // A new TokenInfo featuring the token type an empty string, the current line and column 
        auto tok = hl::make_parser_callback_result(parser.token_type(), "", s.line(), s.column());
        do {
            tok->value += s.peek(); // Fill the value with current value of the FileTokenStream
            s.next(); // Continue to the next value of the stream
        } while (!s.eof() && identifier.is_valid_char(s.peek(), false)); // Until we are not at then of the stream and we are on a valid character
        return tok;
    }
};

int main(void)
{
    hl::Toks tokenizer;

    // You must register a callback for your parser
    // The callback must return either null if the parser does not work here
    tokenizer.register_parser_callback<IdentifierParser>();

    // Or a new valid parser callback result
    // You can then use it like so:
    
    tokenizer.add_parser(new IdentifierParser());

    for (auto& tok : tokenizer.tokenize("Hello World!")) {
        std::cout << tok.value << std::endl;
    }
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

## Tokenizing a full language:

```cpp
#include "Toks.hpp"
#include <tuple>
#include <array>
#include <algorithm>

// The only thing that it does not handle is the case
// where a string literal is escaped with a backslash
// ex: "Hello \"World\""
// This is because the tokenizer does not handle escape
// sequences by default but you can add them yourself
int main(void)
{
    hl::Toks tokenizer;

    // 3 types <name, value, type>
    enum {
        Keyword,
        BEPair,
        Regex
    };
    struct _keywords { const char *name; const char *value; int type; };
    std::array<_keywords> values {
        // Real restricted keywords
        { "If_Keyword", "if", Keyword },
        { "Else_Keyword", "else", Keyword },
        { "Return_Keyword", "return", Keyword },
        { "While_Keyword", "while", Keyword },
        { "For_Keyword", "for", Keyword },
        { "In_Keyword", "in", Keyword },
        { "Break_Keyword", "break", Keyword },
        { "Continue_Keyword", "continue", Keyword },
        { "Function_Keyword", "function", Keyword },
        { "Class_Keyword", "class", Keyword },
        { "New_Keyword", "new", Keyword },
        { "This_Keyword", "this", Keyword },
        { "Super_Keyword", "super", Keyword },
        { "Import_Keyword", "import", Keyword },
        { "From_Keyword", "from", Keyword },
        { "As_Keyword", "as", Keyword },
        { "Try_Keyword", "try", Keyword },
        { "Catch_Keyword", "catch", Keyword },
        { "Finally_Keyword", "finally", Keyword },
        { "Throw_Keyword", "throw", Keyword },
        { "Delete_Keyword", "delete", Keyword },
        { "Typeof_Keyword", "typeof", Keyword },
        { "Instanceof_Keyword", "instanceof", Keyword },
        { "Var_Keyword", "var", Keyword },
        { "Let_Keyword", "let", Keyword },
        { "Const_Keyword", "const", Keyword },
        { "Enum_Keyword", "enum", Keyword },
        { "Export_Keyword", "export", Keyword },
        { "Default_Keyword", "default", Keyword },
        { "Case_Keyword", "case", Keyword },
        { "Switch_Keyword", "switch", Keyword },
        { "Extends_Keyword", "extends", Keyword },
        { "Implements_Keyword", "implements", Keyword },
        { "Interface_Keyword", "interface", Keyword },
        { "Package_Keyword", "package", Keyword },
        { "Private_Keyword", "private", Keyword },
        { "Protected_Keyword", "protected", Keyword },
        { "Public_Keyword", "public", Keyword },
        { "Static_Keyword", "static", Keyword },
        { "Yield_Keyword", "yield", Keyword },
        { "Await_Keyword", "await", Keyword },
        { "Async_Keyword", "async", Keyword },

        // Keyword types
        { "Int_Keyword", "int", Keyword },
        { "Float_Keyword", "float", Keyword },
        { "Double_Keyword", "double", Keyword },
        { "Long_Keyword", "long", Keyword },
        { "Short_Keyword", "short", Keyword },
        { "Char_Keyword", "char", Keyword },
        { "Boolean_Keyword", "boolean", Keyword },
        { "Byte_Keyword", "byte", Keyword },
        { "Void_Keyword", "void", Keyword },
        { "Any_Keyword", "any", Keyword },
        { "String_Keyword", "string", Keyword },
        { "Object_Keyword", "object", Keyword },
        { "Array_Keyword", "array", Keyword },
        { "Map_Keyword", "map", Keyword },
        { "Set_Keyword", "set", Keyword },

        // Separators that can be nested (Treated as keywords)
        { "Open_Paren", "(", Keyword },
        { "Close_Paren", ")", Keyword },
        { "Open_Brace", "{", Keyword },
        { "Close_Brace", "}", Keyword },
        { "Open_Bracket", "[", Keyword },
        { "Close_Bracket", "]", Keyword },

        // Literals
        { "String_Literal_Double", "\"", BEPair },
        { "String_Literal_Single", "'", BEPair },
        { "String_Literal_Backtick", "`", BEPair },
        { "True_Literal", "true", Keyword },
        { "False_Literal", "false", Keyword },
        { "Null_Literal", "null", Keyword },
        { "Undefined_Literal", "undefined", Keyword },
        { "NaN_Literal", "NaN", Keyword },
        { "Infinity_Literal", "Infinity", Keyword },
        { "Integer_Literal", "[0-9]+", Regex },
        { "Float_Literal", "[0-9]+\\.[0-9]+", Regex },
        { "Hex_Literal", "0x[0-9a-fA-F]+", Regex },
        { "Binary_Literal", "0b[01]+", Regex },
        { "Octal_Literal", "0o[0-7]+", Regex },

        // Single Comments
        { "Comment", "//.*", Regex },
        { "Comment", "#.*", Regex },

        // Whitespace
        // { "Whitespace", "[ \t\r\n]+", Regex }, (Handled by default)

        // 3 character operators
        { "Rotate_Left", "<<<", Keyword },
        { "Rotate_Right", ">>>", Keyword },
        { "Strict_Equal", "===", Keyword },
        { "Strict_Not_Equal", "!==", Keyword },
        { "Left_Shift_Assign", "<<=", Keyword },
        { "Right_Shift_Assign", ">>=", Keyword },
        { "Power_Assign", "**=", Keyword },

        // 2 character operators
        { "Increment", "++", Keyword },
        { "Equal", "==", Keyword },
        { "Power", "**", Keyword },
        { "Decrement", "--", Keyword },
        { "Floor_Divide", "//", Keyword },
        { "Not_Equal", "!=", Keyword },
        { "Greater_Equal", ">=", Keyword },
        { "Less_Equal", "<=", Keyword },
        { "And", "&&", Keyword },
        { "Or", "||", Keyword },
        { "Left_Shift", "<<", Keyword },
        { "Right_Shift", ">>", Keyword },
        { "Plus_Assign", "+=", Keyword },
        { "Minus_Assign", "-=", Keyword },
        { "Multiply_Assign", "*=", Keyword },
        { "Divide_Assign", "/=", Keyword },
        { "Modulo_Assign", "%=", Keyword },
        { "Bitwise_And_Assign", "&=", Keyword },
        { "Bitwise_Or_Assign", "|=", Keyword },
        { "Bitwise_Xor_Assign", "^=", Keyword },
        { "Bitwise_Not_Assign", "~=", Keyword },
        { "Arrow", "->", Keyword },

        // 1 character operators
        { "Plus", "+", Keyword },
        { "Minus", "-", Keyword },
        { "Multiply", "*", Keyword },
        { "Divide", "/", Keyword },
        { "Modulo", "%", Keyword },
        { "Assign", "=", Keyword },
        { "Greater", ">", Keyword },
        { "Less", "<", Keyword },
        { "Not", "!", Keyword },
        { "Bitwise_And", "&", Keyword },
        { "Bitwise_Or", "|", Keyword },
        { "Bitwise_Xor", "^", Keyword },
        { "Bitwise_Not", "~", Keyword },
        { "Colon", ":", Keyword },
        { "Question", "?", Keyword },

        // 1 character separators
        { "Semicolon", ";", Keyword },
        { "Comma", ",", Keyword },
        { "Dot", ".", Keyword }
    };

    // sort by length to make sure nothing is shadowed
    std::sort(values.begin(), values.end(), [](const a &a, const a &b) {
        return strlen(a.value) > strlen(b.value);
    });

    // add all the values
    for (auto &v : values) {
        switch (v.type) {
            case Keyword: tokenizer.add_keyword(v.value, v.name); break;
            case BEPair: tokenizer.add_begin_end_pair(v.value, v.value, false, false, v.name); break;
            case Regex: tokenizer.add_regex(v.value, v.name); break;
        }
    }

    // Add the multiline comment parser
    tokenizer.add_begin_end_pair("/*", "*/", false, false, "Comment");

    // Add the identifier parser (used when there is no match)
    tokenizer.add_regex("[a-zA-Z_][a-zA-Z0-9_]*", "Identifier");

    const std::string code = /* read file */;

    for (auto &tok : tokenizer.tokenize(code, false /* do not parse default identifiers */)) {
        std::cout << "Parsed a token type: " << tok.token_type
                  << " it has for value " << tok.value
                  << " and was parsed at " << tok.line << ":" << tok.column
                  << std::endl;
    }
}
```