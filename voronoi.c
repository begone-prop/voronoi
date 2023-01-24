#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include <png.h>
#include <wand/MagickWand.h>

#define ThrowWandException(wand) \
{ \
  char \
    *description; \
 \
  ExceptionType \
    severity; \
 \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
  exit(-1); \
}

typedef struct point {
    long x;
    long y;
} point;

typedef struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color;

typedef struct anchor {
    point pos;
    color col;
} anchor;

typedef struct task_arg {
    long start;
    long run;
    point size;
    const anchor *anchors;
    size_t anchors_size;
    color *map;
} task_arg;

point randomPoint(point);
color randomColor(void);
color determinePixelColor(const anchor *, size_t, point);
void *calculateChunk(void *);
int generateVoronoi(color *, point, const anchor *, size_t);
int generatePNG(const char *, const color *, point);
int generateGIF(const char *, anchor *, size_t, color *, point, size_t);

point randomPoint(point range) {
    return (point){
        random() % range.x,
        random() % range.y
    };
}

color randomColor(void) {
    return (color) {
        random() % 255,
        random() % 255,
        random() % 255,
    };
}

color determinePixelColor(const anchor *anchors, size_t size, point target) {
    long min = LONG_MAX;
    color c;

    for(size_t idx = 0; idx < size; idx++) {
        point d = {
            anchors[idx].pos.x - target.x,
            anchors[idx].pos.y - target.y,
        };

        long current = d.x * d.x + d.y * d.y;

        if(min > current) {
            min = current;
            c = anchors[idx].col;
        }
    }

    return c;
}

void *calculateChunk(void *arg) {
    task_arg *targ = (task_arg*) arg;

    point a = {
        .x = targ->start % targ->size.x,
        .y = targ->start / targ->size.x
    };

    point b = {
        .x = (targ->start + targ->run) % targ->size.x,
        .y = (targ->start + targ->run) / targ->size.x
    };

    for(long x = a.x; x < targ->size.x; x++) {
        targ->map[x + a.y * targ->size.x] = determinePixelColor(targ->anchors, targ->anchors_size, (point){x, a.y});
    }

    for(long y = a.y + 1; y < b.y; y++) {
        for(long x = 0; x < targ->size.x; x++) {
            targ->map[x + y * targ->size.x] = determinePixelColor(targ->anchors, targ->anchors_size, (point){x, y});
        }
    }

    for(long x = 0; x < b.x; x++) {
        targ->map[x + b.y * targ->size.x] = determinePixelColor(targ->anchors, targ->anchors_size, (point){x, b.y});
    }

    return (void*)(int)1;
}

int generatePNG(const char *filename, const color *color_map, point size) {
    FILE *fp;
    png_structp pngp = NULL;
    png_infop infop = NULL;
    png_byte **rows = NULL;
    int pixel_size =3;
    int depth = 8;

    fp = fopen(filename, "wb+");
    if(!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        return 0;
    }

    pngp = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(pngp == NULL) {
        fprintf(stderr, "Failed call to png_create_write_struct()\n");
        return 0;
    }

    infop = png_create_info_struct(pngp);

    if(infop == NULL) {
        fprintf(stderr, "Failed call to png_create_info_struct()\n");
        return 0;
    }

    png_set_IHDR(pngp, infop, size.x, size.y, depth,
            PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
            );

    rows = png_malloc(pngp, size.y * sizeof(png_byte *));
    for(long r = 0; r < size.y; r++) {
        png_byte *row = png_malloc(pngp, size.x * pixel_size);
        rows[r] = row;
        for(long c = 0; c < size.x; c++) {
            color col = color_map[c + r * size.x];
            *row++ = col.red;
            *row++ = col.green;
            *row++ = col.blue;
        }
    }

    png_init_io(pngp, fp);
    png_set_rows(pngp, infop, rows);
    png_write_png(pngp, infop, PNG_TRANSFORM_IDENTITY, NULL);

    for(long r = 0; r < size.y; r++) {
        png_free(pngp, rows[r]);
    }

    png_free(pngp, rows);
    png_destroy_write_struct(&pngp, &infop);
    fclose(fp);
    return 1;
}

