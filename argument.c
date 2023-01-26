
#include "./argument.h"
#include "canvas.h"
#include <stdbool.h>
#include <stdio.h>
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

static point parseSize(const char *);

static point parseSize(const char *fmt) {
    point ret = {0, 0};

    size_t memb_size = 0;
    long *memb = parseEntries(optarg, 2, &memb_size);

    if(!memb || memb_size == 0)
        return ret;

    if(memb_size == 1) {
        ret.x = (long) memb;
        ret.y = ret.x;
    } else {
        ret.x = memb[0];
        ret.y = memb[1];
        free(memb);
    }

    return ret;
}

static anchor *parseAnchors(const char *fmt, long *size) {
    size_t memb_size = 0;
    long *memb = parseEntries(fmt, 2, &memb_size);

    size_t size_anc = 0;

    if(!size || !memb || memb_size == 0)
        return NULL;

    anchor *anc = NULL;

    if(memb_size > 1) {
        size_t count = memb_size / 2;
        anc = calloc(memb_size * memb_size / 2, sizeof(anchor));
        for(size_t idx = 0, ent = 0; idx < memb_size; idx += 2, ent++) {
            anc[ent].pos.x = memb[idx];
            anc[ent].pos.y = memb[idx + 1];
        }

        *size = count;
    } else {
        *size = -1;
        anc = (anchor*)(long)memb;
    }

    return anc;
}

static color *parsePallete(const char *fmt, long *size) {
    size_t memb_size = 0;
    long *memb = parseEntries(fmt, 3, &memb_size);

    size_t size_anc = 0;

    if(!size || !memb || memb_size == 0)
        return NULL;

    color *cols = NULL;

    if(memb_size > 1) {
        size_t count = memb_size / 3;
        cols = calloc(memb_size * count, sizeof(color));
        for(size_t idx = 0, ent = 0; idx < memb_size; idx += 3, ent++) {
            cols[ent].red = memb[idx];
            cols[ent].green = memb[idx + 1];
            cols[ent].blue = memb[idx + 2];
        }

        *size = count;
    } else {
        *size = -1;
        cols = (color*)(long)memb;
    }

    return cols;
}

Params parseArguments(int argc, char **argv) {
    opterr = 0;
    int opt;
    size_t anchors_size = 0;

    Params params = NEW_PARAMS();

    while((opt = getopt_long(argc, argv, "o:s:a:A:p:P:f:kx:v::h", long_options, NULL)) != -1) {
        switch(opt) {
            case 'o': {
                fprintf(stderr, "Got output_file option\n");
                params.filename = optarg;
                break;
            }

            case 's': {
                fprintf(stderr, "Got size option\n");
                point p = parseSize(optarg);

                if(p.x == 0) {
                    fprintf(stderr, "Invalid size option %s\n", optarg);
                    exit(1);
                }

                params.size = p;
                break;
            }

            case 'a': {
                fprintf(stderr, "Got anchors option\n");
                anchor *a = NULL;
                long a_size = 0;

                a = parseAnchors(optarg, &a_size);

                if(!a || (a_size == -1 && (long)a == 0)) {
                    fprintf(stderr, "Invalid anchors option %s\n", optarg);
                    exit(1);
                }

                if(a_size == -1) {
                    params.anchors = NULL;
                    params.anchors_size = (long) a;
                    fprintf(stderr, "Got number %li\n", params.anchors_size);
                }
                else {
                    params.anchors = a;
                    params.anchors_size = a_size;
                    for(size_t idx = 0; idx < a_size; idx++) {
                        fprintf(stderr, "[%3zu]: (%4li, %4li)\n", idx, a[idx].pos.x, a[idx].pos.y);
                    }
                }

                break;
            }

            case 'A': {
                fprintf(stderr, "Got anchors_from option\n");
                break;
            }

            case 'p': {
                fprintf(stderr, "Got pallete option\n");
                color *p = NULL;
                long p_size = 0;

                p = parsePallete(optarg, &p_size);

                if(!p || (p_size == -1 && (long)p == 0)) {
                    fprintf(stderr, "Invalid pallete option %s\n", optarg);
                    exit(1);
                }

                if(p_size == -1) {
                    params.colors = NULL;
                    params.colors_size = (long) p;
                    fprintf(stderr, "Got number %li\n", params.colors_size);
                }
                else {
                    params.colors = p;
                    params.colors_size = p_size;
                    for(size_t idx = 0; idx < p_size; idx++) {
                        fprintf(stderr, "[%3zu]: (%3i, %3i, %3i)\n", idx, p[idx].red, p[idx].green, p[idx].blue);
                    }
                }

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

    if(!params.colors) {
        params.colors = calloc(params.colors_size, sizeof(color));
        if(!params.colors) {
            fprintf(stderr, "Failed to allocate %zu bytes\n", params.colors_size * sizeof(color));
            exit(1);
        }

        for(size_t idx = 0; idx < params.colors_size; idx++) {
            params.colors[idx] = randomColor();
        }
    }

    if(!params.anchors) {
        params.anchors = calloc(params.anchors_size, sizeof(anchor));
        if(!params.anchors) {
            fprintf(stderr, "Failed to allocate %zu bytes\n", params.anchors_size * sizeof(color));
            exit(1);
        }

        for(size_t idx = 0; idx < params.anchors_size; idx++) {
            params.anchors[idx].pos = randomPoint(params.size);
        }
    }

    for(size_t idx = 0; idx < params.anchors_size; idx++) {
        size_t rand_idx = random() % params.colors_size;
        params.anchors[idx].col = params.colors[rand_idx];
    }

    return params;
}
