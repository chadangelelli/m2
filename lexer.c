#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tokens.h"
#include "lexer.h"
#include "debug.h"
#include "array.h"
#include "sds.h"

#define consume_current()\
	sds str = sdsnewlen(&buf->ch, 1);\
	Buffer_set_value(buf, str);\
	sdsfree(str);\
	Buffer_read(buf);

#define consume_while(x)\
	sds str = sdsempty();\
	int consume_counter = 0;\
	while (x) {\
		if (consume_counter > MANANA_MAX_TOKEN_LENGTH) {\
			sdsfree(str);\
			printf("Maximum value length of %d characters exceeded!\n", MANANA_MAX_TOKEN_LENGTH);\
			exit(1);\
		}\
		str = sdscatlen(str, &buf->ch, 1);\
		Buffer_read(buf);\
		consume_counter++;\
	}\
	Buffer_set_value(buf, str);\
	sdsfree(str);

#define consume_chars(x)\
	if (x > MANANA_MAX_TOKEN_LENGTH) {\
		printf("Number provided to consume_chars is greater than allowed max of %d\n", MANANA_MAX_TOKEN_LENGTH);\
		exit(1);\
	}\
	sds str = sdsempty();\
	int consume_counter = 0;\
	while (consume_counter < x) {\
		str = sdscatlen(str, &buf->ch, 1);\
		Buffer_read(buf);\
		consume_counter++;\
	}\
	Buffer_set_value(buf, str);\
	sdsfree(str);

