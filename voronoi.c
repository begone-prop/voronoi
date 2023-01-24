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

#define RANGE 5000

typedef struct point {
    long x;
    long y;
} point;

typedef struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color;

typedef struct task_arg {
    long start;
    long size;
    long range;
    const point *anchors;
    size_t anchors_size;
    size_t *map;
} task_arg;

point randomPoint(long);
color randomColor(void);
size_t findNearestPointIdx(const point *, size_t, point);
void *calculateChunk(void *);
int writePNG(const char *, const size_t *, const color *, long);

point randomPoint(long range) {
    return (point){
        random() % range,
        random() % range
    };
}

color randomColor(void) {
    return (color) {
        random() % 255,
        random() % 255,
        random() % 255,
    };
}

size_t findNearestPointIdx(const point *anchors, size_t size, point target) {
    long min = LONG_MAX;
    size_t min_idx;

    for(size_t idx = 0; idx < size; idx++) {
        point d = {
            anchors[idx].x - target.x,
            anchors[idx].y - target.y,
        };

        long current = d.x * d.x + d.y * d.y;

        if(min > current) {
            min = current;
            min_idx = idx;
        }
    }

    return min_idx;
}

void *calculateChunk(void *arg) {
    task_arg *targ = (task_arg*) arg;

    point a = {
        .x = targ->start % targ->range,
        .y = targ->start / targ->range
    };

    point b = {
        .x = (targ->start + targ->size) % targ->range,
        .y = (targ->start + targ->size) / targ->range
    };

    for(long x = a.x; x < targ->range; x++) {
        targ->map[x + a.y * targ->range] = findNearestPointIdx(targ->anchors, targ->anchors_size, (point){x, a.y});
    }

    for(long y = a.y + 1; y < b.y; y++) {
        for(long x = 0; x < targ->range; x++) {
            targ->map[x + y * targ->range] = findNearestPointIdx(targ->anchors, targ->anchors_size, (point){x, y});
        }
    }

    for(long x = 0; x < b.x; x++) {
        targ->map[x + b.y * targ->range] = findNearestPointIdx(targ->anchors, targ->anchors_size, (point){x, b.y});
    }

    return (void*)(int)1;
}

int writePNG(const char *filename, const size_t *dmap, const color *colors, long range) {
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

    png_set_IHDR(pngp, infop, range, range, depth,
            PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
            );

    rows = png_malloc(pngp, range * sizeof(png_byte *));
    for(long r = 0; r < range; r++) {
        png_byte *row = png_malloc(pngp, range * pixel_size);
        rows[r] = row;
        for(long c = 0; c < range; c++) {
            size_t idx = dmap[c + r * range];
            color c = colors[idx];

            *row++ = c.red;
            *row++ = c.green;
            *row++ = c.blue;
        }
    }

    png_init_io(pngp, fp);
    png_set_rows(pngp, infop, rows);
    png_write_png(pngp, infop, PNG_TRANSFORM_IDENTITY, NULL);

    for(long r = 0; r < range; r++) {
        png_free(pngp, rows[r]);
    }

    png_free(pngp, rows);
    png_destroy_write_struct(&pngp, &infop);
    fclose(fp);
    return 1;
}

int main(int argc, char **argv) {
    void* addr = malloc(0);
    srandom((unsigned)*(unsigned*)&addr);
    free(addr);

    long range = argc > 1 ? strtol(argv[1], NULL, 10) : RANGE;
    long resolution = range * range;

    const size_t anchors_size = 50;
    point anchors[anchors_size];
    color anchor_colors[anchors_size];

    for(size_t idx = 0; idx < anchors_size; idx++) {
        anchors[idx] = randomPoint(range);
        anchor_colors[idx] = randomColor();
    }

    /*size_t *distance_map = calloc(resolution, sizeof(size_t));*/
    size_t *distance_map = mmap(NULL, resolution * sizeof(size_t), PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if(distance_map == MAP_FAILED) {
        fprintf(stderr, "Failed to allocate memory mmap(): %s\n", strerror(errno));
        return 1;
    }

    long threads = sysconf(_SC_NPROCESSORS_CONF);
    /*long threads = 1;*/
    long chunk = resolution / threads;
    long rem = resolution - threads * chunk;

    if(threads == 1) {
        printf("here\n");
        for(int y = 0; y < range; y++) {
            for(int x = 0; x < range; x++) {
                size_t nearest_idx = findNearestPointIdx(anchors, anchors_size, (point){x, y});
                distance_map[x + y * range] = nearest_idx;
            }
        }
    } else {
        long total = 0;
        long thread_count = 0;
        bool partition = true;

        pthread_t th[threads];
        task_arg *args[threads];

        while(partition) {
            long size;

            if(thread_count + 1 == threads || total + chunk + rem >= resolution) {
                size = chunk + rem;
                partition = false;
            } else size = chunk;

            args[thread_count] = malloc(sizeof(task_arg));
            /*fprintf(stderr, "[%li] start offset: %li, size: %li\n", thread_count, total, size);*/

            args[thread_count]->start = total;
            args[thread_count]->size = size;
            args[thread_count]->range = range;
            args[thread_count]->anchors = anchors;
            args[thread_count]->anchors_size = anchors_size;
            args[thread_count]->map = distance_map;

            if(pthread_create(&th[thread_count], NULL, calculateChunk, args[thread_count]) != 0) {
                fprintf(stderr, "Failed to create thread\n");
                return 1;
            }

            total += size;
            thread_count++;
        }

        for(long t = 0; t < thread_count; t++) {
            pthread_join(th[t], NULL);
        }

        for(long idx = 0; idx < thread_count; idx++) {
            free(args[idx]);
        }
    }

    /*writePNG("./vornoi.png", distance_map, anchor_colors, range);*/
    munmap(distance_map, resolution);
    return 0;
}
