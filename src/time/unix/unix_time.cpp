#include <time.h>

#include "base/base.h"

double platform_time_in_seconds()
{
	struct timespec time_current;
	if(clock_gettime(CLOCK_REALTIME, &time_current)) {
		panic();
	}

	//printf("TIME %f:%f\n", (double)time_current.tv_sec, (double)time_current.tv_nsec / 1'000'000'000.0f);
	return (double)time_current.tv_sec + (double)time_current.tv_nsec / 1'000'000'000.0f;
}