#define str_is(a)\
	(strcmp(buf->value, a) == 0)

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_initial(Buffer *buf) {
	printf("(%d:%d) lex_initial\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	// Check for indent.
	if (buf->ch == ' ' || buf->ch == '\t') {
		lex_indent(buf); 
	}
	// Check for tag.
	else if (isalpha(buf->ch) || buf->ch == '.' || buf->ch == '#') {
		lex_tag(buf);
	}
	// Check for logic.
	else if (buf->ch == '-') {
		lex_logic(buf);
	}
	// Check for filter.
	else if (buf->ch == ':' && isalpha(buf->next)) {
		lex_filter(buf);
	}
	// Check for name.
	else if (buf->ch == '@' && buf->next == '{') {
		lex_name(buf);
	}
	// Check for comment.
	else if (buf->ch == '"' && buf->next == '"' && buf->src[buf->pos+1] == '"') {
		lex_comment(buf);
	}
	// Check for newline.
	else if (buf->ch == '\n') {
		buf->line++;
		// ignore newline character itself by advancing buffer.
		Buffer_jump(buf, 1); 
		lex_indent(buf);
	}
	// Check for EOF.
	else if (buf->ch == '\0') {
		lex_eof(buf);
		// Set buf->pos out of bounds to break tokenize function.
		buf->pos++;
	}
	// No match. exit.
	else {
		printf("\tILLEGAL (%d:%d): %c\n", buf->line, buf->pos, buf->ch);
		exit(1);
	}
} // end lex_initial()

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_indent(Buffer *buf) {
	printf("(%d:%d) lex_indent\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	// Allow for "empty indents"/dedents at level zero.
	buf->value = sdsempty();
	buf->length = 0;

	consume_while(buf->ch == ' ' || buf->ch == '\t');

	// Ignore blank lines.
	if (buf->ch == '\n')
		return;

	// Level is based on length of buf->value.
	// buf->length is set in consume_while macro.
	int level = buf->length;

	// Explicitly set value to empty string, we only need level now.
	buf->value = sdsempty();

	// Set appropriate indet level by increasing/descreasing stack.
	if (level > INDENT_HIGHEST) {
		IndentStack_increase(buf->indent_stack, level);
		buf->indent_level = buf->length = level;
		emit(buf, INDENT);
	}
	else if (level < INDENT_HIGHEST) {
		while (INDENT_HIGHEST > level) {
			IndentStack_decrease(buf->indent_stack);	
			buf->indent_level = INDENT_HIGHEST;
			buf->length = buf->indent_level;
			emit(buf, DEDENT);
		}
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_tag(Buffer *buf) {
	printf("(%d:%d) lex_tag\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	sds tag_name = sdsempty();

	// Check for div shorthand.
	if (buf->ch == '.' || buf->ch == '#') {
		sds str = sdsnew("div"); 
		Buffer_set_value(buf, str); 
		sdsfree(str); 
		emit(buf, TAG);

	// Consume tag.
	} else {
		consume_while(isalpha(buf->ch) || isdigit(buf->ch));
		tag_name = sdsnew(buf->value);
		emit(buf, TAG);
	}

	Buffer_read_ignore_whitespace(buf);

	// Check to see if we have IDs, CSS classes, attributes, or text.
	while (buf->ch != '\n' && buf->ch != '\0') {
		// Ignore whitespace.
		Buffer_read_ignore_whitespace(buf);

		// Ignoring trailing whitespace can land us on newline or EOF.
		// Break loop if this is true.
		if (buf->ch == '\n' || buf->ch == '\0')
			break;

		// Set start point in buffer.
		Buffer_set_start(buf);

		// Match optional tag tokens for ID, class, attributes, or text.
		switch (buf->ch) {
		case '#': // Check for ID shorthand.
			lex_tag_id(buf);
			break;
		case '.': // Check for CSS class shorthand.
			lex_tag_class(buf);
			break;
		case '(': // Check for attributes, delimited by left paren.
			lex_tag_attrs(buf);
			break;
		case '=':
			lex_tag_inline_vars(buf);
			break;
		case '-': // Check for src/href shorthand
			if (buf->next == '>') {
				Buffer_jump(buf, 2);

				Buffer_read_ignore_whitespace(buf);

				if (strcmp(tag_name, "a") == 0)
					Buffer_set_value(buf, sdsnew("href"));
				else
					Buffer_set_value(buf, sdsnew("src"));
				emit(buf, ATTRKEY);

				Buffer_set_value(buf, sdsnew("="));
				emit(buf, ATTREQ);

				if (buf->ch == '"' || buf->ch == '\'') {
					lex_str(buf);
				}
			}
			break;
		default: // We have text.
			lex_tag_text(buf);
			break;
		}
	}
}; // end lex_tag()

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_tag_id(Buffer *buf) {
	printf("(%d:%d) lex_tag_id\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	// First character to this function will always be "#", advance buffer.
	Buffer_jump(buf, 1);

	consume_while(isalpha(buf->ch) || isdigit(buf->ch) || buf->ch == '-' || buf->ch == '_');

	emit(buf, TAGID);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_tag_class(Buffer *buf) {
	printf("(%d:%d) lex_tag_class\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	// First character to this function will always be ".", advance buffer.
	Buffer_jump(buf, 1);

	consume_while(isalpha(buf->ch) || isdigit(buf->ch) || buf->ch == '-' || buf->ch == '_');

	emit(buf, TAGCLASS);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_tag_attrs(Buffer *buf) {
	printf("(%d:%d) lex_tag_attrs\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	// First character to this function will always be "(", advance buffer.
	Buffer_jump(buf, 1);

	while (buf->ch != ')' && buf->ch != '\0') {
		// Ignore whitespace inside parens.
		Buffer_read_ignore_whitespace(buf);

		// Check for attribute key.
		if (isalpha(buf->ch)) {
			consume_while(isalpha(buf->ch) || isdigit(buf->ch) || buf->ch == '-' || buf->ch == '_');
			emit(buf, ATTRKEY);
		}
		// Check for data attribute key shorthand.
		else if (buf->ch == '*') {
			Buffer_jump(buf, 1);

			sds str = sdsnew("data-");
			while (isalpha(buf->ch) || isdigit(buf->ch) || buf->ch == '-' || buf->ch == '_') {
				str = sdscatlen(str, &buf->ch, 1);
				Buffer_read(buf);
			}

			Buffer_set_value(buf, str);
			sdsfree(str);

			emit(buf, ATTRKEY);
		} 
		// Check for equals.
		else if (buf->ch == '=') {
			consume_current();
			emit(buf, ATTREQ);
		}
		// Check for string.
		else if (buf->ch == '"' || buf->ch == '\'') {
			lex_str(buf);
		}
		// Check for EOF.
		else if (buf->ch == '\0') {
			printf("Unclosed tag attributes!\n");
			exit(1);
		}
	}

	Buffer_jump(buf, 1); // Advance buffer past closing ")"
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_tag_inline_vars(Buffer *buf) {
	printf("(%d:%d) lex_tag_inline_vars\n", buf->line, buf->pos);

	Buffer_set_start(buf);

	Buffer_jump(buf, 1); // Jump past initial "=" char. 
	Buffer_read_ignore_whitespace(buf);

	if (isalpha(buf->ch)) {
		lex_name_no_delim(buf); 
		Buffer_read_ignore_whitespace(buf);
	}

	if (buf->ch != '\n' && buf->ch != '\0') {
		consume_while(buf->ch != '\n' && buf->ch != '\0');
		emit(buf, ILLEGAL);
		printf("Invalid inline variable.\n");
		exit(1);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_tag_text(Buffer *buf) {
	printf("(%d:%d) lex_tag_text\n", buf->line, buf->pos);

	Buffer_set_start(buf);
	Buffer_read_ignore_whitespace(buf);

	while (buf->ch != '\n' && buf->ch != '\0') {
		// Lex raw text.
		consume_while(
			(buf->ch != '@' && buf->next != '{') 
			&& buf->ch != '\n' 
			&& buf->ch != '\0'
		);
		emit(buf, TAGTEXT);

		// Lex interpolated name.
		if (buf->ch == '@' && buf->next == '{')
			lex_name(buf);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_text(Buffer *buf) {
	printf("(%d:%d) lex_text\n", buf->line, buf->pos);

	Buffer_set_start(buf);
	Buffer_read_ignore_whitespace(buf);

	while (buf->ch != '\n' && buf->ch != '\0') {
		// Lex raw text.
		consume_while(
			(buf->ch != '@' && buf->next != '{') 
			&& buf->ch != '\n' 
			&& buf->ch != '\0'
		);
		emit(buf, TEXT);

		// Lex interpolated name.
		if (buf->ch == '@' && buf->next == '{')
			lex_name(buf);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_name(Buffer *buf) {
	printf("(%d:%d) lex_name\n", buf->line, buf->pos);

	if (buf->ch != '@') {
		printf("Invalid beginning character \"%c\" for name.", buf->ch);
		exit(1);
	}

	Buffer_jump(buf, 2); // Ignore "@{" delimiter. 
	Buffer_read_ignore_whitespace(buf);
	Buffer_set_start(buf);

	while (buf->ch != '}' && buf->ch != '\0') {
		Buffer_read_ignore_whitespace(buf);

		if (isalpha(buf->ch) || buf->ch == '_') {
			consume_while(isalpha(buf->ch) || buf->ch == '_' || isdigit(buf->ch));
			emit(buf, ID);
		}
		else if (buf->ch == '.') {
			consume_current();
			emit(buf, DOT);
		}
		else if (buf->ch == '[') {
			consume_current();
			emit(buf, LBRACK);
		}
		else if (buf->ch == ']') {
			consume_current();
			emit(buf, RBRACK);
		}
		else if (isdigit(buf->ch)) {
			consume_while(isdigit(buf->ch));
			emit(buf, INT);
		}  
		else if (buf->ch == '\n') {
			printf("Invalid name! Newline found inside name declaration.\n");
			exit(1);
		}
	}

	Buffer_jump(buf, 1); // Move buffer after closing "}" delimiter.
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_name_no_delim(Buffer *buf) {
	printf("(%d:%d) lex_name_no_delim\n", buf->line, buf->pos);

	Buffer_read_ignore_whitespace(buf);
	Buffer_set_start(buf);

	if (!isalpha(buf->ch)) {
		printf("Invalid beginning character \"%c\" for name.", buf->ch);
		exit(1);
	}

	while (buf->ch != '\n' && buf->ch != '\0') {
		if (isalpha(buf->ch) || buf->ch == '_') {
			consume_while(isalpha(buf->ch) || buf->ch == '_' || isdigit(buf->ch));
			emit(buf, ID);
		}
		else if (buf->ch == '.') {
			consume_current();
			emit(buf, DOT);
		}
		else if (buf->ch == '[') {
			consume_current();
			emit(buf, LBRACK);
		}
		else if (buf->ch == ']') {
			consume_current();
			emit(buf, RBRACK);
		}
		else if (isdigit(buf->ch)) {
			consume_while(isdigit(buf->ch));
			emit(buf, INT);
		} else {
			break;
		}
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_str(Buffer *buf) {
	printf("(%d:%d) lex_str\n", buf->line, buf->pos);

	// Store single vs double quote to use as delimiter.
	char quote = buf->ch;

	Buffer_jump(buf, 1); // Advance past opening quote.
	Buffer_set_start(buf);

	sds str = sdsempty();
	int is_interpolated = 0;
	char prev;

	for (;;) {
		// Check for name.
		if (buf->ch == '@' && buf->next == '{') {
			is_interpolated = 1;

			Buffer_set_value(buf, str); 
			emit(buf, ISTR);
			str = sdsempty();

			lex_name(buf);
		}
		// Check for escaped quote.
		else if (buf->ch == '\\' && buf->next == quote) {
			str = sdscatlen(str, &buf->ch, 1);
			str = sdscatlen(str, &buf->next, 1);

			Buffer_jump(buf, 2);
		}
		// Check for end quote.
		else if (buf->ch == quote) {
			Buffer_set_value(buf, str);

			if (is_interpolated)
				emit(buf, ISTR);
			else
				emit(buf, STR);

			break;

		}
		// Check for EOF.
		else if (buf->ch == '\0') {
			printf("Unclosed string!");
			exit(1);
		}
		// Continue consuming string.
		else {
			str = sdscatlen(str, &buf->ch, 1);
			Buffer_read(buf);
		}
	}

	sdsfree(str);

	// Advance past closing quote.
	if (buf->ch == quote)
		Buffer_jump(buf, 1);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_comment(Buffer *buf) {
	printf("(%d:%d) lex_comment\n", buf->line, buf->pos);

	Buffer_set_start(buf);
	Buffer_jump(buf, 2); // advance buffer past (""")

	for (;;) {
		if (buf->ch == '"' && buf->next == '"' && buf->src[buf->pos+1] == '"')
			break;
		Buffer_read(buf);
	}

	Buffer_jump(buf, 3); // advance buffer past closing (""")
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_logic(Buffer *buf) {
	printf("(%d:%d) lex_logic\n", buf->line, buf->pos);

	Buffer_jump(buf, 1);
	Buffer_set_start(buf);

	consume_while(isalpha(buf->ch));

	// redirect to appropriate substate.
	if      (str_is("if"     )) { emit(buf, IF)     ; lex_if(buf)     ; }
	else if (str_is("elif"   )) { emit(buf, ELIF)   ; lex_if(buf)     ; }
	else if (str_is("else"   )) { emit(buf, ELSE)                     ; }
	else if (str_is("case"   )) { emit(buf, CASE)   ; lex_case(buf)   ; }
	else if (str_is("when"   )) { emit(buf, WHEN)   ; lex_when(buf)   ; }
	else if (str_is("for"    )) { emit(buf, FOR)    ; lex_for(buf)    ; }
	else if (str_is("each"   )) { emit(buf, EACH)   ; lex_each(buf)   ; }
	else if (str_is("alias"  )) { emit(buf, ALIAS)  ; lex_alias(buf)  ; }
	else if (str_is("unalias")) { emit(buf, UNALIAS); lex_unalias(buf); }
	else if (str_is("include")) { emit(buf, INCLUDE); lex_include(buf); }
	else if (str_is("with"   )) { emit(buf, WITH)   ; lex_with(buf)   ; }
	else                        { emit(buf, ILLEGAL);                   }
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_if(Buffer *buf) { 
	printf("(%d:%d) lex_if\n", buf->line, buf->pos);

	while (buf->ch != '\n' && buf->ch != '\0') {
		Buffer_read_ignore_whitespace(buf);
		Buffer_set_start(buf);

		// Check for type, keyword, or name.
		if (isalpha(buf->ch)) {
			consume_while(isalpha(buf->ch));

			// Check for type.
			if (buf->value[0] >= 'A' && buf->value[0] <= 'Z') {
				if      (str_is("Hash"))    emit(buf, TYPEHASH);
				else if (str_is("List"))    emit(buf, TYPELIST);
				else if (str_is("String"))  emit(buf, TYPESTRING);
				else if (str_is("Int"))     emit(buf, TYPEINT);
				else if (str_is("Number"))  emit(buf, TYPENUMBER);
				else if (str_is("Boolean")) emit(buf, TYPEBOOLEAN);
				// No match, assume name.
				else {
					Buffer_unread(buf);
					lex_name_no_delim(buf);
				}
			// Check for keyword.
			} else {
				if (str_is("in"))
					emit(buf, IN);
				else if (str_is("is"))
					emit(buf, IS);
				else if (str_is("not"))
					emit(buf, NOT);
				else if (str_is("exists"))
					emit(buf, EXISTS);
				// No match, assume name.
				else {
					Buffer_unread(buf);
					lex_name_no_delim(buf);
				}
			}
		}
		// Check for number.
		else if (isdigit(buf->ch)) {
			consume_while(isdigit(buf->ch) || buf->ch == '.');
			emit(buf, NUMBER);
		}
		// Check for condition.
		else {
			switch (buf->ch) {
			case '=': // "=="
				if (buf->next == '=') {
					consume_chars(2);
					emit(buf, EQ);
				} else {
					printf("Invalid symbol \"=\" found.\n");
					exit(1);
				}
				break;
			case '!': // "!="
				if (buf->next == '=') {
					consume_chars(2);
					emit(buf, NEQ);
				} else {
					printf("Invalid symbol \"!\" found.\n");
					exit(1);
				}
				break;
			case '>': // ">" and ">="
				if (buf->next == '=') {
					consume_chars(2);
					emit(buf, GTE);
				} else {
					consume_current();
					emit(buf, GT);
				}
				break;
			case '<': // "<" and "<="
				if (buf->next == '=') {
					consume_chars(2);
					emit(buf, LTE);
				} else {
					consume_current();
					emit(buf, LT);
				}
				break;
			case '%': // "%" 
				Buffer_set_value(buf, "%");
				emit(buf, MOD);
				Buffer_read(buf);
				break;
			default: 
				printf("Invalid character \"%c\" in if-statment\n", buf->ch);
				exit(1);
			}
		}
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_for(Buffer *buf) { 
	printf("(%d:%d) lex_for\n", buf->line, buf->pos);

	Buffer_read_ignore_whitespace(buf);
	Buffer_set_start(buf);

	if (isalpha(buf->ch)) {
		lex_name_no_delim(buf);

		Buffer_read_ignore_whitespace(buf);

		if (isalpha(buf->ch)) {
			lex_keyword(buf);

			Buffer_read_ignore_whitespace(buf);

			if (isalpha(buf->ch)) {
				lex_name_no_delim(buf);
			}
		}
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_each(Buffer *buf) { 
	printf("(%d:%d) lex_each\n", buf->line, buf->pos);

	Buffer_read_ignore_whitespace(buf);
	Buffer_set_start(buf);

	if (isalpha(buf->ch))
		lex_name_no_delim(buf);

	Buffer_read_ignore_whitespace(buf);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_case(Buffer *buf) { 
	printf("(%d:%d) lex_case\n", buf->line, buf->pos);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_when(Buffer *buf) { 
	printf("(%d:%d) lex_when\n", buf->line, buf->pos);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_with(Buffer *buf) { 
	printf("(%d:%d) lex_with\n", buf->line, buf->pos);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_alias(Buffer *buf) {
	printf("(%d:%d) lex_alias\n", buf->line, buf->pos);

	Buffer_read_ignore_whitespace(buf);
	if (isalpha(buf->ch)) {
		lex_name_no_delim(buf);

		Buffer_read_ignore_whitespace(buf);
		if (isalpha(buf->ch)) {
			lex_keyword(buf); 
			
			Buffer_read_ignore_whitespace(buf);
			if (isalpha(buf->ch))
				lex_id(buf);
		}
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_unalias(Buffer *buf) { 
	printf("(%d:%d) lex_unalias\n", buf->line, buf->pos);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_include(Buffer *buf) {
	printf("(%d:%d) lex_include\n", buf->line, buf->pos);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_filter(Buffer *buf) {
	printf("(%d:%d) lex_filter\n", buf->line, buf->pos);

	Buffer_jump(buf, 1); // Advance past initial ":"

	consume_while(isalpha(buf->ch) || isdigit(buf->ch) || buf->ch == '_');
	emit(buf, FILTER);

	Buffer_read_ignore_whitespace(buf);

	// Check for block.
	if (buf->ch == '\n') {
		buf->line++;
		Buffer_jump(buf, 1); // ignore newline character. 
		Buffer_set_start(buf);

		// Store indent level for block to compare.
		int filter_base_indent;
		consume_while(buf->ch == ' ' || buf->ch == '\t');
		filter_base_indent = buf->length;

		// Set buffer back before initial indent.
		Buffer_unread(buf);

		// Get lines.
		while (buf->ch != '\0') {
			Buffer_set_start(buf);

			// Check for next line.
			if (buf->ch == '\n') {
				buf->line++;
				Buffer_read(buf);
				Buffer_set_start(buf);
			}

			// Get line indent.
			consume_while(buf->ch == ' ' || buf->ch == '\t');

			if (buf->length > filter_base_indent) {
				sdsrange(buf->value, filter_base_indent, -1);
				Buffer_set_value(buf, buf->value);
				emit(buf, TEXT);
			} 
			else if (buf->length < filter_base_indent) {
				buf->indent_level = INDENT_HIGHEST;
				buf->length = buf->indent_level;
				emit(buf, DEDENT);
				return;
			}

			// Get line text.
			Buffer_set_start(buf);
			lex_text(buf);
		}
	}
	// Check for EOF.
	else if (buf->ch == '\0') {
		return;
	}
	// Consume text. 
	else {
		lex_text(buf);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_keyword(Buffer *buf) {
	printf("(%d:%d) lex_keyword\n", buf->line, buf->pos);

	consume_while(isalpha(buf->ch));

	if      (str_is("in")) emit(buf, IN);
	else if (str_is("as")) emit(buf, AS);
	else if (str_is("is")) emit(buf, IS);
	else                   emit(buf, ILLEGAL);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_id(Buffer *buf) {
	printf("(%d:%d) lex_id\n", buf->line, buf->pos);

	consume_while(isalpha(buf->ch) || isdigit(buf->ch) || buf->ch == '_');

	Buffer_set_value(buf, str);

	emit(buf, ID);
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
void lex_eof(Buffer *buf) {
	printf("(%d:%d) lex_eof\n", buf->line, buf->pos);

	int initial_indent;
	if (buf->stream[0]->type == INDENT) 
		initial_indent = buf->stream[0]->length;
	else
		initial_indent = 0;

	buf->value = "";
	buf->value = sdsempty();

	while (INDENT_HIGHEST > initial_indent) {
		IndentStack_decrease(buf->indent_stack);
		buf->indent_level = INDENT_HIGHEST;
		buf->length = INDENT_HIGHEST;
		emit(buf, DEDENT);
	}
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
int tokenize(char *src, int src_size) {
	Buffer *buf = Buffer_create(src, src_size);

	while (buf->pos <= buf->src_size)
		lex_initial(buf);

	//Buffer_print_tokens(buf);
	Buffer_print_stream(buf);

	Buffer_destroy(buf);

	return 1;
}

// . .. ... .. . .. ... .. . .. ... .. . .. ... .. . .. ... .. .
int main(int argc, char *argv[]) {
	puts("\n");

	FILE *f = fopen("examples/0.basics.manana", "rb");
	//FILE *f = fopen("examples/1.logic.manana", "rb");

	fseek(f, 0, SEEK_END);
	long src_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *src = malloc(src_size + 1);
	fread(src, src_size, 1, f);
	src[src_size] = '\0';
	fclose(f);

	tokenize(src, src_size);

	return 0;
}
