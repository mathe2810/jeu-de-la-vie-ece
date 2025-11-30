/* main.c - Jeu de la Vie (version finale bit-packed)
 * Projet Game of Life V2 — ECE Embedded C
 */

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>  
#include <inttypes.h>

#ifdef _WIN32
#include <windows.h>
static inline void msleep_ms(double ms) {
	if (ms <= 0.0) return;
	Sleep((DWORD)(ms + 0.5));
}
#else
#include <time.h>
static inline void msleep_ms(double ms) {
	if (ms <= 0.0) return;
	struct timespec ts;
	ts.tv_sec = (time_t)(ms / 1000.0);
	ts.tv_nsec = (long)((ms - (ts.tv_sec * 1000.0)) * 1e6);
	nanosleep(&ts, NULL);
}
#endif


// =============================================================
//  Compatibilité clock_gettime sous Windows
// =============================================================
#ifndef CLOCK_MONOTONIC
#ifdef CLOCK_REALTIME
#define CLOCK_MONOTONIC CLOCK_REALTIME
#else
#define CLOCK_MONOTONIC 0
#endif
#endif

#ifdef _WIN32
static inline int clock_gettime(int clk_id, struct timespec *tp) {
	(void)clk_id;
	if (!tp) return -1;
	FILETIME ft;
	ULARGE_INTEGER uli;
	GetSystemTimeAsFileTime(&ft);
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;

	unsigned long long t = uli.QuadPart;
	t -= 116444736000000000ULL;

	tp->tv_sec  = (time_t)(t / 10000000ULL);
	tp->tv_nsec = (long)((t % 10000000ULL) * 100ULL);
	return 0;
}
#endif



// =============================================================
//  Types
// =============================================================
typedef enum { BOUNDARY_EDGE=0, BOUNDARY_TORUS, BOUNDARY_MIRROR, BOUNDARY_RIM } boundary_t;

typedef struct {
	int width;
	int height;

	size_t words_per_row;   // nombre de uint64_t par ligne
	uint64_t *grid;         // grille bit-packed 1 bit = 1 cellule
	uint64_t *scratch;

	boundary_t boundary;
} world_t;


// =============================================================
//  Outils bit-packed
// =============================================================
static inline uint8_t get_cell(const world_t *w, int x, int y) {
	if (x < 0 || x >= w->width || y < 0 || y >= w->height)
		return 0;

	size_t word_index = (size_t)y * w->words_per_row + (x >> 6);
	int bit = x & 63;

	return (w->grid[word_index] >> bit) & 1ULL;
}

static inline void set_cell(world_t *w, int x, int y, uint8_t val) {
	if (x < 0 || x >= w->width || y < 0 || y >= w->height)
		return; // SAFE: ignore out-of-bounds writes

	size_t word_index = (size_t)y * w->words_per_row + (x >> 6);
	int bit = x & 63;
	uint64_t mask = 1ULL << bit;

	if (val)
		w->grid[word_index] |= mask;
	else
		w->grid[word_index] &= ~mask;
}

static inline void set_cell_scratch(world_t *w, int x, int y, uint8_t val) {
	if (x < 0 || x >= w->width || y < 0 || y >= w->height)
		return; // SAFE: ignore out-of-bounds writes

	size_t word_index = (size_t)y * w->words_per_row + (x >> 6);
	int bit = x & 63;
	uint64_t mask = 1ULL << bit;

	if (val)
		w->scratch[word_index] |= mask;
	else
		w->scratch[word_index] &= ~mask;
}



