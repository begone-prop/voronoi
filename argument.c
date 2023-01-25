#include "./argument.h"
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

typedef enum Token {
    BAD = 0,
    END,
    ENTRY,
    DELIM,
    NUMBER
} Token;

static const char *token_name[] = { "BAD", "END", "ENTRY", "DELIM", "NUMBER" };

static Token parseToken(const char *, const char **, long *);
static long *parseEntries(const char *, size_t, size_t *);

Token parseToken(const char *fmt, const char **next, long *number) {
    char c = 0;
    char digit_buff[16];
    size_t idx = 0;

    do {
        if((c = *fmt) == '\0') {
            if(next) *next = fmt;
            return END;
        }
    } while(isspace(*fmt++));

    switch(c) {
        case 0:
            if(next) *next = fmt;
            return END;

        case ';':
        case '\n':
            if(next) *next = fmt;
            return ENTRY;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            fmt--;

            while(isdigit(*fmt)) {
                digit_buff[idx++] = *fmt++;
            };

            digit_buff[idx++] = '\0';

            *number = strtol(digit_buff, NULL, 10);

            if(next) *next = fmt;
            return NUMBER;

        case ',':
            if(next) *next = fmt;
            return DELIM;

        default:
            return BAD;
    }
}

long *parseEntries(const char *fmt, size_t members_count, size_t *written) {

    if(!fmt || members_count == 0) return NULL;

    size_t numbers = 0;
    const char *next = NULL;
    bool parsed = false;
    long number = 0;
    size_t num_entries = 0;
    bool single_value = false;
    const char *start = fmt;

    while(!parsed) {
        Token t = parseToken(fmt, &next, &number);
        fmt = next;
        switch(t) {
            case NUMBER:
                numbers++;
                break;

            case END:
                parsed = true;
            case ENTRY:
                if(num_entries == 0 && numbers == 0) return NULL;
                if(num_entries == 0 && numbers != 1 && numbers != members_count) return NULL;
                if(num_entries > 0 && numbers != 0 && numbers != members_count) return NULL;

                if(num_entries == 0 && numbers == 1) {
                    single_value = true;
                    if(written) *written = 1;
                    parsed = true;
                }

                if(numbers > 0) num_entries++;
                numbers = 0;
                break;

            case BAD:
                return NULL;

            case DELIM:
                break;
        }
    }

    if(single_value) {
        if(written) *written = 1;
        return (long*)number;
    }

    if(written) *written = num_entries * members_count;

    long *entries = NULL;

    entries = calloc(num_entries * members_count, sizeof(long));
    if(!entries) {
        fprintf(stderr, "Failed to allocate memory\n");
        return NULL;
    }

    parsed = false;
    next = NULL;
    num_entries = 0;
    numbers = 0;

    while(!parsed) {
        Token t = parseToken(start, &next, &number);
        start = next;
        switch(t) {
            case NUMBER:
                entries[numbers + num_entries * members_count] = number;
                numbers++;
                break;

            case END:
                parsed = true;
            case ENTRY:
                num_entries++;
                numbers = 0;
                break;

            default:
                break;
        }
    }

    return entries;
}

Params parseArguments(int argc, char **argv) {
    opterr = 0;
    int opt;

    Params p;

    while((opt = getopt_long(argc, argv, "o:s:a:A:p:P:f:kx:v::h", long_options, NULL)) != -1) {
        switch(opt) {
            case 'o': {
                fprintf(stderr, "Got output_file option\n");
                break;
            }

            case 's': {
                fprintf(stderr, "Got size option\n");
                size_t mem_written = 0;
                long *mem = parseEntries(optarg, 2, &mem_written);

                if(mem == NULL || mem_written == 0) {
                    fprintf(stderr, "Invalid size option %s\n", optarg);
                    return p;
                }

                fprintf(stderr, "Num written: %zu\n", mem_written);

                /*if(mem_written == 1) {*/
                    /*size.x = (long) mem;*/
                    /*size.y = size.x;*/
                /*} else {*/
                    /*size.x = mem[0];*/
                    /*size.y = mem[1];*/
                    /*[>free(mem);<]*/
                /*}*/

                break;
            }

            case 'a': {
                fprintf(stderr, "Got anchors option\n");
                break;
            }

            case 'A': {
                fprintf(stderr, "Got anchors_from option\n");
                break;
            }

            case 'p': {
                fprintf(stderr, "Got pallete option\n");
                break;
            }

            case 'P': {
                fprintf(stderr, "Got pallete_from option\n");
                break;
            }

            case 'f': {
                fprintf(stderr, "Got frames option\n");
                break;
            }

            case 'k': {
                fprintf(stderr, "Got keep option\n");
                break;
            }

            case 'x': {
                fprintf(stderr, "Got seed option\n");
                break;
            }

            case 'v': {
                fprintf(stderr, "Got verbose option\n");
                break;
            }

            case 'h': {
                fprintf(stderr, "Got help option\n");
                break;
            }

            default: {
                fprintf(stderr, "Got %c\n", opt);
                break;
            }
        }
    }

    return p;
}
