#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include "./argument.h"
#include "canvas.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <getopt.h>
#include <err.h>

typedef enum Token {
    BAD = 0,
    END,
    ENTRY,
    DELIM,
    NUMBER
} Token;

static const char *token_name[] = { "BAD", "END", "ENTRY", "DELIM", "NUMBER" };

static const struct option long_options[] = {
    {"output_file", required_argument, NULL, 'o'},
    {"size", required_argument, NULL, 's'},
    {"anchors", required_argument, NULL, 'a'},
    {"anchors_from", required_argument, NULL, 'A'},
    {"colors", required_argument, NULL, 'c'},
    {"colors_from", required_argument, NULL, 'C'},
    {"frames", required_argument, NULL, 'f'},
    {"keep", no_argument, NULL, 'k'},
    {"seed", required_argument, NULL, 'x'},
    {"verbose", optional_argument, NULL, 'v'},
    {"help", optional_argument, NULL, 'h'},
    {0, 0, 0, 0},
};

static const size_t long_options_size = sizeof(long_options) / sizeof(long_options[0]);

static Token parseToken(const char *, const char **, long *);
static long *parseEntries(const char *, size_t, size_t *);
static char *mmapFile(const char *, size_t *);
static long getNumber(const char *);
static point parseSize(const char *);

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
        warn("Failed to allocate memory");
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

static point parseSize(const char *fmt) {
    point ret = {0, 0};

    size_t memb_size = 0;
    long *memb = parseEntries(fmt, 2, &memb_size);

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
        anc = NULL;
        *size = (long)memb;
    }

    return anc;
}

static color *parsePallete(const char *fmt, long *size) {
    size_t memb_size = 0;
    long *memb = parseEntries(fmt, 3, &memb_size);

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
        *size = (long)memb;
        cols = NULL;
    }

    return cols;
}

static char *mmapFile(const char *filename, size_t *size) {
    if(!size) return NULL;
    int fd = open(filename, O_RDONLY);

    struct stat sb;
    if(fstat(fd, &sb) == -1) {
        return NULL;
    }

    void *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if(addr == MAP_FAILED) return NULL;
    *size = sb.st_size;

    close(fd);
    return addr;
}


static long getNumber(const char *fmt) {
    char *c;
    long num = strtol(fmt, &c, 10);
    if(*c != '\0') return -1;
    return num;
}

Params parseArguments(int argc, char **argv) {
    opterr = 0;
    int opt;

    Params params = NEW_PARAMS();
    bool got_anchors = false;
    bool got_colors = false;

    int opt_idx = -1;


    while((opt = getopt_long(argc, argv, "o:s:a:A:c:C:f:kx:v::h", long_options, &opt_idx)) != -1) {
        switch(opt) {
            case 'o': {
                params.filename = optarg;
                break;
            }

            case 's': {
                point p = parseSize(optarg);

                if(p.x == 0) {
                    errx(1, "Invalid size option %s", optarg);
                }

                params.size = p;
                break;
            }

            case 'a': {
                if(got_anchors) {
                    errx(1, "Duplicate anchors options");
                }

                got_anchors = true;
                anchor *a = NULL;
                long a_size = 0;

                a = parseAnchors(optarg, &a_size);

                if(!a && a_size == 0) {
                    errx(1, "Invalid anchors option %s", optarg);
                }

                params.anchors = a;
                params.anchors_size = a_size;
                break;
            }

            case 'A': {
                if(got_anchors) {
                    errx(1, "Duplicate anchors options");
                }

                got_anchors = true;
                char *anchor_file = optarg;
                size_t cont_size = 0;
                char *contents = mmapFile(anchor_file, &cont_size);

                if(!contents) {
                    err(1, "mmap()");
                }

                long af_size = 0;
                anchor *af = parseAnchors(contents, &af_size);
                if(!af && af_size == 0) {
                    errx(1, "Invalid anchors option %s", optarg);
                }

                params.anchors = af;
                params.anchors_size = af_size;
                munmap(contents, cont_size);
                break;
            }

            case 'c': {
                if(got_colors) {
                    errx(1, "Duplicate colors options");
                }

                got_colors = true;
                color *p = NULL;
                long p_size = 0;

                p = parsePallete(optarg, &p_size);

                if(!p && p_size == 0) {
                    errx(1, "Invalid pallete option %s", optarg);
                }

                params.colors = p;
                params.colors_size = p_size;
                break;
            }

            case 'C': {
                if(got_colors) {
                    errx(1, "Duplicate colors options");
                }

                got_colors = true;
                char *colors_file = optarg;
                size_t cont_size = 0;
                char *contents = mmapFile(colors_file, &cont_size);

                if(!contents) {
                    err(1, "Failed call to mmap()");
                }

                long cl_size = 0;
                color *cl = parsePallete(contents, &cl_size);
                if(!cl && cl_size == 0) {
                    errx(1, "Invalid anchors option %s", optarg);
                }

                params.colors = cl;
                params.colors_size = cl_size;
                munmap(contents, cont_size);
                break;
            }

            case 'f': {
                long frames = getNumber(optarg);
                if(frames == -1) {
                    errx(1, "Invalid frames option: %s", optarg);
                }

                params.frames = frames;
                break;
            }

            case 'k': {
                params.keep = true;
                break;
            }

            case 'x': {
                long seed = getNumber(optarg);

                if(seed == -1) {
                    errx(1, "Invalid frames option: %s", optarg);
                }

                params.seed = seed;
                break;
            }

            case 'v': {
                break;
            }

            case 'h': {
                break;
            }

            case 0:
            case '?': {
                errx(1, "Got invaid argument: %s", argv[optind - 1]);
            }
        }
    }

    if(params.seed == 0) {
        void* addr = malloc(0);
        params.seed = (long)*(long*)&addr;
        free(addr);
    }

    srandom(params.seed);
    if(!params.filename) {
        errx(1, "No input file");
    }

    if(!params.colors) {
        params.colors = calloc(params.colors_size, sizeof(color));
        if(!params.colors) {
            err(1, "Failed to allocate %zu bytes", params.colors_size * sizeof(color));
        }

        for(size_t idx = 0; idx < params.colors_size; idx++) {
            params.colors[idx] = randomColor();
        }
    }

    if(!params.anchors) {
        params.anchors = calloc(params.anchors_size, sizeof(anchor));
        if(!params.anchors) {
            err(1, "Failed to allocate %zu bytes", params.anchors_size * sizeof(color));
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
