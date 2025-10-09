#ifndef random_h_INCLUDED
#define random_h_INCLUDED

#include <time.h>

void random_init();
f32 random_f32();

#ifdef CSM_BASE_IMPLEMENTATION

void random_init() {
	srand(time(nullptr));
}

f32 random_f32() {
	return (f32)rand() / (f32)RAND_MAX;
}

#endif

#endif
