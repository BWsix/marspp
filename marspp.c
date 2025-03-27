#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdbool.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void die(char *msg) { printf("%s\n", msg); exit(1); }

typedef struct {
    char *string;
    int length;
} sv_t;

typedef struct {
    // lexer variables
    char *input_stream;
    char *eof;
    char *parse_point;
    char *string_storage;
    int string_storage_len;

    // lexer parse location
    char *line_start;
    char *where_firstchar;
    char *where_lastchar;

    // lexer token variables
    long token;
    long int_number;
    char *string;
    int string_len;
} lexer_t;

enum {
    ALEX_eof = 256,
    ALEX_parse_error,
    ALEX_ident,
    ALEX_register,
    ALEX_directive,
    ALEX_intlit,
    ALEX_string,
};

enum {
    APAR_others,
    APAR_text,
    APAR_data,
};

void lexer_print_token(lexer_t *lexer)
{
    switch (lexer->token) {
    case ALEX_ident:     printf("ident(%s)", lexer->string); break;
    case ALEX_register:  printf("register(%s)", lexer->string); break;
    case ALEX_directive: printf("directive(%s)", lexer->string); break;
    case ALEX_intlit:    printf("intlit(%d)", lexer->int_number); break;
    case ALEX_string:    printf("string(\"%s\")", lexer->string); break;
    default: {
        if (lexer->token >= 0 && lexer->token < 256) {
            printf("char(%c)", (int)lexer->token);
        } else {
            printf("UNKNOWN TOKEN(%ld)", lexer->token);
            die("");
        }
    } break;
    }
}

int _lexer_token(lexer_t *lexer, int token, char *start, char *end)
{
    lexer->token = token;
    lexer->where_firstchar = start;
    lexer->where_lastchar = end;
    lexer->parse_point = end + 1;
    return 1;
}

int _lexer_iswhite(int x)
{
    return x == ' ' || x == '\t' || x == '\r' || x == '\n' || x == '\f';
}

int _lexer_isnewline(int x)
{
    return x == '\r' || x == '\n';
}

int _lexer_eof(lexer_t *lexer)
{
    lexer->token = ALEX_eof;
    return 0;
}

void lexer_init(lexer_t *lexer, const char *input_stream, const char *input_stream_end, char *string_storage, size_t store_length)
{
    *lexer = (lexer_t){0};
    lexer->input_stream = (char *)input_stream;
    lexer->eof = (char *)input_stream_end;
    lexer->parse_point = (char *)input_stream;
    lexer->string_storage = string_storage;
    lexer->string_storage_len = store_length;
}

// returns negative number if error, or a char
int _lexer_parse_char(char *p, char **q)
{
    if (*p == '\\') {
        *q = p + 2; // tentatively guess we'll parse two characters
        switch(p[1]) {
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        case 't': return '\t';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case '0': return '\0';
        case 'x': case 'X': return -1;
        case 'u': return -1;
        default: break;
        }
   }
   *q = p + 1;
   return (unsigned char) *p;
}

int _lexer_parse_string(lexer_t *lexer, char *p, int type)
{
    char *start = p;
    char delim = *p++; // grab the delimeter (" or ') for later matching
    char *out = lexer->string_storage;
    char *outend = lexer->string_storage + lexer->string_storage_len;
    while (*p != delim) {
        int n;
        if (*p == '\\') {
            char *q;
            n = _lexer_parse_char(p, &q);
            if (n < 0) return _lexer_token(lexer, ALEX_parse_error, start, q);
            p = q;
        } else {
            n = (unsigned char) *p++;
        }
        if (out + 1 > outend) {
            return _lexer_token(lexer, ALEX_parse_error, start, p);
        }
        *out++ = (char) n;
    }
    *out = 0;
    lexer->string = lexer->string_storage;
    lexer->string_len = (int) (out - lexer->string_storage);
    return _lexer_token(lexer, type, start, p);
}

