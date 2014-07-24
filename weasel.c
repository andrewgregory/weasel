/*
 * Copyright 2014 Andrew Gregory <andrew.gregory.8@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* advanced implementation of Dawkins' weasel program:
 * http://en.wikipedia.org/wiki/Weasel_program */

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *myname = "weasel", *myver = "v1.0.0";

char *pool = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *target = "METHINKS IT IS LIKE A WEASEL";

size_t population_size = 5;
unsigned int rand_seed = 1;
int verbose = 0;

double mutation_seed = 5;
char *value_seed = "";
int reproduction_seed = 1;

size_t tlen, plen;

typedef size_t score_t;
typedef struct {
	int parent;
	int children;
	double sigma;
	score_t score;
	char *value;
} organism_t;

score_t score(char *value) {
	char *t, *s;
	score_t score = 0;
	for(t = target, s = value; *t && *s; ++t, ++s) {
		size_t offset_s = strchr(pool, *s) - pool;
		size_t offset_t = strchr(pool, *t) - pool;
		score += plen - abs(offset_s - offset_t);
	}
	return score;
}

void free_organism(organism_t *o) {
	if(o != NULL) {
		free(o->value);
		free(o);
	}
}

/* http://en.wikipedia.org/wiki/Marsaglia_polar_method */
double rand_gaussian(double sigma) {
	double u, v, s;
	do {
		u = ((double) rand() / RAND_MAX) * 2 - 1;
		v = ((double) rand() / RAND_MAX) * 2 - 1;
		s = (u * u) + (v * v);
	} while (s > 1.0 || s == 0);
	return sigma * v * sqrt(-2.0 * log(s) / s);
}

void mutate(organism_t *o) {
	size_t i;
	for(i = 0; i < tlen; ++i) {
		size_t offset = strchr(pool, o->value[i]) - pool;
		offset += (size_t) rand_gaussian(o->sigma);
		offset %= plen;
		o->value[i] = pool[offset];
	}
	o->children += (int) rand_gaussian(o->sigma);
	if(o->children < 1) { o->children = 1; }
	o->sigma += rand_gaussian(o->sigma);
	if(o->sigma > -0.01 && o->sigma < 0.01) { o->sigma = 0.01; }
	o->score = score(o->value);
}

organism_t *new_organism(void) {
	size_t i;
	organism_t *o = malloc(sizeof(organism_t));
	o->parent = 0;
	o->children = reproduction_seed;
	o->sigma = mutation_seed;
	o->value = calloc(tlen + 1, 1);
	strncpy(o->value, value_seed, tlen);
	for(i = strlen(o->value); i < tlen; ++i) {
		o->value[i] = pool[rand() % plen];
	}
	o->score = score(o->value);
	return o;
}

organism_t *reproduce(organism_t *parent) {
	organism_t *child = malloc(sizeof(organism_t));
	child->children = parent->children;
	child->sigma = parent->sigma;
	child->value = strdup(parent->value);
	mutate(child);
	return child;
}

int organism_beats(organism_t *o, organism_t *challenger) {
	return o->score > challenger->score;
}

void usage(int retcode) {
	FILE *stream = retcode ? stderr : stdout;
#define hputf(format, ...) fprintf(stream, format"\n", __VA_ARGS__);
#define hputs(x) fprintf(stream, x"\n");
	hputf("%s - evolution simulator", myname);
	hputf("usage: %s [options]", myname);
	hputf("usage: %s (--help|--version)", myname);
	hputs("");
	hputs("options:");
	hputs("   -p, --population=i     population limit (default: 5)");
	hputs("   -t, --target=s         target string (default: METHINKS IT IS LIKE A WEASEL)");
	hputs("   -s, --seed=i           provide seed for reproducible runs");
	hputs("   -g, --pool=s           specify the available gene pool");
	hputs("                          (default: ABCDEFGHIJKLMNOPQRSTUVWXYZ)");
	hputs("");
	hputs("   -m, --mutation=i       initial mutation rate (default: 5)");
	hputs("   -r, --reproduction=i   initial reproduction rate (default: 1)");
	hputs("");
	hputs("   -h, --help             display program usage information");
	hputs("   -V, --version          display program version");
	hputs("   -v, --verbose          display additional information");
#undef hputs
#undef hputf
	exit(retcode);
}

void version(void) {
	printf("%s - %s\n", myname, myver);
	exit(0);
}

size_t parse_size_t(const char *value) {
	char *ptr;
	size_t val;
	while(isspace(*value)) { ++value; }
	val = strtoul(optarg, &ptr, 10);
	if(!*value || *value == '-' || *ptr) {
		errno = ERANGE;
	}
	return val;
}

