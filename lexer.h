#ifndef _MANANA_LEXER_H
#define _MANANA_LEXER_H

#include "array.h"
#include "indentation.h"
#include "tokens.h"
#include "sds.h"

#define INDENT_HIGHEST buf->indent_stack->first->value
#define INDENT_LOWEST buf->indent_stack->last->value

static const int MANANA_MAX_TOKEN_LENGTH = 1000;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
typedef struct Buffer {
	IndentStack *indent_stack;
	Token *stream[1000];
	int line, start, pos, length, indent_level, stream_index;
	char ch, next, *src, *value;
	long src_size;
} Buffer;

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
Buffer *Buffer_create(char *src, long src_size) {
	Buffer *buf = malloc(sizeof(Buffer));

	buf->src = src;
	buf->src_size = src_size;

	buf->line = 1;
	buf->start = 0;
	buf->pos = 0; 
	buf->length = 0;
	buf->value = "";
	buf->ch = buf->src[buf->pos];
	buf->next = buf->src[buf->pos+1];

	buf->indent_stack = IndentStack_create();
	IndentStack_increase(buf->indent_stack, 0); // Initialize Indent Stack to zero
	buf->indent_level = 0;

	return buf;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_read(Buffer *buf) {
	++buf->pos;
	buf->ch = buf->src[buf->pos];
	buf->next = buf->src[buf->pos+1];
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_read_ignore_whitespace(Buffer *buf) {
	while (buf->ch == ' ' || buf->ch == '\t')
		Buffer_read(buf);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_unread(Buffer *buf) {
	buf->pos = buf->start;
	buf->ch = buf->src[buf->pos];
	buf->next = buf->src[buf->pos+1];
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline char Buffer_peek(Buffer *buf, int inc) {
	return buf->src[buf->pos+inc];
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_jump(Buffer *buf, int inc) {
	buf->pos = buf->pos + inc;
	buf->ch = buf->src[buf->pos];
	buf->next = buf->src[buf->pos+1];
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_set_start(Buffer *buf) {
	buf->start = buf->pos;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_set_value(Buffer *buf, char *str) {
	buf->value = str;
	buf->length = sdslen(str);

	if (buf->length > MANANA_MAX_TOKEN_LENGTH) {
		printf("Maximum value length of %d characters exceeded!\n", MANANA_MAX_TOKEN_LENGTH);
		exit(1);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void Buffer_destroy(Buffer *buf) {
	if (buf->indent_stack)
		IndentStack_destroy(buf->indent_stack);
	free(buf);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Buffer_print(Buffer *buf) {
    puts("\n  BUFFER ###########################################################");
	printf("\tsrc_size: %ld\n", buf->src_size);
	printf("\tline: %d\n", buf->line);
	printf("\tstart: %d\n", buf->start);
	printf("\tpos: %d\n", buf->pos);
	printf("\tch: %c\n", buf->ch);
	printf("\tnext: %c\n", buf->next);
	printf("\tvalue: %s\n", buf->value);
	printf("\tvalue length: %d\n", buf->length);
    puts("  /BUFFER ###########################################################");
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Buffer_print_src(Buffer *buf) {
    puts("\n  SOURCE ###########################################################");
	printf("%s", buf->src);
    puts("  /SOURCE ###########################################################");
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Buffer_print_tokens(Buffer *buf) {
    puts("\n  TOKENS ###########################################################");
    int i = 0;
    Token *tok;
    while ((tok = buf->stream[i])) {
        Token_print(tok);
        i++;
    }
    puts("  /TOKENS ###########################################################");
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Buffer_print_stream(Buffer *buf) {
    puts("\n  STREAM ###########################################################");
    int i = 0;
    Token *tok;
    while ((tok = buf->stream[i])) {
    	if (tok->type == INDENT || tok->type == DEDENT) {
			printf("\t(%s\t %d)\n", tokens[tok->type], tok->length);
		} else {
			printf("\t(%s\t %s)\n", tokens[tok->type], tok->value);
		}
        i++;
    }
    puts("\n  /STREAM ###########################################################");
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Buffer_print_loc(Buffer *buf) {
	printf("\tstart: %d\n\tpos: %d\n", buf->start, buf->pos);
	printf("\tch: %c\n", buf->ch);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void Buffer_print_value(Buffer *buf) {
	printf("\tvalue %s\n", buf->value);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_initial(Buffer *buf);
void lex_comment(Buffer *buf);
void lex_indent(Buffer *buf);
void lex_tag(Buffer *buf);
void lex_tag_id(Buffer *buf);
void lex_tag_class(Buffer *buf);
void lex_tag_attrs(Buffer *buf);
void lex_tag_inline_vars(Buffer *buf);
void lex_tag_text(Buffer *buf);
void lex_text(Buffer *buf);
void lex_str(Buffer *buf);
void lex_int(Buffer *buf);
void lex_number(Buffer *buf);
void lex_name(Buffer *buf);
void lex_name_no_delim(Buffer *buf);
void lex_logic(Buffer *buf);
void lex_if(Buffer *buf);
void lex_for(Buffer *buf);
void lex_each(Buffer *buf);
void lex_case(Buffer *buf);
void lex_when(Buffer *buf);
void lex_with(Buffer *buf);
void lex_alias(Buffer *buf);
void lex_unalias(Buffer *buf);
void lex_include(Buffer *buf);
void lex_filter(Buffer *buf);
void lex_id(Buffer *buf);
void lex_keyword(Buffer *buf);
void lex_eof(Buffer *buf);

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
static inline void emit(Buffer *buf, TokenType type) {
	Token *tok = Token_new(type, buf->value, buf->length, buf->line, buf->start);
	buf->stream[buf->stream_index] = tok;
	buf->stream_index++;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
#endif
