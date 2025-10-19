#include "time/time.h"

namespace Time {
	double seconds() 
	{
		return platform_time_in_seconds();
	}
}
