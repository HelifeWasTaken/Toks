#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <stack>
#include <tuple>
#include <regex>

namespace hl {


// A simple token stream that can be used to tokenize a string
// It is not meant to be used for large files, but rather for small
// files that can be loaded into memory.
// It is used to easily tokenize a string, and it is not meant to be
// used for anything else.
//
// It will replace all \r\n with \n, and all \r with \n
// It will also keep track of the line and column of the current position
// in the string.
//
// It also provides a simple way to skip whitespace, and get a word
// (a string of characters that are not whitespace)
//
class FileTokenStream {
private:
    std::string m_string;
    size_t m_pos = 0;
    size_t m_line = 0;
    size_t m_column = 0;

    std::stack<std::tuple<size_t, size_t, size_t>> m_stack_states;

public:
    // Create a token stream from a string
    FileTokenStream(const std::string& string)
        : m_string(string)
    {
        // replace all \r\n with \n
        for (size_t i = 0; i < m_string.size(); ++i) {
            if (m_string[i] == '\r' && m_string[i + 1] == '\n') {
                m_string.erase(i, 1);
            }
        }
        // replace all \r with \n
        for (size_t i = 0; i < m_string.size(); ++i) {
            if (m_string[i] == '\r') {
                m_string.erase(i, 1);
            }
        }
    }

    // returns the string that is being tokenized
    const std::string& str() const {
        return m_string;
    }

    const char* c_str() const {
        return m_string.c_str();
    }

    // returns the current position in the string
    size_t pos() const {
        return m_pos;
    }

    // returns the current line in the string
    size_t line() const {
        return m_line;
    }

    // returns the current column in the string
    size_t column() const {
        return m_column;
    }

    // returns the size of the string
    size_t size() const {
        return m_string.size();
    }

    // returns true if the end of the string has been reached
    bool eof() const {
        return m_pos >= m_string.size();
    }

    // returns the character at the current position
    char peek() const {
        return m_string[m_pos];
    }

    bool is_linebreak() const {
        return peek() == '\n' || peek() == '\r';
    }

    bool is_whitespace() const {
        return peek() == ' ' || peek() == '\t' || is_linebreak();
    }

    // returns the character at the current position, and advances the position
    void next(size_t n=1) {
        for (size_t i = 0; i < n && !eof(); ++i) {
            if (is_linebreak()) {
                ++m_line;
                m_column = 0;
            } else {
                ++m_column;
            }
            ++m_pos;
        }
    }

    // skips all whitespace characters
    void skip_whitespace() {
        for (; !eof() && is_whitespace(); next());
    }

    // checks if the current position starts with the given string
    bool starts_with(const std::string& str) const {
        return m_string.compare(m_pos, str.size(), str) == 0;
    }

    // finds the first occurence of the given string, starting at the current position
    size_t find(const std::string& str, size_t pos=0) const {
        // start at m_pos, and find the first occurence of str
        return m_string.find(str, m_pos + pos) - m_pos;
    }

    // Substring from the current position
    std::string substr(size_t pos, size_t len) const {
        return m_string.substr(m_pos + pos, len);
    }

    // Checks if the given regex matches the current position
    bool regex_match(const std::regex& regex, std::smatch& match) const {
        return std::regex_search(m_string.cbegin() + m_pos, m_string.cend(), match, regex);
    }

    // Stores the current position, line and column (so that it can be restored later)
    void push_state() {
        m_stack_states.push(std::make_tuple(m_pos, m_line, m_column));
    }

    // Frees the stored state, and optionally restores it
    // It restores the position, line and column by default
    void pop_state(bool restore=true) {
        if (restore) {
            auto state = m_stack_states.top();
            m_pos = std::get<0>(state);
            m_line = std::get<1>(state);
            m_column = std::get<2>(state);
        }
        m_stack_states.pop();
    }
};

// Represents a token that has been parsed from a string
struct TokenInfo {
    const char *token_type; // token_type is the type of the token
    std::string value; // the value of the token
    size_t line, column; // the line and column of the token

