#ifndef random_h_INCLUDED
#define random_h_INCLUDED

#include <time.h>

void random_init();
float random_float();

#ifdef CSM_BASE_IMPLEMENTATION

void random_init() {
	srand(time(nullptr));
}

float random_float() {
	return (float)rand() / (float)RAND_MAX;
}

#endif

#endif