int generateVoronoi(color *buffer, point size, const anchor *anchors, size_t num_anchors) {
    long area = size.x * size.y;
    long threads = sysconf(_SC_NPROCESSORS_CONF);
    long chunk = area / threads;
    long rem = area - threads * chunk;

    if(threads == 1) {
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
            buffer[x + y * size.x] = determinePixelColor(anchors, num_anchors, (point){x, y});
            }
        }
    } else {
        long total = 0;
        long thread_count = 0;
        bool partition = true;

        pthread_t th[threads];
        task_arg *args[threads];

        while(partition) {
            long run;

            if(thread_count + 1 == threads || total + chunk + rem >= area) {
                run = chunk + rem;
                partition = false;
            } else run = chunk;

            args[thread_count] = malloc(sizeof(task_arg));

            args[thread_count]->start = total;
            args[thread_count]->size = size;
            args[thread_count]->run = run;
            args[thread_count]->anchors = anchors;
            args[thread_count]->anchors_size = num_anchors;
            args[thread_count]->map = buffer;

            if(pthread_create(&th[thread_count], NULL, calculateChunk, args[thread_count]) != 0) {
                fprintf(stderr, "Failed to create thread\n");
                return 1;
            }

            total += run;
            thread_count++;
        }

        for(long t = 0; t < thread_count; t++) {
            pthread_join(th[t], NULL);
        }

        for(long idx = 0; idx < thread_count; idx++) {
            free(args[idx]);
        }
    }

    return 1;
}

int generateGIF(const char *filename, anchor *anchors, size_t anchors_size, color *color_map, point size, size_t frames) {
    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();
    MagickBooleanType status;

    static char filepath[PATH_MAX];
    for(size_t frame = 1; frame <= frames; frame++) {
        sprintf(filepath, "frame_%02zu.png", frame);
        /*fprintf(stderr, "%s\n", filepath);*/

        for(size_t idx = 0; idx < anchors_size; idx++) {
            point sign = {
                .x = random() % 2 ? -1 : 1,
                .y = random() % 2 ? -1 : 1,
            };

            point offset = randomPoint((point){3, 3});
            anchors[idx].pos.x += offset.x * sign.x;
            anchors[idx].pos.y += offset.y * sign.y;
        }

        generateVoronoi(color_map, size, anchors, anchors_size);
        generatePNG(filepath, color_map, size);
        MagickReadImage(wand, filepath);
    }


    status = MagickWriteImages(wand, filename, MagickTrue);

    if(status == MagickFalse) {
        ThrowWandException(wand);
    }

    wand = DestroyMagickWand(wand);
    MagickWandTerminus();
    return 0;
}

int main(int argc, char **argv) {
    void* addr = malloc(0);
    srandom((unsigned)*(unsigned*)&addr);
    free(addr);

    const char *filename = "./voronoi.png";
    point size = {1000, 1000};
    long area = size.x * size.y;

    const size_t anchors_size = 10;
    anchor anchors[anchors_size];

    for(size_t idx = 0; idx < anchors_size; idx++) {
        anchors[idx].pos = randomPoint(size);
        anchors[idx].col = randomColor();
    }

    color *color_map = mmap(NULL, area * sizeof(color), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(color_map == MAP_FAILED) {
        fprintf(stderr, "Failed to allocate memory mmap(): %s\n", strerror(errno));
        return 1;
    }

    /*generateVoronoi(color_map, size, anchors, anchors_size);*/
    /*writePNG(filename, color_map, size);*/

    generateGIF("final.gif", anchors, anchors_size, color_map, size, 60);

    munmap(color_map, area);
    return 0;
}