    TokenInfo(const char *token_type, const std::string& keyword, const size_t &line, const size_t &column)
        : token_type(token_type), value(keyword), line(line), column(column)
    {}
};

class TokenParser;
class Tokenizer;

// Represents how a specific token will be parsed from a string
using ParserCallbackResult = std::unique_ptr<TokenInfo>;
using ParserCallback = std::function<ParserCallbackResult(FileTokenStream&, TokenParser&)>;


static ParserCallbackResult make_parser_callback_result(const char *token_type, const std::string& keyword, const size_t &line, const size_t &column)
{
    return std::make_unique<TokenInfo>(token_type, keyword, line, column);
}

// Base class for all token parsers
class TokenParser {
private:
    const char *m_token_type;
    const char *m_parser_name;

public:
    // Utility function to standardize the name of the parser
    template<typename T>
    static const char *GetParserTypeName() {
        return typeid(T).name();
    }

protected:
    // the type of the token
    TokenParser(const char *token_type, const char *parser_name)
        : m_token_type(token_type), m_parser_name(parser_name)
    {}

public:
    // Destructor
    virtual ~TokenParser() = default;

    // returns the type of the parser
    const char *parser_type() const {
        return m_parser_name;
    }

    // returns the type of the specific registered token
    const char *token_type() const {
        return m_token_type;
    }
};

template<typename T>
class TokenParserProxy : public TokenParser {
public:
    // the type of the token
    TokenParserProxy(const char *token_type)
        : TokenParser(token_type, GetParserTypeName<T>())
    {}

    // Destructor
    virtual ~TokenParserProxy() = default;
};

// A token parser that parses a keyword
class TokenKeyword : public TokenParserProxy<TokenKeyword> {
public:
    // the keyword to parse
    std::string m_keyword;

    static ParserCallbackResult parser_callback(FileTokenStream& s, TokenParser& parser) {
        auto& keyword = static_cast<TokenKeyword&>(parser);

        if (s.starts_with(keyword.keyword())) {
            auto token = make_parser_callback_result(keyword.token_type(), keyword.keyword(), s.line(), s.column());
            s.next(keyword.keyword().size());
            return token;
        }
        return nullptr;
    }

public:
    // create a new keyword parser
    TokenKeyword(const std::string& keyword, const char *type)
        : TokenParserProxy(type)
        , m_keyword(keyword)
    {}

    // Destructor
    virtual ~TokenKeyword() = default;

    // returns the keyword
    const std::string& keyword() const {
        return m_keyword;
    }
};

// This token parser parses a pair of begin and end strings
// This may be used to parse string literals, comments, etc.
// It will keep track of the begin and end strings, and the
// contents between them.
//
// For example, if the begin string is "/*" and the end string
// is "*/", then the following string:
//
// "/* this is a comment */"
//
// It can be used like this:
//
// TokenBeginEndPair comment("/*", "*/"); If you want to keep the begin and end strings -> "/* this is a comment */"
// TokenBeginEndPair comment("/*", "*/", false, false); If you do not want to keep the begin and end strings -> " this is a comment "
// TokenBeginEndPair comment("/*", "*/", false, true); If you do not want to keep the begin string -> " this is a comment */"
// TokenBeginEndPair comment("/*", "*/", true, false); If you do not want to keep the end string -> "/* this is a comment "
//
class TokenBeginEndPair : public TokenParserProxy<TokenBeginEndPair> {
public:

    static ParserCallbackResult parser_callback(FileTokenStream& s, TokenParser& parser) {
        auto& begin_end = static_cast<TokenBeginEndPair&>(parser);

        if (!s.starts_with(begin_end.begin())) {
            return nullptr;
        }

        auto pos = s.find(begin_end.end(), begin_end.begin().size());

        if (pos == std::string::npos) {
            return nullptr;
        }

        std::string result = s.substr(0, pos + begin_end.end().size());

        if (!begin_end.keep_begin()) {
            result.erase(0, begin_end.begin().size());
        }
        if (!begin_end.keep_end()) {
            result.erase(result.size() - begin_end.end().size(), begin_end.end().size());
        }

        auto token = make_parser_callback_result(begin_end.token_type(), result, s.line(), s.column());

        s.next(pos + begin_end.end().size());
        return token;
    }

private:
    // the begin and end strings
    std::string m_begin;
    std::string m_end;

    // whether or not to keep the begin and end strings
    bool m_keep_begin = false;
    bool m_keep_end = false;

public:
    // create a new begin/end pair parser
    TokenBeginEndPair(const std::string& begin, const std::string& end,
                    bool keep_begin, bool keep_end,
                    const char *type)
        : TokenParserProxy(type)
        , m_begin(begin)
        , m_end(end)
        , m_keep_begin(keep_begin)
        , m_keep_end(keep_end)
    {}

    // returns whether or not to keep the begin string
    bool keep_begin() const {
        return m_keep_begin;
    }

