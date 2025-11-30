#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <getopt.h>
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
#endif

typedef enum { BOUNDARY_EDGE=0, BOUNDARY_TORUS, BOUNDARY_MIRROR, BOUNDARY_RIM } boundary_t;

typedef struct {
    int width;
    int height;
    size_t words_per_row;
    uint64_t *grid;
    uint64_t *scratch;
    boundary_t boundary;
} world_t;

static inline uint8_t get_cell(const world_t *w, int x, int y) {
    if (x < 0 || x >= w->width || y < 0 || y >= w->height) return 0;
    size_t wi = (size_t)y * w->words_per_row + (x >> 6);
    int bit = x & 63;
    return (w->grid[wi] >> bit) & 1ULL;
}
static inline void set_cell(world_t *w, int x, int y, uint8_t v) {
    if (x < 0 || x >= w->width || y < 0 || y >= w->height) return;
    size_t wi = (size_t)y * w->words_per_row + (x >> 6);
    int bit = x & 63;
    uint64_t mask = 1ULL << bit;
    if (v) w->grid[wi] |= mask; else w->grid[wi] &= ~mask;
}
static inline void set_cell_scratch(world_t *w, int x, int y, uint8_t v) {
    if (x < 0 || x >= w->width || y < 0 || y >= w->height) return;
    size_t wi = (size_t)y * w->words_per_row + (x >> 6);
    int bit = x & 63;
    uint64_t mask = 1ULL << bit;
    if (v) w->scratch[wi] |= mask; else w->scratch[wi] &= ~mask;
}

static int get_cell_boundary(const world_t *w, int x, int y) {
    if (x >= 0 && x < w->width && y >= 0 && y < w->height)
        return get_cell(w, x, y);
    switch (w->boundary) {
        case BOUNDARY_EDGE:   return 0;
        case BOUNDARY_RIM:    return 1;
        case BOUNDARY_TORUS: {
            int xx = (x % w->width + w->width) % w->width;
            int yy = (y % w->height + w->height) % w->height;
            return get_cell(w, xx, yy);
        }
        case BOUNDARY_MIRROR: {
            int xx = x, yy = y;
            if (xx < 0) xx = -xx - 1;
            if (yy < 0) yy = -yy - 1;
            if (xx >= w->width)  xx = 2*w->width  - xx - 1;
            if (yy >= w->height) yy = 2*w->height - yy - 1;
            if (xx < 0) xx = 0; if (yy < 0) yy = 0;
            if (xx >= w->width)  xx = w->width - 1;
            if (yy >= w->height) yy = w->height - 1;
            return get_cell(w, xx, yy);
        }
    }
    return 0;
}

static int count_neighbors(const world_t *w, int x, int y) {
    int sum = 0;
    for (int oy = -1; oy <= 1; oy++)
        for (int ox = -1; ox <= 1; ox++)
            if (!(ox == 0 && oy == 0))
                sum += get_cell_boundary(w, x + ox, y + oy);
    return sum;
}

static void next_generation(world_t *w) {
    size_t tw = (size_t)w->height * w->words_per_row;
    memset(w->scratch, 0, tw * sizeof(uint64_t));
    for (int y = 0; y < w->height; y++) {
        for (int x = 0; x < w->width; x++) {
            int n = count_neighbors(w, x, y);
            int c = get_cell(w, x, y);
            int next = c ? (n == 2 || n == 3) : (n == 3);
            set_cell_scratch(w, x, y, next);
        }
    }
    uint64_t *tmp = w->grid; w->grid = w->scratch; w->scratch = tmp;
}

static world_t *world_create(int w, int h, boundary_t b) {
    world_t *W = (world_t*)malloc(sizeof(world_t));
    if (!W) return NULL;
    W->width = w; W->height = h; W->boundary = b;
    W->words_per_row = ((size_t)w + 63) / 64;
    size_t total_words = W->words_per_row * (size_t)h;
    W->grid    = (uint64_t*)calloc(total_words, sizeof(uint64_t));
    W->scratch = (uint64_t*)calloc(total_words, sizeof(uint64_t));
    if (!W->grid || !W->scratch) { free(W->grid); free(W->scratch); free(W); return NULL; }
    return W;
}
static void world_free(world_t *w) {
    if (!w) return;
    free(w->grid); free(w->scratch); free(w);
}

