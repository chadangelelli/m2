#include <stdio.h>
#include <stdlib.h> 
#include <assert.h>
#include <string.h>
#include "tokens.h"

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. . 
int stream_index = 0;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. . 
// Lookup array that matches TokenType enum in tokens.h
char *tokens[] = {
    // special
    "ILLEGAL", "END", "NEWLINE",
    // whitespace
    "INDENT", "DEDENT", "SPACE",
    // boolean
    "TRUE", "FALSE", "AND", "OR", "NOT",
    // data types
    "TYPEHASH", "TYPELIST", "TYPESTRING", "TYPEINT", "TYPENUMBER", "TYPEBOOLEAN",
    // punctuation
    "LPAREN", "RPAREN", "LBRACK", "RBRACK", "LBRACE", "RBRACE", "DOT",
    // tags
    "TAG", "TAGID", "TAGCLASS", "ATTRKEY", "ATTREQ",
    // text
    "TEXT", "TAGTEXT",
    // keywords
	"IF", "ELIF", "ELSE", "CASE", "WHEN", "FOR", "EACH", "IN", "IS", "AS", "WITH", "ALIAS", "UNALIAS", "INCLUDE",
    // loop control
    "BREAK", "CONTINUE",
    // conditions
    "EQ", "NEQ", "GTE", "LTE", "GT", "LT", "MOD", "EXISTS",
	// names
	"ID",
	// values
	"STR", "ISTR", "INT", "NUMBER",
	// filters
	"FILTER"
};

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. . 
Token *Token_new(TokenType type, char *value, int length, int line, int pos) 
{
	Token *tok = malloc(sizeof(Token));

	tok->type = type;
	tok->value = strdup(value);
	tok->length = length;
	tok->line = line;
	tok->pos = pos;

	return tok;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. . 
void Token_print(Token *tok) 
{
	printf("Token: %s\n", tokens[tok->type]);
	printf("\ttype: %s (%d)\n", tokens[tok->type], tok->type);
	printf("\tline: %d\n", tok->line);
	printf("\tpos: %d\n", tok->pos);
	printf("\tvalue: |%s|\n", tok->value);
	printf("\tlength: %d\n", tok->length);
	puts("\n");
}
