#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
// Use mingw-w64 getopt if available; otherwise, fallback simple parser
#include <getopt.h>
#else
#include <getopt.h>
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
    if (!f) { perror("open pattern"); return -1; }
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

static uint64_t mix64(uint64_t x){ x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33; return x; }
static uint64_t world_hash(const world_t *w) {
    uint64_t h = 0x9e3779b97f4a7c15ULL;
    size_t total_words = w->words_per_row * (size_t)w->height;
    for (size_t i = 0; i < total_words; i++) {
        h ^= mix64(w->grid[i] + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2));
    }
    return h;
}
static int bbox(const world_t *w, int *minx, int *miny, int *maxx, int *maxy) {
    int found = 0;
    int miX = w->width, miY = w->height, maX = -1, maY = -1;
    for (int y=0; y<w->height; y++) {
        for (int x=0; x<w->width; x++) {
            if (get_cell(w,x,y)) {
                found = 1;
                if (x<miX) miX=x; if (x>maX) maX=x;
                if (y<miY) miY=y; if (y>maY) maY=y;
            }
        }
    }
    if (!found) return 0;
    *minx = miX; *miny = miY; *maxx = maX; *maxy = maY;
    return 1;
}

typedef struct {
    int is_still_life;
    int is_oscillator;
    int period;
    int is_spaceship;
    int dx, dy;
} pattern_info_t;

static pattern_info_t analyze_pattern(world_t *w, int gens) {
    const int max_hist = (gens < 2048 ? gens : 2048);
    uint64_t *hist = (uint64_t*)malloc(sizeof(uint64_t) * (size_t)max_hist);
    int *minxs = (int*)malloc(sizeof(int) * (size_t)max_hist);
    int *minys = (int*)malloc(sizeof(int) * (size_t)max_hist);
    int *maxxs = (int*)malloc(sizeof(int) * (size_t)max_hist);
    int *maxys = (int*)malloc(sizeof(int) * (size_t)max_hist);

    pattern_info_t info = {0};

    hist[0] = world_hash(w);
    (void)bbox(w, &minxs[0], &minys[0], &maxxs[0], &maxys[0]);

    int first_repeat_idx = -1;
    for (int i = 1; i < max_hist; i++) {
        next_generation(w);
        hist[i] = world_hash(w);
        (void)bbox(w, &minxs[i], &minys[i], &maxxs[i], &maxys[i]);
        for (int j = 0; j < i; j++) {
            if (hist[j] == hist[i]) {
                first_repeat_idx = j;
                info.period = i - j;
                info.is_oscillator = (info.period > 0);
                break;
            }
        }
        if (first_repeat_idx >= 0) break;
    }
    if (info.is_oscillator && info.period == 1) {
        info.is_still_life = 1;
    }
    if (info.is_oscillator && info.period >= 2) {
        int j = first_repeat_idx;
        int i = j + info.period;
        int dx = minxs[i] - minxs[j];
        int dy = minys[i] - minys[j];
        if (dx != 0 || dy != 0) {
            info.is_spaceship = 1;
            info.dx = dx;
            info.dy = dy;
        }
    }

    free(hist); free(minxs); free(minys); free(maxxs); free(maxys);
    return info;
}

static void usage(const char *p) {
    fprintf(stderr,
        "Usage: %s --in file --width W --height H --gens N --boundary edge|torus|mirror|rim\n",
        p);
}

static boundary_t parse_boundary(const char *s) {
    if (!strcmp(s,"edge")) return BOUNDARY_EDGE;
    if (!strcmp(s,"torus")) return BOUNDARY_TORUS;
    if (!strcmp(s,"mirror")) return BOUNDARY_MIRROR;
    if (!strcmp(s,"rim")) return BOUNDARY_RIM;
    return BOUNDARY_TORUS;
}

int main(int argc, char **argv) {
    int width = 80, height = 40, gens = 200;
    char *infile = NULL;
    boundary_t boundary = BOUNDARY_TORUS;

    // Simple manual parsing of --key value pairs
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        if (!strcmp(argv[i], "--in") && i+1 < argc) { infile = strdup(argv[++i]); continue; }
        if (!strcmp(argv[i], "--width") && i+1 < argc) { width = atoi(argv[++i]); continue; }
        if (!strcmp(argv[i], "--height") && i+1 < argc) { height = atoi(argv[++i]); continue; }
        if (!strcmp(argv[i], "--gens") && i+1 < argc) { gens = atoi(argv[++i]); continue; }
        if (!strcmp(argv[i], "--boundary") && i+1 < argc) { boundary = parse_boundary(argv[++i]); continue; }
    }
    if (!infile) { usage(argv[0]); return 1; }

    world_t *w = world_create(width, height, boundary);
    if (!w) { fprintf(stderr,"alloc fail\n"); return 1; }
    if (load_pattern_center(w, infile) != 0) fprintf(stderr,"warn: failed loading %s\n", infile);

    pattern_info_t info = analyze_pattern(w, gens);

    printf("Motif: %s\n", infile);
    printf("Boundary: %d (0=edge,1=torus,2=mirror,3=rim)\n", boundary);
    if (info.is_still_life) {
        printf("Classification: Still life (stable, period 1)\n");
    } else if (info.is_oscillator) {
        printf("Classification: Oscillateur (periode = %d)\n", info.period);
        if (info.is_spaceship) {
            printf("Sous-type: Vaisseau (deplacement par periode: dx=%d dy=%d)\n", info.dx, info.dy);
        }
    } else {
        printf("Classification: Non periodique ou extinction dans %d generations\n", gens);
    }

    world_free(w);
    free(infile);
    return 0;
}