static int load_pattern_center(world_t *w, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    char *line = NULL; size_t cap = 0; ssize_t len;
    char **lines = NULL; size_t nlines = 0; size_t maxlen = 0;
    while ((len = getline(&line, &cap, f)) > 0) {
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
        if (len == 0) continue;
        lines = (char**)realloc(lines, (nlines+1) * sizeof(char*));
        lines[nlines++] = strdup(line);
        if ((size_t)len > maxlen) maxlen = (size_t)len;
    }
    free(line); fclose(f);
    if (nlines == 0) { free(lines); return 0; }
    int patW = (int)maxlen, patH = (int)nlines;
    int offX = (w->width - patW) / 2; if (offX < 0) offX = 0;
    int offY = (w->height - patH) / 2; if (offY < 0) offY = 0;
    for (int r = 0; r < patH; r++) {
        int L = (int)strlen(lines[r]);
        for (int c = 0; c < L; c++) {
            int x = offX + c, y = offY + r;
            if (x >= 0 && x < w->width && y >= 0 && y < w->height) {
                char ch = lines[r][c];
                set_cell(w, x, y, (ch == '*' || ch == '1' || ch == 'X'));
            }
        }
    }
    for (size_t i = 0; i < nlines; i++) free(lines[i]);
    free(lines);
    return 0;
}

static double timespec_diff_ms(const struct timespec *a, const struct timespec *b) {
    return (b->tv_sec - a->tv_sec) * 1000.0 + (b->tv_nsec - a->tv_nsec) / 1e6;
}

static boundary_t parse_boundary(const char *s) {
    if (!strcmp(s,"edge")) return BOUNDARY_EDGE;
    if (!strcmp(s,"torus")) return BOUNDARY_TORUS;
    if (!strcmp(s,"mirror")) return BOUNDARY_MIRROR;
    if (!strcmp(s,"rim")) return BOUNDARY_RIM;
    return BOUNDARY_TORUS;
}

static void usage(const char *p) {
    fprintf(stderr, "Usage: %s --width W --height H --gens N --boundary edge|torus|mirror|rim [--in file]\n", p);
}

int main(int argc, char **argv) {
    int width = 320, height = 240, gens = 1000;
    char *infile = NULL;
    boundary_t boundary = BOUNDARY_TORUS;

    // Simple manual parsing of --key value pairs
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        if (!strcmp(argv[i], "--width") && i+1 < argc) { width = atoi(argv[++i]); continue; }
        if (!strcmp(argv[i], "--height") && i+1 < argc) { height = atoi(argv[++i]); continue; }
        if (!strcmp(argv[i], "--gens") && i+1 < argc) { gens = atoi(argv[++i]); continue; }
        if (!strcmp(argv[i], "--boundary") && i+1 < argc) { boundary = parse_boundary(argv[++i]); continue; }
        if (!strcmp(argv[i], "--in") && i+1 < argc) { infile = strdup(argv[++i]); continue; }
    }

    world_t *w = world_create(width, height, boundary);
    if (!w) { fprintf(stderr,"alloc fail\n"); return 1; }

    if (infile) {
        if (load_pattern_center(w, infile) != 0)
            fprintf(stderr,"warn: failed loading %s, using random init\n", infile);
        else goto measure;
    }

    srand((unsigned)time(NULL));
    for (int y=0; y<w->height; y++)
        for (int x=0; x<w->width; x++)
            set_cell(w, x, y, (rand() % 4 == 0));

measure:
    double mean=0.0, worst=0.0;
    struct timespec t0, t1;
    for (int g=0; g<gens; g++) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        next_generation(w);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ms = timespec_diff_ms(&t0, &t1);
        mean += ms;
        if (ms > worst) worst = ms;
    }
    mean /= gens;
    double jitter = worst - mean;

    printf("Mesures (width=%d height=%d gens=%d boundary=%d):\n", width, height, gens, boundary);
    printf("Moyenne: %.3f ms\n", mean);
    printf("Pire cas: %.3f ms\n", worst);
    printf("Jitter: %.3f ms\n", jitter);
    printf("Cible temps reel 60 Hz: 16.7 ms par generation -> %s\n",
           (mean <= 16.7 && worst <= 16.7) ? "OK" : "A optimiser");

    world_free(w);
    free(infile);
    return 0;
}
