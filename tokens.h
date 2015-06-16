#ifndef _MANANA_TOKENS_H
#define _MANANA_TOKENS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "debug.h"
#include "array.h"

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
char *tokens[61];

typedef enum {
	// special
	ILLEGAL, END, NEWLINE,
	// whitespace
	INDENT, DEDENT, SPACE,
	// boolean
	TRUE, FALSE, AND, OR, NOT, 
	// data types
	TYPEHASH, TYPELIST, TYPESTRING, TYPEINT, TYPENUMBER, TYPEBOOLEAN,
	// punctuation
	LPAREN, RPAREN, LBRACK, RBRACK, LBRACE, RBRACE, DOT,
	// tags
	TAG, TAGID, TAGCLASS, ATTRKEY, ATTREQ,
	// text
	TEXT, TAGTEXT,
	// keywords
	IF, ELIF, ELSE, CASE, WHEN, FOR, EACH, IN, IS, AS, WITH, ALIAS, UNALIAS, INCLUDE, 
	// loop control
	BREAK, CONTINUE,
	// conditions
	EQ, NEQ, GTE, LTE, GT, LT, MOD, EXISTS,
	// names
	ID,
	// values
	STR, ISTR, INT, NUMBER,
	// filters
	FILTER
} TokenType;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
typedef struct Token {
	TokenType type;
	int line, pos, length;
	void *value;
} Token;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
Token *Token_new(TokenType type, char *value, int length, int line, int pos);

void Token_print(Token *tok);

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#endif