// =============================================================
//  Gestion des frontières
// =============================================================
static int get_cell_boundary(const world_t *w, int x, int y) {
	if (x >= 0 && x < w->width && y >= 0 && y < w->height)
		return get_cell(w, x, y);

	switch (w->boundary) {

	case BOUNDARY_EDGE:
		return 0;

	case BOUNDARY_RIM:
		return 1;

	case BOUNDARY_TORUS: {
		int xx = (x % w->width + w->width) % w->width;
		int yy = (y % w->height + w->height) % w->height;
		return get_cell(w, xx, yy);
	}

	case BOUNDARY_MIRROR: {
		int xx = x, yy = y;
		if (xx < 0)           xx = -xx - 1;
		if (yy < 0)           yy = -yy - 1;
		if (xx >= w->width)   xx = 2*w->width - xx - 1;
		if (yy >= w->height)  yy = 2*w->height - yy - 1;

		if (xx < 0) xx = 0;
		if (yy < 0) yy = 0;
		if (xx >= w->width)   xx = w->width - 1;
		if (yy >= w->height)  yy = w->height - 1;

		return get_cell(w, xx, yy);
	}
	}
	return 0;
}


// =============================================================
//  Comptage des voisins
// =============================================================
static int count_neighbors(const world_t *w, int x, int y) {
	int sum = 0;
	for (int oy = -1; oy <= 1; oy++)
		for (int ox = -1; ox <= 1; ox++)
			if (!(ox == 0 && oy == 0))
				sum += get_cell_boundary(w, x + ox, y + oy);
	return sum;
}


// =============================================================
//  Génération suivante
// =============================================================
static void next_generation(world_t *w) {
	size_t total_words = (size_t)w->height * w->words_per_row;
	memset(w->scratch, 0, total_words * sizeof(uint64_t));

	for (int y = 0; y < w->height; y++) {
		for (int x = 0; x < w->width; x++) {
			int n = count_neighbors(w, x, y);
			int c = get_cell(w, x, y);
			int next = 0;

			if (c) next = (n == 2 || n == 3);
			else   next = (n == 3);

			set_cell_scratch(w, x, y, next);
		}
	}

	uint64_t *tmp = w->grid;
	w->grid = w->scratch;
	w->scratch = tmp;
}



// =============================================================
//  Affichage ASCII
// =============================================================
static void display_world(const world_t *w) {
	printf("\033[H"); // Move cursor home
	for (int y = 0; y < w->height; y++) {
		for (int x = 0; x < w->width; x++)
			putchar(get_cell(w, x, y) ? '*' : ' ');
		putchar('\n');
	}
	fflush(stdout);
}



// =============================================================
//  Création du monde bit-packed avec limite 64 KiB
// =============================================================
static world_t *world_create(int w, int h, boundary_t b) {
	world_t *wld = malloc(sizeof(world_t));
	if (!wld) return NULL;

	wld->width  = w;
	wld->height = h;
	wld->boundary = b;

	wld->words_per_row = ((size_t)w + 63) / 64;
	size_t total_words = wld->words_per_row * h;
	size_t bytes = total_words * sizeof(uint64_t);

	/* Grid + scratch both allocated -> check total bytes */
	size_t total_bytes = bytes * 2;
	if (total_bytes > 64 * 1024) {
		fprintf(stderr, "ERROR: grid %dx%d = %zu bytes total > 64 KiB limit\n",
				w, h, total_bytes);
		free(wld);
		return NULL;
	}

	wld->grid    = calloc(total_words, sizeof(uint64_t));
	wld->scratch = calloc(total_words, sizeof(uint64_t));

	if (!wld->grid || !wld->scratch) {
		fprintf(stderr, "Allocation error\n");
		free(wld->grid);
		free(wld->scratch);
		free(wld);
		return NULL;
	}

	return wld;
}


// =============================================================
//  Libération
// =============================================================
static void world_free(world_t *w) {
	if (!w) return;
	free(w->grid);
	free(w->scratch);
	free(w);
}