    // returns whether or not to keep the end string
    bool keep_end() const {
        return m_keep_end;
    }

    // returns the begin string
    const std::string& begin() const {
        return m_begin;
    }

    // returns the end string
    const std::string& end() const {
        return m_end;
    }

    // Destructor
    virtual ~TokenBeginEndPair() = default;
};

class RegexParser : public TokenParserProxy<RegexParser> {
public:

    static ParserCallbackResult parser_callback(FileTokenStream& s, TokenParser& parser) {
        auto& regex = static_cast<RegexParser&>(parser);

        std::smatch match;
        if (s.regex_match(regex.regex(), match)) {
            if (match.size() == 0) {
                return nullptr;
            }
            auto token = make_parser_callback_result(regex.token_type(), match.str(), s.line(), s.column());
            s.next(match.position() + match.length());
            return token;
        }
        return nullptr;
    }

private:
    std::regex m_regex;

public:
    // create a new regex parser
    RegexParser(const std::string& regex, const char *type)
        : TokenParserProxy(type)
        , m_regex(regex)
    {}

    // returns the regex
    const std::regex& regex() const {
        return m_regex;
    }

    // Destructor
    virtual ~RegexParser() = default;
};

// Implements a combinator parser (Works like a AND operator)
class CombinatorParser : public TokenParserProxy<CombinatorParser> {
private:
    std::vector<std::unique_ptr<TokenParser>> m_parsers;
    Tokenizer& m_tokenizer_ref;

public:
    // create a new combinator parser
    CombinatorParser(const char *type, Tokenizer& tokenizer)
        : TokenParserProxy(type)
        , m_tokenizer_ref(tokenizer)
    {}

    // add a new parser
    void add_parser(std::unique_ptr<TokenParser> &&parser) {
        m_parsers.push_back(std::move(parser));
    }

    // add a new parser
    void add_parser(TokenParser* parser) {
        m_parsers.push_back(std::unique_ptr<TokenParser>(parser));
    }

    // make a parser and add it
    template<typename T, typename... Args>
    T &build_and_add_parser(Args&&... args) {
        auto parser = new T(std::forward<Args>(args)...);
        add_parser(parser);
        return *parser;
    }

    // returns the parsers
    const std::vector<std::unique_ptr<TokenParser>>& parsers() const {
        return m_parsers;
    }

    // Destructor
    virtual ~CombinatorParser() = default;

    // Get reference to the tokenizer
    Tokenizer& tokenizer() {
        return m_tokenizer_ref;
    }

    static ParserCallbackResult parser_callback(FileTokenStream& s, TokenParser& parser);
};

// The tokenizer class is used to parse a string into tokens
// It will orchestrate the parsing of the string, and will
// call the appropriate parser for each token
class Tokenizer {
private:
    // the default token type
    const char *m_default_type = "__default__";
    // the token parsers
    std::vector<std::unique_ptr<TokenParser>> m_representations;
    // the callbacks for each token parser
    std::unordered_map<const char *, ParserCallback> m_callbacks;

    // whether or not to parse each token as a word
    bool m_default_as_words = true;

    class TokenizerError : public std::exception {
    private:
        std::string m_message;
        unsigned int m_line, m_column;

    public:
        TokenizerError(size_t line, size_t column)
            : m_line(line), m_column(column)
        {
            m_message = "Tokenizer error at line " + std::to_string(line) + ", column " + std::to_string(column);
        }

        virtual const char* what() const noexcept override {
            return m_message.c_str();
        }

        unsigned int line() const {
            return m_line;
        }

        unsigned int column() const {
            return m_column;
        }
    };


public:
    // register a new token parser
    template<typename T>
    void register_parser_callback() {
        m_callbacks[TokenParser::GetParserTypeName<T>()] = T::parser_callback;
    }

    // create a new tokenizer
    // It will register the default token parsers
    // The default token parsers are:
    // - Keyword
    // - BeginEndPair
    Tokenizer()
    {
        register_parser_callback<TokenKeyword>();
        register_parser_callback<TokenBeginEndPair>();
        register_parser_callback<RegexParser>();
        register_parser_callback<CombinatorParser>();
    }

    // add a new token parser
    inline void add_parser(std::unique_ptr<TokenParser> &&parser) {
        m_representations.push_back(std::move(parser));
    }

    // add a new token parser
    inline void add_parser(TokenParser* parser) {
        m_representations.push_back(std::unique_ptr<TokenParser>(parser));
    }

