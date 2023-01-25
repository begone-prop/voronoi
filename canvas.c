#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#include "./canvas.h"
#include <png.h>
/*#include <wand/MagickWand.h>*/

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

/*int generateGIF(const char *filename, anchor *anchors, size_t anchors_size, color *color_map, point size, size_t frames, int velocity) {*/
    /*MagickWandGenesis();*/
    /*MagickWand *wand = NewMagickWand();*/
    /*MagickBooleanType status;*/
    /*[>MagickSetCompression(wand, BZipCompression);<]*/

    /*point direction[] = {*/
        /*{-1, 0}, {-1, 1}, {0, 1}, {1, 1},*/
        /*{1, 0}, {1, -1}, {0, -1}, {-1, -1}*/
    /*};*/

    /*static const size_t direction_size = sizeof(direction) / sizeof(direction[0]);*/

    /*static char filepath[PATH_MAX];*/
    /*for(size_t frame = 1; frame <= frames; frame++) {*/
        /*sprintf(filepath, "frame_%zu.png", frame);*/

        /*for(size_t idx = 0; idx < anchors_size; idx++) {*/
            /*size_t dir = random() % direction_size;*/
            /*point step = direction[dir];*/
            /*anchors[idx].pos.x += step.x * velocity;*/
            /*anchors[idx].pos.y += step.y * velocity;*/
        /*}*/

        /*generateVoronoi(color_map, size, anchors, anchors_size);*/
        /*generatePNG(filepath, color_map, size);*/
        /*MagickReadImage(wand, filepath);*/
    /*}*/

    /*status = MagickWriteImages(wand, filename, MagickTrue);*/

    /*if(status == MagickFalse) {*/
        /*ExceptionType severity;*/
        /*char *description = MagickGetException(wand,&severity);*/
        /*(void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description);*/
        /*description= (char *) MagickRelinquishMemory(description);*/
        /*exit(-1);*/
    /*}*/

    /*for(size_t frame = 1; frame <= frames; frame++) {*/
        /*sprintf(filepath, "frame_%zu.png", frame);*/
        /*remove(filepath);*/
    /*}*/

    /*wand = DestroyMagickWand(wand);*/
    /*MagickWandTerminus();*/
    /*return 0;*/
/*}*/