// =============================================================
//  Chargement fichier texte
// =============================================================
static int load_pattern_center(world_t *w, const char *filename) {
	FILE *f = fopen(filename, "r");
	if (!f) return -1;

	char *line = NULL;
	size_t cap = 0;
	ssize_t len;

	char **lines = NULL;
	size_t nlines = 0, maxlen = 0;

	while ((len = getline(&line, &cap, f)) > 0) {
		while (len > 0 && (line[len-1] == '\n' || line[len-1]=='\r'))
			line[--len] = '\0';

		if (len == 0) continue;

		lines = realloc(lines, (nlines+1) * sizeof(char*));
		lines[nlines] = strdup(line);
		if ((size_t)len > maxlen) maxlen = len;
		nlines++;
	}

	free(line);
	fclose(f);

	if (nlines == 0) return 0;

	int patW = maxlen;
	int patH = nlines;
	int offX = (w->width  - patW) / 2;
	int offY = (w->height - patH) / 2;

	if (offX < 0) offX = 0;
	if (offY < 0) offY = 0;

	/* Warn if pattern is larger than the world */
	if (patW > w->width || patH > w->height) {
		fprintf(stderr, "WARNING: pattern %s (%dx%d) is larger than world %dx%d; it will be cropped\n",
				filename, patW, patH, w->width, w->height);
		/* Alternatively, return -1 to refuse large patterns:
		   return -1;
		*/
	}

	for (int r = 0; r < patH; r++) {
		for (int c = 0; c < (int)strlen(lines[r]); c++) {
			char ch = lines[r][c];
			int x = offX + c;
			int y = offY + r;
			if (x >= 0 && x < w->width && y >= 0 && y < w->height)
				set_cell(w, x, y, (ch == '*' || ch=='1' || ch=='X'));
		}
	}

	for (size_t i = 0; i < nlines; i++) free(lines[i]);
	free(lines);

	return 0;
}



// =============================================================
//  Sauvegarde
// =============================================================
static int save_world(const world_t *w, const char *filename) {
	FILE *f = fopen(filename, "w");
	if (!f) return -1;

	for (int y = 0; y < w->height; y++) {
		for (int x = 0; x < w->width; x++)
			fputc(get_cell(w, x, y) ? '*' : '.', f);
		fputc('\n', f);
	}
	fclose(f);
	return 0;
}


// =============================================================
//  Différence timespec
// =============================================================
static double timespec_diff_ms(const struct timespec *a, const struct timespec *b) {
	return (b->tv_sec - a->tv_sec) * 1000.0
		 + (b->tv_nsec - a->tv_nsec) / 1e6;
}



// =============================================================
//  MAIN
// =============================================================
static void usage(const char *p) {
	fprintf(stderr,
	"Usage: %s [options]\n"
	"  --width W\n"
	"  --height H\n"
	"  --gens N\n"
	"  --boundary edge|torus|mirror|rim\n"
	"  --in file\n"
	"  --out file\n"
	"  --target-hz N\n",
	p);
}


