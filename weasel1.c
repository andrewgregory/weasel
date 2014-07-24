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

/* basic implementation of Dawkins' original weasel program:
 * http://en.wikipedia.org/wiki/Weasel_program */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *myname = "weasal1", *myver = "v1.0.0";

char *target = "METHINKS IT IS LIKE A WEASEL";
char *pool = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
int generation_size = 100;
int mutation_rate = 5; /* out of 100 */
int verbose = 0; /* display all children */
int rand_seed = 0; /* 0 for current time */

size_t score(char *string) {
    char *t, *s;
    size_t score = 0;
    for(t = target, s = string; *t && *s; ++t, ++s) {
        if(*t == *s) { ++score; }
    }
    return score;
}

char rand_char(void) {
    return pool[rand() % strlen(pool)];
}

void mutate(char *string) {
    for(; *string; ++string) {
        if(rand() % 100 < mutation_rate) { *string = rand_char(); }
    }
}

int main(int argc, char** argv) {
    size_t i, candidate_score, generation, tlen = strlen(target);
    char candidate[tlen + 1];

    rand_seed = rand_seed ? rand_seed : time(NULL);
    srand(rand_seed);

    for(i = 0; i < tlen; ++i) {
        candidate[i] = rand_char();
    }
    candidate_score = score(candidate);

    printf("target: '%s'\n", target);
    printf("pool: '%s'\n", pool);
    printf("population size: %d\n", generation_size);
    printf("mutation rate: %d/100\n", mutation_rate);
    printf("random seed: %d\n", rand_seed);
    printf("------------------------------------------------\n");
    printf("%zd: '%s' (%zd)\n", 1, candidate, candidate_score);
    for(generation = 2; candidate_score != tlen; ++generation) {
        char base[tlen + 1];
        strcpy(base, candidate);
        candidate_score = 0;
        for(i = 1; i <= generation_size; ++i) {
            char child[tlen + 1];
            size_t child_score;
            strcpy(child, base);
            mutate(child);
            child_score = score(child);
            if(verbose) {
                printf("    child %zd: '%s' (%zd)\n", i, child, child_score);
            }
            if(child_score >= candidate_score) {
                strcpy(candidate, child);
                candidate_score = child_score;
            }
        }
        printf("%zd: '%s' (%zd)\n", generation, candidate, candidate_score);
    }

    return 0;
}