// returns non-zero if a token is parsed, or 0 if at EOF
int lexer_get_token(lexer_t *lexer)
{
    char *p = lexer->parse_point;

    // skip whitespace and comments
    for (;;) {
        while (p != lexer->eof && _lexer_iswhite(*p)) {
            if (_lexer_isnewline(*p)) {
                lexer->line_start = p + 1;
            }
            p += 1;
        }

        if (p != lexer->eof && p[0] == '#') {
            while (p != lexer->eof && *p != '\r' && *p != '\n') {
                ++p;
            }
            continue;
        }

        break;
    }

    if (p == lexer->eof) {
        return _lexer_eof(lexer);
    }

    switch (*p) {
    default: {
        if (
            (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_'
            || (unsigned char) *p >= 128 // >= 128 is UTF8 char
            || *p == '$' // register
            || *p == '.' // directive
        ) {
            int n = 0;
            lexer->string = lexer->string_storage;
            do {
                if (n + 1 >= lexer->string_storage_len) {
                    return _lexer_token(lexer, ALEX_parse_error, p, p + n);
                }
                lexer->string[n] = p[n];
                n += 1;
            } while (
                (p[n] >= 'a' && p[n] <= 'z') || (p[n] >= 'A' && p[n] <= 'Z') || p[n] == '_'
                || (p[n] >= '0' && p[n] <= '9') // allow digits in middle of identifier
                || (unsigned char) p[n] >= 128 // >= 128 is UTF8 char
            );
            lexer->string[n] = 0;
            lexer->string_len = n;

            int type = ALEX_ident;
            if (*p == '$') type = ALEX_register;
            if (*p == '.') type = ALEX_directive;
            return _lexer_token(lexer, type, p, p + n - 1);
        }

        if (*p == 0) {
            return _lexer_eof(lexer);
        }
    
        return _lexer_token(lexer, *p, p, p);
    } break;

    case '"': return _lexer_parse_string(lexer, p, ALEX_string);
    case '0':
        if (p + 1 != lexer->eof) {
            if (p[1] == 'x' || p[1] == 'X') {
                die("HEX numbers are not supported.");
            }
        }
    // fall through
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
        // decimal float
        {
            char *q = p;
            while (q != lexer->eof && (*q >= '0' && *q <= '9')) q++;
            if (q != lexer->eof) {
                if (*q == '.') {
                    die("Floats are not supported.");
               }
            }
        }
        // octal int or 0
        if (p[0] == '0') {
            char *q = p;
            lexer->int_number = strtol((char *)p, (char **)&q, 8);
            return _lexer_token(lexer, ALEX_intlit, p, q - 1);
        }
        // decimal int
        {
            char *q = p;
            lexer->int_number = strtol((char *)p, (char **) &q, 10);
            return _lexer_token(lexer, ALEX_intlit, p, q - 1);
        }
    } break;
    }
}