void parse_options(int argc, char **argv) {
	int c;
	const char *short_opts = "g:p:s:t:m:r:vVh";
	struct option long_options[] = {
		{"population",	 required_argument, 0, 'p'},
		{"seed",		 required_argument, 0, 's'},
		{"target",		 required_argument, 0, 't'},
		{"mutation",	 required_argument, 0, 'm'},
		{"reproduction", required_argument, 0, 'r'},
		{"verbose",		 no_argument,		0, 'v'},
		{"version",		 no_argument,		0, 'V'},
		{"help",		 no_argument,		0, 'h'},
		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, short_opts ,long_options, NULL)) != -1) {
		switch(c) {
			case 'r':
				reproduction_seed = parse_size_t(optarg);
				if(reproduction_seed < 1 || errno == ERANGE) {
					fprintf(stderr, "invalid reproduction rate '%s'\n\n", optarg);
					usage(1);
				}
				break;
			case 'p':
				population_size = parse_size_t(optarg);
				if(population_size < 1 || errno == ERANGE) {
					fprintf(stderr, "invalid population size '%s'\n\n", optarg);
					usage(1);
				}
				break;
			case 'm':
				{
					char *ptr;
					mutation_seed = strtod(optarg, &ptr);
					if(!*optarg || *ptr) {
						fprintf(stderr, "invalid mutation rate '%s'\n\n", optarg);
						usage(1);
					}
				}
				break;
			case 's':
				rand_seed = parse_size_t(optarg);
				if(errno == ERANGE) {
					fprintf(stderr, "invalid random seed '%s'\n\n", optarg);
					usage(1);
				}
				break;
			case 't':
				target = optarg;
				break;
			case 'g':
				pool = optarg;
				break;
			case 'V':
				version();
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}
}

int main(int argc, char **argv) {
	organism_t **population;
	size_t i, generation = 1;
	score_t best_score, target_score;

	rand_seed = (unsigned int) time(NULL);

	parse_options(argc, argv);

	srand(rand_seed);

	tlen = strlen(target);
	plen = strlen(pool);
	for(i = 0; i < tlen; ++i) {
		if(strchr(pool, target[i]) == NULL) {
			fprintf(stderr, "invalid target '%s'\n", target);
			fprintf(stderr, "target must consist of '%s'\n\n", pool);
			usage(1);
		}
	}
	population = calloc(sizeof(organism_t*), population_size);
	target_score = score(target);

	population[0] = new_organism();
	best_score = population[0]->score;

	/* start running */
	printf("pool: '%s'\n", pool);
	printf("target: '%s' (%zu)\n", target, target_score);
	printf("population size: %zu\n", population_size);
	printf("random seed: %u\n", rand_seed);
	printf("<id>: (<parent>) '<value>' (<reproduction>/<mutation>) (<score>)\n");
	printf("---[ Generation %zu ]-----------------------------\n", generation++);
	printf("1: '%s' (%u/%f) (%zu)\n",
			population[0]->value, population[0]->children,
			population[0]->sigma, population[0]->score);
	while(best_score < target_score) {
		score_t total_score;
		organism_t **new_population = calloc(sizeof(organism_t*), population_size);
		for(i = 0; i < population_size; ++i) {
			organism_t *parent = population[i];
			int j;
			if(!parent) {
				continue;
			}
			for(j = 0; j < parent->children; ++j) {
				organism_t *child = reproduce(parent);
				int k, saved = 0;
				child->parent = (int) i + 1;
				/* check for open slots */
				for(k = 0; k < population_size && !saved; ++k) {
					if(new_population[k] == NULL) {
						new_population[k] = child;
						saved = 1;
					}
				}
				/* check if we beat any of the current population */
				for(k = 0; k < population_size && !saved; ++k) {
					if(organism_beats(child, new_population[k])) {
						free_organism(new_population[k]);
						new_population[k] = child;
						saved = 1;
					}
				}
				if(child->score > best_score) {
					best_score = child->score;
				}
				if(!saved) {
					free_organism(child);
				}
			}
		}
		/* save the selected generation */
		for(i = 0, total_score = 0; i < population_size; ++i) {
			free_organism(population[i]);
			population[i] = new_population[i];
			if(population[i]) {
				total_score += population[i]->score;
			}
		}
		/* print this generation */
		printf("---[ Generation %zu: Avg Score: %zd (%zd%%) ]------------------\n",
				generation, total_score / population_size,
				(total_score / population_size) * 100 / target_score);
		for(i = 0; i < population_size; ++i) {
			if(population[i]) {
				printf("%zu: (%d) '%s' (%u/%f) (%zu)\n", i+1,
						population[i]->parent,
						population[i]->value, population[i]->children,
						population[i]->sigma, population[i]->score);
			}
		}
		++generation;
		free(new_population);
	}

	/* cleanup */
	for(i = 0; i < population_size; ++i) {
		free_organism(population[i]);
	}
	free(population);
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
