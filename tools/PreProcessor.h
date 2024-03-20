#ifndef PREPROCESSOR_H_
#define PREPROCESSOR_H_

namespace zorya {
	enum class TKTP {
		END_OF_FILE,
		HASH,
		IDENTIFIER,
		OPEN_PARENTHESIS,
		CLOSE_PARENTHESIS,
		OPEN_SQUARE_BRACKET,
		CLOSE_SQUARE_BRACKET,
		OPEN_BRACES,
		CLOSE_BRACES,
		EQUALS,
		COMMA,
		COLON,
		SEMICOLON,
		UNDEFINED
	};


}

struct Token {
	zorya::TKTP type;
	char* text;
	size_t textLength;
};

struct Tokenizer {
	char* at;
};


#endif // !PREPROCESSOR_H_