    // make a parser and add it
    template<typename T, typename... Args>
    T &build_and_add_parser(Args&&... args) {
        auto parser = new T(std::forward<Args>(args)...);
        add_parser(parser);
        return *parser;
    }

    // add a new keyword token parser
    void add_keyword(const std::string& keyword, const char *type) {
        add_parser(new TokenKeyword(keyword, type));
    }

    // add a new begin/end pair token parser
    void add_begin_end_pair(const std::string& begin, const std::string& end,
                            bool keep_begin, bool keep_end,
                            const char *type) {
        add_parser(new TokenBeginEndPair(begin, end, keep_begin, keep_end, type));
    }

    // add a new regex token parser
    void add_regex(const std::string& regex, const char *type) {
        add_parser(new RegexParser(regex, type));
    }

    // set the default token type
    // This is the token type that will be used when a token
    // is not recognized by any of the token parsers and is parsed as an identifier
    void set_default_type(const char *type) {
        m_default_type = type;
    }

    // Set the default token parser as words
    // This means that the default token parser will parse the string as a sequence of words
    // A word is a sequence of characters that are not spaces
    // The default token parser will parse the string as a sequence of words
    // and will return a token for each word
    void set_default_as_words() {
        m_default_as_words = true;
    }

    // Set the default token parser as until parser match
    // This means that the default token parser will parse the string as a single token
    // until it finds a token that matches one of the token parsers
    // The default token parser will parse the string as a single token
    // until it finds a token that matches one of the token parsers
    // and will return a token for the string until the match
    void set_default_as_until_parser_match() {
        m_default_as_words = false;
    }


    // tokenize a string
    // This will parse the string and return a vector of tokens
    // Each token will have a type, a value, and a position
    // If the token is not recognized by any of the token parsers
    // it will be parsed as an identifier
    // The identifier is a token that does not match any of the token parsers
    // If the tokenize is not allowed to parse default identifiers it will throw an exception
    // with the position of the first unrecognized token
    std::vector<TokenInfo> tokenize(const std::string& str, bool allow_default_identifiers = true) {
        FileTokenStream stream(str);
        std::vector<TokenInfo> tokens;
        auto try_parsers = [&]() -> bool {
            for (auto& rep : m_representations) {
                auto token = m_callbacks[rep->parser_type()](stream, *rep);
                if (token != nullptr) {
                    tokens.push_back(*token);
                    return true;
                }
            }
            return false;
        };

        while (!stream.eof()) {
            stream.skip_whitespace();

            if (stream.eof()) {
                break;
            }

            if (try_parsers()) {
                continue;
            }

            if (!allow_default_identifiers) {
                throw TokenizerError(stream.line(), stream.column());
            }

            auto token = make_parser_callback_result(m_default_type, "", stream.line(), stream.column());

            // parse the default identifier as a word
            if (m_default_as_words) {
                while (!stream.eof() && !stream.is_whitespace()) {
                    token->value += stream.peek();
                    stream.next();
                }
                tokens.push_back(*token);
                continue;
            }

            // parse the default identifier as a sequence of characters until a parser matches
            while (!stream.eof() && !stream.is_whitespace()) {
                token->value += stream.peek();
                stream.next();

                if (try_parsers()) {
                    tokens.insert(tokens.end() - 1, *token); // insert the identifier before the token that was found
                    token = nullptr;
                    break;
                }
            }

            if (token != nullptr) {
                if (token->value.empty()) {
                    throw TokenizerError(stream.line(), stream.column());
                } else {
                    tokens.push_back(*token);
                }
            }
        }
        return tokens;
    }

    // returns the callbacks for each token parser
    const std::unordered_map<const char *, ParserCallback>& callbacks() const {
        return m_callbacks;
    }


};

ParserCallbackResult CombinatorParser::parser_callback(FileTokenStream& s, TokenParser& parser) {
    auto& combinator = dynamic_cast<CombinatorParser&>(parser);
    s.push_state();

    auto token = make_parser_callback_result(combinator.token_type(), "", s.line(), s.column());

    for (auto& p : combinator.parsers()) {
        auto info = combinator.m_tokenizer_ref.callbacks().at(p->parser_type())(s, *p);

        if (info == nullptr) {
            s.pop_state();
            return nullptr;
        }
        token->value += info->value;
    }
    s.pop_state(false);
    return token;
}

using Toks = Tokenizer;
template<typename T>
using ToksParser = TokenParserProxy<T>;

}