int main(int argc, char **argv)
{
    argc--; argv++;

    if (argc != 2) die("usage: marspp <input.asm> <output.asm>");
    const char *input_filename = argv[0];
    const char *output_filename = argv[1];

    FILE *f = fopen(input_filename, "rb");
    if (!f) die("Failed to open input.asm");

    char *input_file_content = (char *) malloc(1 << 20);
    int input_file_len = (int) fread(input_file_content, 1, 1 << 20, f);
    fclose(f);

    if (input_file_len < 0) die("Failed to read input.asm");
    input_file_content[input_file_len] = 0;

    lexer_t lexer;
    lexer_init(&lexer, input_file_content, input_file_content + input_file_len, (char *)malloc(1 << 8), 1 << 8);

    sv_t *output_asm = NULL;
    arrpush(output_asm, ((sv_t){
        .string = input_file_content,
        .length = input_file_len,
    }));

    sv_t *output_asm_data_patches = NULL;
    // If .data section is present, points to the .data part in the output_asm array
    int output_asm_data_idx = -1;
    // If .data section is present and there's at least one label, points to the first label in the output_asm array
    int output_asm_first_data_label_idx = -1;
    sv_t output_asm_first_data_label_lpad = {0};

    int mode = APAR_others;
    while (lexer_get_token(&lexer)) {
        if (lexer.token == ALEX_parse_error) die("\nPARSE ERROR");

        if (lexer.token == ALEX_directive) {
            if (strcmp(lexer.string, ".text") == 0) {
                mode = APAR_text;
            } else if (strcmp(lexer.string, ".data") == 0) {
                mode = APAR_data;

                char *match_begin = lexer.where_firstchar;
                char *match_end = lexer.where_lastchar;

                sv_t last = arrpop(output_asm);
                arrpush(output_asm, ((sv_t){ .string=last.string, .length=match_begin - last.string }));

                char *buffer = (char *)malloc(lexer.string_len + 1);
                snprintf(buffer, (lexer.string_len + 1), "%s", lexer.string);
                arrpush(output_asm, ((sv_t){ .string=buffer, .length=lexer.string_len }));
                output_asm_data_idx = arrlen(output_asm) - 1;

                arrpush(output_asm, ((sv_t){ .string=match_end + 1, .length=last.length - (match_begin - last.string) - (match_end - match_begin) }));
            } else {
                mode = APAR_others;
            }
        }

        switch (mode) {
        case APAR_data: {
            if (lexer.token == ALEX_ident && output_asm_first_data_label_idx == -1) {
                char *match_begin = lexer.where_firstchar;
                char *match_end = lexer.where_lastchar;

                sv_t last = arrpop(output_asm);
                arrpush(output_asm, ((sv_t){ .string=last.string, .length=match_begin - last.string }));

                char *buffer = (char *)malloc(lexer.string_len + 1);
                snprintf(buffer, (lexer.string_len + 1), "%s", lexer.string);
                arrpush(output_asm, ((sv_t){ .string=buffer, .length=lexer.string_len }));
                output_asm_first_data_label_idx = arrlen(output_asm) - 1;

                output_asm_first_data_label_lpad.length = match_begin - lexer.line_start;
                output_asm_first_data_label_lpad.string = (char *)malloc(output_asm_first_data_label_lpad.length);
                memcpy(output_asm_first_data_label_lpad.string, lexer.line_start, output_asm_first_data_label_lpad.length);
                for (int i = 0; i < output_asm_first_data_label_lpad.length; i++) {
                    if (!_lexer_iswhite(output_asm_first_data_label_lpad.string[i])) output_asm_first_data_label_lpad.string[i] = ' ';
                }

                arrpush(output_asm, ((sv_t){ .string=match_end + 1, .length=last.length - (match_begin - last.string) - (match_end - match_begin) }));
            }
        } break;
        case APAR_text: {
            if (strcmp(lexer.string, "exit") == 0) {
                char *match_begin = lexer.where_firstchar;
                if (!lexer_get_token(&lexer)) die("Unexpected EOF");
                if (lexer.token != '(') die("Expected ( after `exit`");
                if (!lexer_get_token(&lexer)) die("Unexpected EOF");
                if (lexer.token != ')') die("Expected ) after `exit(`");

                sv_t lpad = { .string=(char *)malloc(match_begin - lexer.line_start), .length=(match_begin - lexer.line_start) };
                memcpy(lpad.string, lexer.line_start, lpad.length);
                for (int i = 0; i < lpad.length; i++) {
                    if (!_lexer_iswhite(lpad.string[i])) lpad.string[i] = ' ';
                }
                sv_t last = arrpop(output_asm);
                arrpush(output_asm, ((sv_t){ .string=last.string, .length=match_begin - last.string }));
                char *buffer = (char *)malloc(1 << 8);
                int buffer_len = 0;
                buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "li $v0, 10\t# specify exit service\n%.*s", lpad.length, lpad.string);
                buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "syscall\t# exit\n%.*s", lpad.length, lpad.string);
                arrpush(output_asm, ((sv_t){ .string=buffer, .length=buffer_len }));
                char *match_end = lexer.where_lastchar;
                arrpush(output_asm, ((sv_t){ .string=match_end + 1, .length=last.length - (match_begin - last.string) - (match_end - match_begin) }));
            }
            if (strcmp(lexer.string, "print_string") == 0) {
                char *match_begin = lexer.where_firstchar;
                if (!lexer_get_token(&lexer)) die("Unexpected EOF");
                if (lexer.token != '(') die("Expected ( after `print_string`");

                bool parsing_second_arg = false;
            one_more_arg:
                sv_t lpad = { .string=(char *)malloc(match_begin - lexer.line_start), .length=(match_begin - lexer.line_start) };
                memcpy(lpad.string, lexer.line_start, lpad.length);
                for (int i = 0; i < lpad.length; i++) {
                    if (!_lexer_iswhite(lpad.string[i])) lpad.string[i] = ' ';
                }

                sv_t last = arrpop(output_asm);
                arrpush(output_asm, ((sv_t){ .string=last.string, .length=match_begin - last.string }));

                if (!lexer_get_token(&lexer)) die("Unexpected EOF");
                switch (lexer.token) {
                default: die("Expected an ident as the argumnt to print_string, got something else");
                case ALEX_string: {
                    char *generated_label = (char *)malloc(1 << 8);
                    snprintf(generated_label, (1 << 8), "label_marspp_%d", arrlen(output_asm_data_patches));

                    char *buffer = (char *)malloc(1 << 10);
                    snprintf(buffer, (1 << 10), "%s:\t.asciiz\t\"%s\"", generated_label, lexer.string);
                    arrpush(output_asm_data_patches, ((sv_t){ .string=buffer, .length=strlen(buffer) }));

                    buffer = (char *)malloc(1 << 8);
                    int buffer_len = 0;
                    if (parsing_second_arg) {
                        buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "\n%.*s", lpad.length, lpad.string);
                    }
                    buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "la $a0, %s\t# load \"%s\"\n%.*s", generated_label, lexer.string, lpad.length, lpad.string);
                    buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "li $v0, 4\t# specify print string service\n%.*s", lpad.length, lpad.string);
                    buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "syscall\t# print \"%s\"", lexer.string);

                    arrpush(output_asm, ((sv_t){ .string=buffer, .length=buffer_len }));
                } break;
                case ALEX_ident: {
                    char *buffer = (char *)malloc(1 << 8);
                    int buffer_len = 0;
                    if (parsing_second_arg) {
                        buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "\n%.*s", lpad.length, lpad.string);
                    }
                    buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "la $a0, %s\t# load %s\n%.*s", lexer.string, lexer.string, lpad.length, lpad.string);
                    buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "li $v0, 4\t# specify print string service\n%.*s", lpad.length, lpad.string);
                    buffer_len += snprintf(buffer + buffer_len, (1 << 8) - buffer_len, "syscall\t# print %s", lexer.string);

                    arrpush(output_asm, ((sv_t){ .string=buffer, .length=buffer_len }));
                } break;
                }

                if (!lexer_get_token(&lexer)) die("Unexpected EOF");
                if (lexer.token == ',') {
                    parsing_second_arg = true;
                    goto one_more_arg;
                }
                if (lexer.token != ')') die("Expected ) after `print_string(`");

                char *match_end = lexer.where_lastchar;
                arrpush(output_asm, ((sv_t){ .string=match_end + 1, .length=last.length - (match_begin - last.string) - (match_end - match_begin) }));
            }
        } break;
        default: break;
        }
    }

    if (output_asm_data_idx == -1 && arrlen(output_asm_data_patches) > 0) {
        char *data = "\n.data\n";
        arrpush(output_asm, ((sv_t){ .string=data, .length=strlen(data) }));
        output_asm_data_idx = arrlen(output_asm) - 1;
    }

    f = fopen(output_filename, "wb");
    if (!f) die("Failed to open output.asm");

    for (int i = 0; i < arrlen(output_asm); i++) {
        fprintf(f, "%.*s", output_asm[i].length, output_asm[i].string);

        // only one of the 2 if-statements will be true
        // 1: in the .data section, there's no labels present so we have to push the patches right after the .data section
        if (i == output_asm_data_idx) {
            if (output_asm_first_data_label_idx == -1) {
                for (int j = 0; j < arrlen(output_asm_data_patches); j++) {
                    fprintf(f, "\n%.*s", output_asm_data_patches[j].length, output_asm_data_patches[j].string);
                }
            }
        }
        // 2: there's a label present in the .data section, we have to push the patches right before the label, respecting the prefix
        if (i == output_asm_first_data_label_idx - 1) {
            for (int j = 0; j < arrlen(output_asm_data_patches); j++) {
                fprintf(f, "%.*s\n%.*s", output_asm_data_patches[j].length, output_asm_data_patches[j].string, output_asm_first_data_label_lpad.length, output_asm_first_data_label_lpad.string);
            }
        }
    }

    fclose(f);

    return 0;
}