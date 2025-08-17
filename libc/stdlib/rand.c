#include <stdlib.h>

static unsigned long state = 0;

int rand()
{
    state = (1664525*state + 1013904223) % RAND_MAX;
    return state;
}

void srand(unsigned int seed)
{
    state = seed;
}
