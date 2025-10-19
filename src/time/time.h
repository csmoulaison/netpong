#ifndef time_h_INCLUDED
#define time_h_INCLUDED

#include "base/base.h"

// Forward declarations: anything which includes time.h must link with a unit
// that implements these.
double platform_time_in_seconds();

#endif