int main(int argc, char **argv) {

	int width = 40, height = 20;
	int gens = 200;
	int target_hz = 10;
	char *infile = NULL;
	char *outfile = NULL;
	boundary_t boundary = BOUNDARY_TORUS;

	static struct option opts[] = {
		{"width", required_argument, NULL, 0},
		{"height", required_argument, NULL, 0},
		{"gens", required_argument, NULL, 0},
		{"boundary", required_argument, NULL, 0},
		{"in", required_argument, NULL, 0},
		{"out", required_argument, NULL, 0},
		{"target-hz", required_argument, NULL, 0},
		{"help", no_argument, NULL, 'h'},
		{0,0,0,0}
	};

	int idx, c;
	while ((c = getopt_long(argc, argv, "h", opts, &idx)) != -1) {
		if (c == 'h') { usage(argv[0]); return 0; }
		if (c == 0) {
			char *name = (char*)opts[idx].name;
			if (!strcmp(name,"width")) width = atoi(optarg);
			else if (!strcmp(name,"height")) height = atoi(optarg);
			else if (!strcmp(name,"gens")) gens = atoi(optarg);
			else if (!strcmp(name,"target-hz")) target_hz=atoi(optarg);
			else if (!strcmp(name,"in")) infile=strdup(optarg);
			else if (!strcmp(name,"out")) outfile=strdup(optarg);
			else if (!strcmp(name,"boundary")) {
				if (!strcmp(optarg,"edge")) boundary=BOUNDARY_EDGE;
				else if (!strcmp(optarg,"torus")) boundary=BOUNDARY_TORUS;
				else if (!strcmp(optarg,"mirror")) boundary=BOUNDARY_MIRROR;
				else if (!strcmp(optarg,"rim")) boundary=BOUNDARY_RIM;
				else { fprintf(stderr,"Unknown boundary\n"); return 1; }
			}
		}
	}

	// ensure dimensions are positive
	if (width < 1) width = 1;
	if (height < 1) height = 1;

	// Maximum total memory: 64 KiB for both grids (grid + scratch)
	const size_t MAX_TOTAL_BYTES = 64 * 1024;
	
	// Calculate actual memory needed for requested dimensions
	size_t words_per_row = ((size_t)width + 63) / 64;
	size_t total_words = words_per_row * (size_t)height;
	size_t bytes_needed = total_words * sizeof(uint64_t) * 2; // grid + scratch

	if (bytes_needed > MAX_TOTAL_BYTES) {
		// Compute proportional scale to keep aspect ratio
		// Target: words_per_row * height * 8 * 2 <= 65536
		// Approximate: width * height / 64 * 8 * 2 <= 65536
		// Simplified: width * height <= 65536 * 4 = 262144
		const uint64_t MAX_CELLS = MAX_TOTAL_BYTES / 2 / 8 * 64; // 262144
		uint64_t requested_cells = (uint64_t)width * (uint64_t)height;
		
		double scale = sqrt((double)MAX_CELLS / (double)requested_cells);
		int new_width = (int)(width * scale);
		int new_height = (int)(height * scale);
		if (new_width < 1) new_width = 1;
		if (new_height < 1) new_height = 1;

		// Verify and adjust conservatively
		while (1) {
			size_t wpr = ((size_t)new_width + 63) / 64;
			size_t tw = wpr * (size_t)new_height;
			size_t bytes = tw * sizeof(uint64_t) * 2;
			if (bytes <= MAX_TOTAL_BYTES) break;
			
			// Reduce larger dimension
			if (new_width >= new_height) {
				if (new_width > 1) new_width--;
				else break;
			} else {
				if (new_height > 1) new_height--;
				else break;
			}
		}

		fprintf(stderr,
			"Requested grid %dx%d exceeds 64 KiB. Resizing to %dx%d to fit.\n",
			width, height, new_width, new_height);

		width = new_width;
		height = new_height;
	}

	// Now try to create the world
	world_t *w = world_create(width, height, boundary);
	if (!w) {
		fprintf(stderr, "Failed to create world after resize. This should not happen.\n");
		return 1;
	}

	printf("\033[2J\033[?25l");

	if (infile) {
		if (load_pattern_center(w, infile) != 0)
			fprintf(stderr,"Failed to load %s\n", infile);
	} else {
		srand(time(NULL));
		for (int y = 0; y < w->height; y++)
			for (int x = 0; x < w->width; x++)
				set_cell(w, x, y, (rand()%4==0));
	}

	double *samples = calloc(gens, sizeof(double));
	double target_ms = 1000.0 / target_hz;

	struct timespec t0, t1;

	for (int g = 0; g < gens; g++) {
		clock_gettime(CLOCK_MONOTONIC, &t0);

		display_world(w);
		next_generation(w);

		clock_gettime(CLOCK_MONOTONIC, &t1);

		double elapsed = timespec_diff_ms(&t0,&t1);
		samples[g] = elapsed;

		double to_sleep = target_ms - elapsed;
		if (to_sleep > 0) msleep_ms(to_sleep);
	}

	printf("\033[?25h");

	double sum=0, worst=0;
	for (int i = 0; i < gens; i++) {
		sum += samples[i];
		if (samples[i] > worst) worst = samples[i];
	}
	double mean = sum / gens;
	fprintf(stderr,"Stats: mean=%.3fms worst=%.3fms jitter=%.3fms\n",
			mean, worst, worst-mean);

	if (outfile) save_world(w, outfile);

	free(samples);
	world_free(w);
	free(infile);
	free(outfile);
	return 0;
}
